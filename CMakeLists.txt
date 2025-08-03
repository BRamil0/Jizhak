cmake_minimum_required(VERSION 4.0.3)

if (!CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # https://github.com/Kitware/CMake/blob/v4.0.3/Help/dev/experimental.rst#c-import-std-supportclang++
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
    set(CMAKE_CXX_MODULE_STD ON)
endif()

project(Jizhak LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(NOT_USE_NATIVE_STD OFF)

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
#    set(GCC_STD_MODULE_PATH "E:/msys64/mingw64/include/c++/15.1.0/bits/std.cc")
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

# Залежності через vcpkg
# cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS locale)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
if (!CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_EXTENSIONS ON)
endif()

add_library(Jizhak)

if (NOT_USE_NATIVE_STD)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        # --- Крок 1: Знаходимо системний std.cc ---
        find_file(STD_MODULE_SYSTEM_PATH NAMES std.cc HINTS ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
                  PATH_SUFFIXES bits DOC "Шлях до системного файлу std.cc" REQUIRED)
        message(STATUS "Знайдено системний std.cc за шляхом: ${STD_MODULE_SYSTEM_PATH}")

        # --- Крок 2: Копіюємо std.cc в директорію збірки ---
        set(STD_MODULE_LOCAL_PATH ${CMAKE_CURRENT_BINARY_DIR}/std.cc)
        configure_file(${STD_MODULE_SYSTEM_PATH} ${STD_MODULE_LOCAL_PATH} COPYONLY)

        # --- Крок 3: Створюємо повноцінну бібліотеку-модуль для std ---
        # Важливо: ми створюємо бібліотеку, але не передаємо їй джерела одразу.
        add_library(std_module)

        # --- Крок 4: Оголошуємо наш скопійований std.cc як ІНТЕРФЕЙС МОДУЛЯ ---
        # ЦЕЙ БЛОК ВИРІШУЄ ПОМИЛКУ "not found in a FILE_SET".
        target_sources(std_module PUBLIC
            FILE_SET CXX_MODULES
            BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}
            FILES ${STD_MODULE_LOCAL_PATH}
        )

        # --- Крок 5: Пов'язуємо нашу головну бібліотеку з модулем std ---
        target_link_libraries(Jizhak PRIVATE std_module)
    endif()
endif()

target_link_libraries(Jizhak PUBLIC fmt::fmt)
target_link_libraries(Jizhak PUBLIC Boost::locale)

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

# Локальні залежності (тести та приклад)

option(JIZHAK_BUILD_TESTS "Build the tests for Jizhak" OFF)
option(JIZHAK_BUILD_EXAMPLE "Build the example for Jizhak" ON)

if (JIZHAK_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if (JIZHAK_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()