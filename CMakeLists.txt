cmake_minimum_required(VERSION 3.27)

project(ray_tracer VERSION 0.1.0)


set(CMAKE_CXX_COMPILER "/usr/bin/g++") 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O5")

# Add the executable
add_executable(ray_tracer 
src/Shader.cpp 
src/main.cpp 
src/TextureUnitManager.cpp
src/VertexBuffer.cpp
src/BvhTree.cpp
src/KeyEventNotifier.cpp
src/KeyEventObserver.cpp
src/Camera.cpp
src/ObjectLoader.cpp
src/Scene.cpp
src/BounceLimitManager.cpp
src/ConfigParser/ConfigParser.cpp)

# Include directories
target_include_directories(ray_tracer PRIVATE ./Include ./Include/ConfigParser ./Include/StbImage)

# Add the glfw subdirectory and link it as a dependency
target_link_libraries(ray_tracer PRIVATE glfw GL GLEW)


