# ProtonG

ProtonG is a high-performance, Vulkan-based media playback engine with Luau scripting capabilities.

## Features

- **Vulkan Rendering**: Low-level, high-performance graphics.
- **Luau Scripting**: Fully integrated scripting for engine logic and custom behaviors.
- **Media Playback**: Optimized for high-quality image sequence and audio synchronization.
- **Cross-Platform**: Built with portability in mind using CMake.
- **Performance Focused**: Includes built-in system monitoring and optimized resource management.

## Repository Layout

- `.github/`: CI/CD workflows.
- `docs/`: In-depth documentation on roadmap, architecture, and scripting.
- `sandbox/`: Example scripts, shaders, and assets for testing.
- `src/`: Core engine source code.
- `third_party/`: External libraries (Luau, stb, miniaudio).

## Getting Started

### Prerequisites

- CMake 3.15 or higher
- Vulkan SDK
- C++20 compliant compiler (MSVC, GCC, Clang)

### Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Documentation

See the `docs/` directory for detailed information:
- [Architecture Overview](docs/ARCHITECTURE.md)
- [Project Roadmap](docs/ROADMAP.md)
- [Scripting Guide](docs/SCRIPTING.md)

## License

This project is licensed under the Apache 2.0 License. See [LICENSE](LICENSE) for details.
