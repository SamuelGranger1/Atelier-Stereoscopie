cmake_minimum_required(VERSION 3.10)
project(LaboMoteurVR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(vr
    main.cpp
)

if (WIN32)
    # Noms des libs sous MSYS2 / MinGW64
    target_link_libraries(vr
        PRIVATE
            glfw3
            glew32
            opengl32
    )
else()
    # Noms des libs sous Linux / Docker
    target_link_libraries(vr
        PRIVATE
            glfw
            GLEW
            GL
    )
endif()
