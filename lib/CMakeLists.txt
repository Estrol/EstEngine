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

set(SHADER_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Shaders")

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
        COMMAND "${glslangValidator}" -gVS
            -V -o ${out_file}
            --vn "__glsl_${out_string}" ${shader}

        RESULT_VARIABLE out 
    )

    if (out EQUAL 1)
        message(FATAL_ERROR "glslangValidator compile fail")
    endif()
endforeach()

add_library(EstEngineLib STATIC ${HEADERS}
    "src/Game.cpp" 
    "src/MsgBox.cpp" 

    # Main Graphics
    "src/Graphics/NativeWindow.cpp" 
    "src/Graphics/Renderer.cpp" 

    # Screens
    "src/Screens/Base.cpp" 
    "src/Screens/ScreenManager.cpp"

    # UI
    "src/UI/UIBase.cpp" 
    "src/UI/Rectangle.cpp" 
    "src/UI/Image.cpp" 
    "src/UI/Text.cpp" 

    # Vulkan backends
    "src/Graphics/Backends/Vulkan/VulkanBackend.cpp" 
    "src/Graphics/Backends/Vulkan/vkinit.cpp" 
    "src/Graphics/Backends/Vulkan/VulkanBootstrap/VkBootstrap.cpp" 
    "src/Graphics/Backends/Vulkan/Volk/volk.cpp" 

    # Vulkan image backends
    "src/Graphics/Backends/Vulkan/VulkanTexture2D.cpp" 

    # OpenGL backends
    "src/Graphics/Backends/OpenGL/OpenGLBackend.cpp"

    # OpenGl image backends
    "src/Graphics/Backends/OpenGL/OpenGlTexture2D.cpp"

    # Audio
    "src/Audio/AudioEngine.cpp" 
    "src/Audio/AudioStream.cpp" 
    "src/Audio/AudioSample.cpp" 

    # Math
    "src/Math/Color3.cpp" 
    "src/Math/UDim.cpp" 
    "src/Math/UDim2.cpp" 
    "src/Math/Vector2.cpp" 
    "src/Math/Vector4.cpp"

    # Inputs
    "src/Inputs/InputManager.cpp" 

    # Threads
    "src/Threads/Thread.cpp" 
    "src/Threads/TimeWatch.cpp"

    # Fonts
    "src/Fonts/FontManager.cpp"
    "src/Fonts/Platform/Win32.cpp"
    "src/Fonts/Platform/Linux.cpp"

    # Misc
    "src/Exceptions/EstException.cpp" 
    "src/Misc/Filesystem.cpp"
    "src/Misc/MD5.cpp"
    "src/Graphics/Utils/stb_image.cpp"
    "src/Graphics/Utils/signalsmith-stretch.cpp"

    # Imgui
    "src/Imgui/imgui.cpp" 
    "src/Imgui/imgui_draw.cpp" 
    "src/Imgui/imgui_tables.cpp" 
    "src/Imgui/imgui_widgets.cpp"
    "src/Imgui/imgui_demo.cpp"
    "src/Graphics/ImguiBackends/imgui_impl_sdl2.cpp"
    "src/Graphics/ImguiBackends/imgui_impl_vulkan.cpp"
    "src/Graphics/ImguiBackends/imgui_impl_opengl3.cpp"
)

include_directories("./include")
target_include_directories(EstEngineLib PUBLIC include)