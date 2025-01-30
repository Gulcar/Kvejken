cmake_minimum_required(VERSION 3.20)

project(Kvejken C CXX)

add_executable(${PROJECT_NAME}
    src/Main.cpp
    src/Renderer.cpp
    src/Model.cpp
    src/ECS.cpp
    src/Input.cpp
    src/Collision.cpp
    src/Player.cpp
    src/Enemy.cpp
    src/Interactable.cpp
    libs/glad/src/glad.c
    libs/stb/stb_image.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

add_subdirectory(libs/glfw/)
add_subdirectory(libs/glm/)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glm::glm
)

target_include_directories(${PROJECT_NAME} PRIVATE
    libs/glad/include/
    libs/stb/
)
