if (!CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # https://github.com/Kitware/CMake/blob/v4.0.3/Help/dev/experimental.rst#c-import-std-supportclang++
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
    set(CMAKE_CXX_MODULE_STD ON)
endif()

add_executable(example main.cpp)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
if (!CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_EXTENSIONS ON)
endif()

target_link_libraries(example PRIVATE Jizhak)