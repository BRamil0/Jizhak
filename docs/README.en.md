## Jižak

__[Ukrainian](../README.md)__

#### Description
This is a small C++23 library that provides various tools.

### Features
- Written using cutting-edge C++23 features.
- Full use of modules.
- Full Unicode support.
- Versatility.

### Dependencies
- CMake 4.0.3 or newer.
- A compiler with C++23 support (Clang 20+ or MSVC 14.44+).
- Git.
- Boost and fmt.
- Optional: a package manager (Conan or vcpkg).

### Installation

It is recommended to use automatic installation via the build system (CMake):
```bash
git clone https://github.com/BRamil0/Jizhak.git
cd Jizhak
cmake -B build -S .
cmake --build build
```

For manual installation, see the [manual_installation](manual_installation.en.md) file (temporarily in progress).

### Usage
Simply import the `jizhak` module.

```c++
import jizhak;
```

### Usage Examples
See the [example.cpp](example/main_example.cpp) file.

### License
This project is licensed under the **MIT** License. For more details, see the [LICENSE](../LICENSE.txt) file.