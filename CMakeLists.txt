cmake_minimum_required(VERSION 3.27)

project(ray_tracer VERSION 0.1.0)

set(CMAKE_BUILD_TYPE Debug)

add_executable(ray_tracer src/Shader.cpp src/main.cpp)

target_include_directories(ray_tracer PRIVATE ./Include)

target_link_libraries(ray_tracer glfw GL GLEW)