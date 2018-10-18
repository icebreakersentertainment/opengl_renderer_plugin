cmake_minimum_required(VERSION 3.1.0)

project(opengl_renderer_plugin)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(OPENGL_RENDERER_PLUGIN_COMPILER_FLAGS "")
if(MSVC)
  set(OPENGL_RENDERER_PLUGIN_COMPILER_FLAGS "/EHsc /MP")
  if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(OPENGL_RENDERER_PLUGIN_COMPILER_FLAGS "/EHsc /MP")
  endif()
endif()

if(NOT TARGET glew)
  add_subdirectory(./glew/glew/build/cmake)
endif()

if(NOT TARGET SDL2)
    add_subdirectory(./sdl/SDL)
endif()

# Headers
include_directories(include)

# Source
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(opengl_renderer_plugin SHARED ${SOURCES})

target_include_directories(opengl_renderer_plugin PRIVATE ${ICEENGINE_INCLUDE_DIRS})
target_include_directories(opengl_renderer_plugin PRIVATE ${BOOST_INCLUDE_DIRS})
target_include_directories(opengl_renderer_plugin PRIVATE ${GLM_INCLUDE_DIRS})
target_include_directories(opengl_renderer_plugin PRIVATE glew/glew/include)
target_include_directories(opengl_renderer_plugin PRIVATE sdl/SDL/include)

target_compile_options(opengl_renderer_plugin PRIVATE ${OPENGL_RENDERER_PLUGIN_COMPILER_FLAGS})

target_link_libraries(opengl_renderer_plugin glew)
target_link_libraries(opengl_renderer_plugin SDL2)

# Copy our shaders
file(COPY ./include//gl33/shaders DESTINATION ./)