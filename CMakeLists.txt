cmake_minimum_required(VERSION 3.24)

project(ProtonG VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

find_package(Vulkan REQUIRED)

set(LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_WEB OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/luau)

add_executable(ProtonG
        src/main.cpp
        src/ProtonApplication.cpp
        src/ProtonLog.cpp
        src/ProtonVulkanContext.cpp
        src/ProtonSystemStats.cpp
        src/ProtonScriptEngine.cpp
        src/ProtonImageLoader.cpp
        src/ProtonMediaPlayer.cpp
)

add_custom_command(
        TARGET ProtonG
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf
        $<TARGET_FILE_DIR:ProtonG>/sandbox
        COMMENT "Cleaning copied Proton G sandbox runtime folder"
)

add_custom_command(
        TARGET ProtonG
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/sandbox
        $<TARGET_FILE_DIR:ProtonG>/sandbox
        COMMENT "Copying Proton G sandbox runtime folder"
)

target_compile_definitions(ProtonG PRIVATE NOMINMAX)

target_include_directories(ProtonG
        PRIVATE
        src
        third_party/stb
        third_party/miniaudio
)

target_link_libraries(ProtonG
        PRIVATE
        glfw
        Vulkan::Vulkan

        Luau.Compiler
        Luau.VM
)