cmake_minimum_required(VERSION 3.15)
project(ShaderSaver LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ShaderSaver is deliberately OpenGL + GLFW only. These are normal (non-cache)
# variables so the meta build's sibling projects keep their own backend choices.
set(FE_EXCLUDE_GLFW OFF)
set(FE_EXCLUDE_VULKAN ON)
set(FE_EXCLUDE_OPENXR ON)
set(FE_EXCLUDE_KISSFFT ON)
set(FE_EXCLUDE_IMGUI ON)

option(SHADERSAVER_BUILD_SDL "Build the optional SDL backend variant" OFF)
option(SHADERSAVER_BUILD_PINGPONG "Build the optional ping-pong variant" OFF)

set(ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../FenixEngine")

if(NOT TARGET glfw)
    if(NOT EXISTS "${ENGINE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "FenixEngine not found at ${ENGINE_DIR}")
    endif()
    add_subdirectory("${ENGINE_DIR}" FenixEngine)
endif()

# Only the objects this app touches: GLFW window, GL device and their GL
# resource helpers. No Renderer.lib, Vulkan, ImGui, physics, audio or XR.
add_library(ShaderSaverGL OBJECT
    ${ENGINE_DIR}/src/window/GLFW3Window.cpp
    ${ENGINE_DIR}/src/glad/glad.c
    ${ENGINE_DIR}/src/Graphics/OpenGLRenderDevice.cpp
    ${ENGINE_DIR}/src/Graphics/OpenGLGPUBuffers.cpp
    ${ENGINE_DIR}/src/Graphics/OpenGLGPUTexture.cpp
    ${ENGINE_DIR}/src/ImageLoader.cpp
)

if(SHADERSAVER_BUILD_SDL)
    target_sources(ShaderSaverGL PRIVATE ${ENGINE_DIR}/src/window/SDLWindow.cpp)
endif()

target_include_directories(ShaderSaverGL PUBLIC
    ${ENGINE_DIR}/src
    ${ENGINE_DIR}/include
    ${ENGINE_DIR}/external/glm
)

target_compile_definitions(ShaderSaverGL PUBLIC GLFW_INCLUDE_NONE)
target_link_libraries(ShaderSaverGL PUBLIC glfw)

if(SHADERSAVER_BUILD_SDL)
    target_link_libraries(ShaderSaverGL PUBLIC SDL3::SDL3)
endif()

if(WIN32)
    target_link_libraries(ShaderSaverGL PUBLIC opengl32 gdi32 user32 shell32)
elseif(APPLE)
    target_link_libraries(ShaderSaverGL PUBLIC "-framework OpenGL")
else()
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(ShaderSaverGL PUBLIC OpenGL::GL X11::X11)
endif()

target_precompile_headers(ShaderSaverGL PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${ENGINE_DIR}/src/pch.hpp>"
)

file(GLOB SHADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/FragmentShader*.glsl)

function(add_saver_executable target source)
    add_executable(${target} WIN32 ${source})
    target_link_libraries(${target} PRIVATE ShaderSaverGL)
    set_property(TARGET ${target} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_FILES} $<TARGET_FILE_DIR:${target}>
    )
    if(MSVC)
        target_link_options(${target} PRIVATE $<$<CONFIG:Debug>:/DEBUG>)
    endif()
    if(EMSCRIPTEN)
        set_target_properties(${target} PROPERTIES SUFFIX ".html")
        target_link_options(${target} PRIVATE
            "-s USE_WEBGL2=1"
            "-s FULL_ES3=1"
            "-sGL_ENABLE_GET_PROC_ADDRESS=1"
            "-s ALLOW_MEMORY_GROWTH=1"
            "-s MIN_WEBGL_VERSION=2"
            "-s MAX_WEBGL_VERSION=2"
            "-sFORCE_FILESYSTEM=1"
            "--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/FragmentShader.glsl"
        )
    endif()
endfunction()

add_saver_executable(ShaderSaver main.cpp)

if(SHADERSAVER_BUILD_SDL)
    add_saver_executable(ShaderSaverSDL mainSDL.cpp)
endif()

if(SHADERSAVER_BUILD_PINGPONG)
    add_saver_executable(ShaderSaverPingPong main.cpp)
    target_compile_definitions(ShaderSaverPingPong PRIVATE SHADERSAVER_PINGPONG)
endif()
