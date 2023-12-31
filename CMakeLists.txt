cmake_minimum_required(VERSION 3.0)
message("EstEngine Project - CMake Build System")

# check if Environment VULKAN_SDK is set
if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "Environment VULKAN_SDK is required to compile the shaders!")
endif()

project(EstEngine)

set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_compile_options(/utf-8 /D_CRT_SECURE_NO_WARNINGS)
else()
    set(CMAKE_C_FLAGS "-m64")
    set(CMAKE_CXX_FLAGS "-m64")
    add_definitions(-D__cpp_lib_char8_t)
    add_definitions(-D__cpp_lib_clamp)
    add_compile_options(-finput-charset=UTF-8)
    add_compile_options(-Wall -Wextra -pedantic)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (MSVC)
        # add hot reload on visual studio
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /ZI /Ob1")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI /Ob1")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /ignore:4099")
    endif()
endif()

set(GFLAGS_USE_TARGET_NAMESPACE ON)

# BEGIN PACKAGES
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(Freetype REQUIRED)
find_package(spirv_cross_core REQUIRED)
find_package(spirv_cross_glsl REQUIRED)
find_package(OpenMP)
# END PACKAGES

include_directories(
	${CMAKE_SOURCE_DIR}/third-party
    ${CMAKE_SOURCE_DIR}/build/vcpkg_installed/x64-windows/include
)

set(EstEngine
    spirv-cross-core 
    spirv-cross-glsl 
    SDL2::SDL2 
    SDL2_image::SDL2_image 
    Freetype::Freetype 
)

message("Libraries: ${EstEngine}")

add_subdirectory(lib)
if (ENABLE_TEST STREQUAL "1") 
    add_subdirectory(test)
endif()