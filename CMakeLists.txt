cmake_minimum_required(VERSION 3.0)

cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0072 NEW)

project(SRGGE)

set (CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

execute_process(COMMAND ln -s ../shaders)

set(appName SRGGE)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLM REQUIRED)

include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(${GLFW3_INCLUDE_DIRS})
include_directories(${GLM_INCLUDE_DIRS})
include_directories("${CMAKE_SOURCE_DIR}/gl3w")

link_directories(${OPENGL_LIBRARY_DIRS})
link_directories(${GLFW3_LIBRARY_DIRS})

set(gl3w_files gl3w/gl3w.c gl3w/GL/gl3w.h)

add_executable(${appName} ${gl3w_files} PLYReader.h PLYReader.cpp TriangleMesh.h TriangleMesh.cpp TriangleMeshInstance.h TriangleMeshInstance.cpp VectorCamera.h VectorCamera.cpp Scene.h Scene.cpp Shader.h Shader.cpp ShaderProgram.h ShaderProgram.cpp Application.h Application.cpp main.cpp)

target_link_libraries(${appName} ${OPENGL_LIBRARIES} glfw)



