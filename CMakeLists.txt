cmake_minimum_required(VERSION 3.0)
message("Unnamed O2 Game Clone Project - CMake Build System")

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

# if (CMAKE_BUILD_TYPE MATCHES Debug)
#     if (MSVC)
#         set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /ZI /DVK_NO_PROTOTYPES /MD /D_ITERATOR_DEBUG_LEVEL=2 /D_DEBUG")
#         set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /ZI /DVK_NO_PROTOTYPES /MD  /D_ITERATOR_DEBUG_LEVEL=2 /D_DEBUG")
#     else()
#         set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -DVK_NO_PROTOTYPES")
#         set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -DVK_NO_PROTOTYPES")
#     endif()
# elseif (CMAKE_BUILD_TYPE MATCHES Release)
#     if (MSVC)
#         set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /GL /DVK_NO_PROTOTYPES")
#         set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}/O2 /Ob2 /Oi /Ot /GL /DVK_NO_PROTOTYPES")
#         set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")
#     else()
#         set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DVK_NO_PROTOTYPES")
#         set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DVK_NO_PROTOTYPES")
#     endif()
# endif()

set(GFLAGS_USE_TARGET_NAMESPACE ON)

# BEGIN PACKAGES
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
set(SDL2_IMAGE_LIBRARIES $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)

find_package(SoundTouch CONFIG REQUIRED)
set(SoundTouch_LIBRARIES SoundTouch::SoundTouch)
find_path(SoundTouch_INCLUDE_DIR NAMES SoundTouch.h PATH_SUFFIXES soundtouch)
message("SoundTouch include dir: ${SoundTouch_INCLUDE_DIR}")

find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED) 
 
find_package(Vulkan)

if (NOT DEFINED Vulkan_INCLUDE_DIRS)
    if (NOT DEFINED ENV{VULKAN_SDK})
		message(FATAL_ERROR "Cannot find Vulkan and VULKAN_SDK environment variable not defined")
	endif()

	set(Vulkan_INCLUDE_DIRS $ENV{VULKAN_SDK}/include)
	set(Vulkan_LIBRARIES $ENV{VULKAN_SDK}/lib/vulkan-1.lib)
endif()
# END PACKAGES

include_directories(
	third-party/headers 
	${Vulkan_INCLUDE_DIRS} 
	${SDL2_INCLUDE_DIRS}
	${GLM_INCLUDE_DIRS} 
    ${SoundTouch_INCLUDE_DIR}
)

# third-party libraries
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
set(LIB_INDEX 3)
else()
set(LIB_INDEX 1)
endif()

set(EstEngine
    ${SDL2_LIBRARIES} 
    ${SDL2_IMAGE_LIBRARIES} 
    spirv-cross-glsl 
    ${Iconv_LIBRARIES} 
    ${SoundTouch_LIBRARIES}
    ${THIRD_PARTY_LIBS} 
)

message("Libraries: ${EstEngine}")

add_subdirectory(lib)
add_subdirectory(test)