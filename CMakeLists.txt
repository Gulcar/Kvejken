cmake_minimum_required(VERSION 3.20)

project(Kvejken C CXX)

add_executable(${PROJECT_NAME}
    src/Main.cpp
    libs/glad/src/glad.c
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

add_subdirectory(libs/glfw/)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
)

target_include_directories(${PROJECT_NAME} PRIVATE
    libs/glad/include/
)
