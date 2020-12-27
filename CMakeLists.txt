cmake_minimum_required(VERSION 3.1.0)

project(opengl_renderer_plugin)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(OPENGL_RENDERER_PLUGIN_COMPILER_FLAGS "")
set(OPENGL_RENDERER_PLUGIN_DEFINITIONS "")
if(MSVC)
  list(APPEND OPENGL_RENDERER_PLUGIN_COMPILER_FLAGS /EHsc /MP)
  if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    list(APPEND OPENGL_RENDERER_PLUGIN_COMPILER_FLAGS /EHsc /MP)
  endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo OR ICEENGINE_ENABLE_DEBUG_LOGGING)
  list(APPEND OPENGL_RENDERER_PLUGIN_DEFINITIONS -DICEENGINE_ENABLE_DEBUG_LOGGING)
endif()

if(ICEENGINE_ENABLE_TRACE_LOGGING)
  list(APPEND OPENGL_RENDERER_PLUGIN_DEFINITIONS -DICEENGINE_ENABLE_TRACE_LOGGING)
endif()

find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Boost REQUIRED COMPONENTS stacktrace)

# Source
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(APPEND SOURCES "${ICEENGINE_BASE_DIR}/src/exceptions/Exception.cpp")
list(APPEND SOURCES "${ICEENGINE_BASE_DIR}/src/exceptions/Stacktrace.cpp")

add_library(opengl_renderer_plugin SHARED ${SOURCES})

# Headers
target_include_directories(opengl_renderer_plugin PRIVATE include)

target_include_directories(opengl_renderer_plugin PRIVATE ${ICEENGINE_INCLUDE_DIRS})

target_compile_definitions(opengl_renderer_plugin PRIVATE ${OPENGL_RENDERER_PLUGIN_DEFINITIONS})
target_compile_options(opengl_renderer_plugin PRIVATE ${OPENGL_RENDERER_PLUGIN_COMPILER_FLAGS})

target_link_libraries(opengl_renderer_plugin PRIVATE GLEW::GLEW)
target_link_libraries(opengl_renderer_plugin PRIVATE glm::glm)
target_link_libraries(opengl_renderer_plugin PRIVATE SDL2::SDL2)
target_link_libraries(opengl_renderer_plugin PRIVATE Boost::stacktrace)

# Copy our shaders
file(COPY ./include/gl33/shaders DESTINATION ./)