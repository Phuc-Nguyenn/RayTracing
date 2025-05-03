cmake_minimum_required(VERSION 3.27)

project(ray_tracer VERSION 0.1.0)

set(CMAKE_BUILD_TYPE Debug)

# Add the executable
add_executable(ray_tracer src/Shader.cpp src/main.cpp)

# Include directories
target_include_directories(ray_tracer PRIVATE ./Include)

# Add the glfw subdirectory and link it as a dependency
target_link_libraries(ray_tracer PRIVATE glfw GL GLEW)
