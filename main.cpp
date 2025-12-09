#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

// --- Utilitaire pour charger un fichier texte (glsl) ---
std::string loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Erreur: impossible d'ouvrir le fichier " << path << "\n";
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// --- Compilation d'un shader ---
GLuint compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        std::cerr << "Erreur de compilation shader :\n" << log << "\n";
    }
    return shader;
}

// --- Création du programme shader complet ---
GLuint createProgram(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSrc = loadFile(vertPath);
    std::string fragSrc = loadFile(fragPath);
    if (vertSrc.empty() || fragSrc.empty()) {
        std::cerr << "Erreur: shaders vides\n";
        return 0;
    }

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, log.data());
        std::cerr << "Erreur de linkage du programme :\n" << log << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return prog;
}

// --- Etat de la "tête" VR (caméra) ---
struct CameraState {
    glm::vec3 position = glm::vec3(0.0f, 1.6f, 5.0f); // hauteur humaine + recul
    float yaw   = 0.0f;  // rotation gauche/droite
    float pitch = 0.0f;  // regarder haut/bas
};

void updateCamera(GLFWwindow* window, CameraState& cam, float dt) {
    const float moveSpeed = 3.0f;   // m/s
    const float rotSpeed  = 1.5f;   // rad/s

    // Calcul du vecteur "forward" et "right" selon yaw/pitch
    glm::vec3 forward;
    forward.x = cosf(cam.pitch) * sinf(cam.yaw);
    forward.y = sinf(cam.pitch);
    forward.z = cosf(cam.pitch) * cosf(cam.yaw) * -1.0f; // -Z vers l'avant

    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);

    // Déplacements (WASDQE)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cam.position += forward * moveSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cam.position -= forward * moveSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cam.position -= right * moveSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cam.position += right * moveSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cam.position += up * moveSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cam.position -= up * moveSpeed * dt;

    // Rotations (flèches)
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cam.yaw += rotSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cam.yaw -= rotSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        cam.pitch += rotSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        cam.pitch -= rotSpeed * dt;

    // Clamp du pitch pour éviter de se retourner totalement
    const float maxPitch = glm::radians(89.0f);
    if (cam.pitch > maxPitch) cam.pitch = maxPitch;
    if (cam.pitch < -maxPitch) cam.pitch = -maxPitch;
}

// --- Fonction utilitaire : dessiner la scène (sol + plusieurs cubes) ---
void drawScene(GLuint vao,
               GLint locModel,
               GLint locColor,
               float timeSeconds)
{
    glBindVertexArray(vao);

    // 1) Cube central qui tourne (bleu)
    glm::mat4 modelCenter = glm::mat4(1.0f);
    modelCenter = glm::translate(modelCenter, glm::vec3(0.0f, 1.2f, 0.0f));
    modelCenter = glm::rotate(modelCenter, timeSeconds * 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(locModel, 1, GL_FALSE, &modelCenter[0][0]);
    glUniform4f(locColor, 0.15f, 0.55f, 1.0f, 1.0f); // bleu
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // 2) Cube proche à gauche (orange/rouge)
    glm::mat4 modelNear = glm::mat4(1.0f);
    modelNear = glm::translate(modelNear, glm::vec3(-1.5f, 0.8f, -1.0f));
    glUniformMatrix4fv(locModel, 1, GL_FALSE, &modelNear[0][0]);
    glUniform4f(locColor, 1.0f, 0.45f, 0.1f, 1.0f); // orange
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // 3) Cube loin à droite (vert)
    glm::mat4 modelFar = glm::mat4(1.0f);
    modelFar = glm::translate(modelFar, glm::vec3(2.5f, 0.8f, -8.0f));
    glUniformMatrix4fv(locModel, 1, GL_FALSE, &modelFar[0][0]);
    glUniform4f(locColor, 0.2f, 0.9f, 0.3f, 1.0f); // vert
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // 4) Sol gris très large
    glm::mat4 modelFloor = glm::mat4(1.0f);
    modelFloor = glm::translate(modelFloor, glm::vec3(0.0f, -0.2f, -5.0f));
    modelFloor = glm::scale(modelFloor, glm::vec3(30.0f, 0.05f, 30.0f));
    glUniformMatrix4fv(locModel, 1, GL_FALSE, &modelFloor[0][0]);
    glUniform4f(locColor, 0.85f, 0.85f, 0.85f, 1.0f); // gris clair
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

int main() {
    // --- Initialisation GLFW ---
    if (!glfwInit()) {
        std::cerr << "Erreur: Impossible d'initialiser GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    const int WIDTH  = 1600;
    const int HEIGHT = 900;

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Mini VR Engine - Stereo Demo", nullptr, nullptr);
    if (!window) {
        std::cerr << "Erreur: Impossible de créer la fenêtre GLFW\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // --- GLEW ---
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Erreur: glewInit a échoué : " << glewGetErrorString(err) << "\n";
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // --- Programme shader ---
    GLuint program = createProgram("shaders/vertex.glsl", "shaders/fragment.glsl");
    if (program == 0) {
        glfwTerminate();
        return -1;
    }

    // --- Données cube (cube unité) ---
    float cubeVertices[] = {
        // Face avant
        -1.f, -1.f,  1.f,   1.f, -1.f,  1.f,   1.f,  1.f,  1.f,
        -1.f, -1.f,  1.f,   1.f,  1.f,  1.f,  -1.f,  1.f,  1.f,
        // Face arrière
        -1.f, -1.f, -1.f,   1.f,  1.f, -1.f,   1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f,  -1.f,  1.f, -1.f,   1.f,  1.f, -1.f,
        // Face gauche
        -1.f, -1.f, -1.f,  -1.f, -1.f,  1.f,  -1.f,  1.f,  1.f,
        -1.f, -1.f, -1.f,  -1.f,  1.f,  1.f,  -1.f,  1.f, -1.f,
        // Face droite
         1.f, -1.f, -1.f,   1.f,  1.f,  1.f,   1.f, -1.f,  1.f,
         1.f, -1.f, -1.f,   1.f,  1.f, -1.f,   1.f,  1.f,  1.f,
        // Face haut
        -1.f,  1.f, -1.f,  -1.f,  1.f,  1.f,   1.f,  1.f,  1.f,
        -1.f,  1.f, -1.f,   1.f,  1.f,  1.f,   1.f,  1.f, -1.f,
        // Face bas
        -1.f, -1.f, -1.f,   1.f, -1.f,  1.f,  -1.f, -1.f,  1.f,
        -1.f, -1.f, -1.f,   1.f, -1.f, -1.f,   1.f, -1.f,  1.f
    };

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); // layout (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // --- Uniforms ---
    GLint locModel = glGetUniformLocation(program, "model");
    GLint locView  = glGetUniformLocation(program, "view");
    GLint locProj  = glGetUniformLocation(program, "proj");
    GLint locColor = glGetUniformLocation(program, "colorOverride");

    // IPD EXAGÉRÉE pour que la différence soit évidente sur écran
    float ipd = 0.30f; // 30 cm pour la démo (dans la vraie vie ~0.064f)

    CameraState cam;
    bool stereoEnabled = true;
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        glfwPollEvents();

        // Toggle mono/stéréo avec la touche M
        static bool mWasPressed = false;
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
            if (!mWasPressed) {
                stereoEnabled = !stereoEnabled;
                std::cout << (stereoEnabled ? "Mode = STEREO" : "Mode = MONO") << "\n";
            }
            mWasPressed = true;
        } else {
            mWasPressed = false;
        }

        updateCamera(window, cam, dt);

        // Recalcul de la direction de la caméra
        glm::vec3 forward;
        forward.x = cosf(cam.pitch) * sinf(cam.yaw);
        forward.y = sinf(cam.pitch);
        forward.z = cosf(cam.pitch) * cosf(cam.yaw) * -1.0f;

        glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
        glm::vec3 up    = glm::normalize(glm::cross(right, forward));

        // Projections pour chaque oeil (moitié d'écran)
        float halfWidth = WIDTH * 0.5f;
        glm::mat4 projLeft  = glm::perspective(glm::radians(90.0f), halfWidth / (float)HEIGHT, 0.1f, 100.0f);
        glm::mat4 projRight = projLeft;

        glm::vec3 eyeCenter = cam.position;
        glm::vec3 eyeLeft;
        glm::vec3 eyeRight;
        glm::mat4 viewLeft;
        glm::mat4 viewRight;

        if (stereoEnabled) {
            // --- MODE STEREO : deux caméras légèrement décalées ---
            eyeLeft  = eyeCenter - right * (ipd * 0.5f);
            eyeRight = eyeCenter + right * (ipd * 0.5f);

            viewLeft  = glm::lookAt(eyeLeft,  eyeLeft  + forward, up);
            viewRight = glm::lookAt(eyeRight, eyeRight + forward, up);
        } else {
            // --- MODE MONO : même point de vue dans les deux moitiés ---
            glm::mat4 viewMono = glm::lookAt(eyeCenter, eyeCenter + forward, up);
            viewLeft  = viewMono;
            viewRight = viewMono;
        }

        glUseProgram(program);
        glEnable(GL_SCISSOR_TEST);

        // --- Oeil gauche : vue à gauche de l'écran ---
        glViewport(0, 0, (int)halfWidth, HEIGHT);
        glScissor(0, 0, (int)halfWidth, HEIGHT);
        glClearColor(0.03f, 0.03f, 0.12f, 1.0f);  // bleu nuit
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(locView,  1, GL_FALSE, &viewLeft[0][0]);
        glUniformMatrix4fv(locProj,  1, GL_FALSE, &projLeft[0][0]);
        drawScene(vao, locModel, locColor, static_cast<float>(currentTime));

        // --- Oeil droit : vue à droite de l'écran ---
        glViewport((int)halfWidth, 0, (int)halfWidth, HEIGHT);
        glScissor((int)halfWidth, 0, (int)halfWidth, HEIGHT);
        glClearColor(0.02f, 0.09f, 0.04f, 1.0f); // vert très foncé
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(locView,  1, GL_FALSE, &viewRight[0][0]);
        glUniformMatrix4fv(locProj,  1, GL_FALSE, &projRight[0][0]);
        drawScene(vao, locModel, locColor, static_cast<float>(currentTime));

        glDisable(GL_SCISSOR_TEST);
        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwTerminate();
    return 0;
}
