cmake_minimum_required(VERSION 3.0.0)
project(EstEngineLib)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE HEADERS "./include/*.h" "./include/*.hpp")

if (WIN32)
	set(glslangValidator "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
elseif (UNIX AND NOT APPLE)
	set(glslangValidator "$ENV{VULKAN_SDK}/Bin/glslangValidator")
else()
	message(FATAL_ERROR "Unsupported platform")
endif()

set(SHADER_LOCATION "${CMAKE_SOURCE_DIR}/lib/src/Graphics/Shaders")

set(SHADERS
    "${SHADER_LOCATION}/image.frag" 
    "${SHADER_LOCATION}/solid.frag" 
    "${SHADER_LOCATION}/position.vert" 
)

foreach(shader IN LISTS SHADERS)
    set(out_string ${shader})
    string(REPLACE "${SHADER_LOCATION}/" "" out_string ${out_string})
    string(REPLACE ".frag" "" out_string ${out_string})
    string(REPLACE ".vert" "" out_string ${out_string})

    set(out_file "${SHADER_LOCATION}/${out_string}.spv.h")
    message("[SHADER] Compiling shaders ${shader} -> ${out_file}")

    execute_process(
        COMMAND "${glslangValidator}" 
            -V -o ${out_file}
            --vn "__glsl_${out_string}" ${shader}

        RESULT_VARIABLE out 
        OUTPUT_QUIET 
    )

    if (out EQUAL 1)
        message(FATAL_ERROR "glslangValidator compile fail")
    endif()
endforeach()

include_directories("./include")

add_library(EstEngineLib STATIC ${HEADERS}
    "src/Game.cpp" 
    "src/MsgBox.cpp" 

    # Main Graphics
    "src/Graphics/NativeWindow.cpp" 
    "src/Graphics/Renderer.cpp" 

    # UI
    "src/UI/UIBase.cpp" 
    "src/UI/Rectangle.cpp" 

    # Vulkan backends
    "src/Graphics/Backends/Vulkan/VulkanBackend.cpp" 
    "src/Graphics/Backends/Vulkan/vkinit.cpp" 
    "src/Graphics/Backends/Vulkan/VulkanBootstrap/VkBootstrap.cpp" 
    "src/Graphics/Backends/Vulkan/Volk/volk.cpp" 

    # OpenGL backends
    # TODO

    # Audio
    "src/Audio/AudioEngine.cpp" 
    "src/Audio/AudioStream.cpp" 
    "src/Audio/AudioSample.cpp" 

    # Math
    "src/Math/Color3.cpp" 
    "src/Math/UDim.cpp" 
    "src/Math/UDim2.cpp" 
    "src/Math/Vector2.cpp" 

    # Inputs
    "src/Inputs/Manager.cpp" 

    # Misc
    "src/Exceptions/EstException.cpp" 
)
