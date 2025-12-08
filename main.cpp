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
    float yaw   = 0.0f; // rotation gauche/droite
    float pitch = 0.0f; // regarder haut/bas
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

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Mini VR Engine - Vue Stereo", nullptr, nullptr);
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

    // --- Données cube ---
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

    // Matrice de projection (pour chaque oeil, on ajustera l'aspect)
    float ipd = 0.064f; // distance interpupillaire en mètres

    CameraState cam;
    double lastTime = glfwGetTime();

    // --- Boucle principale ---
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        glfwPollEvents();
        updateCamera(window, cam, dt);

        // Vision "forward" et vecteur right recalculés
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

        // Décalage des yeux (gauche/droite)
        glm::vec3 eyeCenter = cam.position;
        glm::vec3 eyeLeft   = eyeCenter - right * (ipd * 0.5f);
        glm::vec3 eyeRight  = eyeCenter + right * (ipd * 0.5f);

        glm::mat4 viewLeft  = glm::lookAt(eyeLeft,  eyeLeft  + forward, up);
        glm::mat4 viewRight = glm::lookAt(eyeRight, eyeRight + forward, up);

        // Modèle (cube qui tourne devant nous)
        static float angle = 0.0f;
        angle += dt * 0.8f;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 1.2f, 0.0f));
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        glEnable(GL_SCISSOR_TEST); // pour bien nettoyer chaque oeil

        glUseProgram(program);
        glBindVertexArray(vao);

        // --- Oeil gauche : vue à gauche de l'écran ---
        glViewport(0, 0, (int)halfWidth, HEIGHT);
        glScissor(0, 0, (int)halfWidth, HEIGHT);
        glClearColor(0.05f, 0.05f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(locModel, 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(locView,  1, GL_FALSE, &viewLeft[0][0]);
        glUniformMatrix4fv(locProj,  1, GL_FALSE, &projLeft[0][0]);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // --- Oeil droit : vue à droite de l'écran ---
        glViewport((int)halfWidth, 0, (int)halfWidth, HEIGHT);
        glScissor((int)halfWidth, 0, (int)halfWidth, HEIGHT);
        glClearColor(0.05f, 0.1f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(locView,  1, GL_FALSE, &viewRight[0][0]);
        glUniformMatrix4fv(locProj,  1, GL_FALSE, &projRight[0][0]);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);
        glDisable(GL_SCISSOR_TEST);

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwTerminate();
    return 0;
}
