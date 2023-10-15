# EstEngine 2D Game Library
A lightweight 2D game library for C++.

## Features
The library is still in development, so the features are not yet complete.

However the following features are already implemented:

- Audio playback
    - Using [miniaudio](https://github.com/mackron/miniaudio) as backend
    - Support TimeStretching using [SoundTouch](https://gitlab.com/soundtouch/soundtouch)
    - Support Audio stream and sample
- Graphics
    - Vulkan
    - OpenGL (soon:tm:)
- Input
    - Keyboard
    - Mouse

## Building
### Dependencies
- [CMake](https://cmake.org/)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
- [vcpkg](https://github.com/microsoft/vcpkg)

### Building
Make sure you have installed all the dependencies before building the project.

```bash
git clone https://github.com/estrol/estengine.git
cd estengine
cmake -B build -S .
cmake --build build
```

### Development
To start development on the project, you can use the following command to open the project in your preferred IDE:

```bash
cmake --open build
```

## License
This project is licensed under the MIT-0 License. See the [LICENSE](LICENSE) file for details.