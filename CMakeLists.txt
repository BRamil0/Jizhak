cmake_minimum_required(VERSION 4.2.2)
set(USE_OF_STD_MODULE ON)

if (USE_OF_STD_MODULE)
    # https://github.com/Kitware/CMake/blob/v4.2.2/Help/dev/experimental.rst#c-import-std-support
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444" CACHE STRING "")
    set(CMAKE_CXX_MODULE_STD ON CACHE BOOL "")
endif()

include(FetchContent)

project(Jizhak LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    set(USE_OF_CONAN ON)
else()
    set(USE_OF_CONAN OFF)
endif()

if (USE_OF_CONAN)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmodules -Wall -Wextra -pedantic -Wshadow -Wconversion") # -Werror -stdlib=libc++
        message(STATUS "Clang-CL (in VS) CXX Flags: ${CMAKE_CXX_FLAGS}")

    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /permissive- /Zc:__cplusplus") # /Wall /WX
        message(STATUS "MSVC CXX Flags: ${CMAKE_CXX_FLAGS}")
    endif()

elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmodules -Wall -Wextra -pedantic -Wshadow -Wconversion") # -Werror -stdlib=libc++
    message(STATUS "Clang CXX Flags: ${CMAKE_CXX_FLAGS}")

elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmodules -Wall -Wextra -pedantic -Wshadow -Wconversion") # -fsearch-include-path bits/std.cc
    message(STATUS "GCC CXX Flags: ${CMAKE_CXX_FLAGS}")

endif()

if (WIN32)
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
    message(STATUS "MinGW detected. Enabling static runtime linking.")
endif()

if (NOT USE_OF_CONAN)
    # fmt
    find_package(fmt CONFIG QUIET)
    if (NOT fmt_FOUND)
        message(STATUS "fmt not found, fetching with FetchContent")
        FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        )
        FetchContent_MakeAvailable(fmt)
    endif()

    # Boost.Locale
    find_package(Boost COMPONENTS locale QUIET)
    if (NOT Boost_FOUND)
        message(STATUS "Boost not found, fetching with FetchContent")
        FetchContent_Declare(
            boost
            GIT_REPOSITORY https://github.com/boostorg/boost.git
        )
        FetchContent_MakeAvailable(boost)
    endif()
endif()

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
if (USE_OF_STD_MODULE)
    set(CMAKE_CXX_EXTENSIONS ON)
endif()

add_library(Jizhak)

if (USE_OF_STD_MODULE)
    set_target_properties(Jizhak PROPERTIES CXX_MODULE_STD ON)
    target_compile_definitions(Jizhak PRIVATE USE_OF_STD_MODULE)
endif()

if (USE_OF_CONAN)
    target_link_libraries(Jizhak PUBLIC ${CONAN_LIBS})
else()
    target_link_libraries(Jizhak PUBLIC fmt::fmt)
    target_link_libraries(Jizhak PUBLIC Boost::locale)
endif()

file(GLOB_RECURSE JIZHAK_MODULES "src/jizhak/*.cppm")
file(GLOB_RECURSE JIZHAK_SOURCES "src/jizhak/*.cpp")

target_sources(Jizhak PRIVATE
    ${JIZHAK_SOURCES}
)

target_sources(Jizhak PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/jizhak"
    FILES ${JIZHAK_MODULES}
)

target_compile_features(Jizhak PUBLIC cxx_std_23)

target_include_directories(Jizhak PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

option(JIZHAK_BUILD_TESTS "Build the tests for Jizhak" OFF)
option(JIZHAK_BUILD_EXAMPLE "Build the example for Jizhak" ON)

if (JIZHAK_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if (JIZHAK_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()