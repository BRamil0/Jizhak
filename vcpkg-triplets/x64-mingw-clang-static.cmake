set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_ENV_PASSTHROUGH PATH)

set(VCPKG_CMAKE_SYSTEM_NAME MinGW-clang)

set(VCPKG_TARGET_TRIPLET_IS_NATIVE_WINDOWS 1)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "D:/Programming/my/Jizhak/vcpkg-triplets/clang-mingw-toolchain.cmake")

set(VCPKG_CXX_FLAGS "-stdlib=libc++ -pthread")
set(VCPKG_C_FLAGS "-stdlib=libc++ -pthread")
set(VCPKG_LINKER_FLAGS "-stdlib=libc++ -pthread")