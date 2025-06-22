cmake_minimum_required(VERSION 4.0)
project(Jizhak LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)


# --- Налаштування прапорців компілятора ---

# GCC/Clang:
#  -Wall: Базовий набір корисних попереджень.
#  -Wextra: Додаткові корисні попередження.
#  -pedantic: Попередження про відхилення від стандарту C++.
#  -Werror: Всі попередження стають помилками. Збірка зупиниться.
#  -Wshadow: Попереджати про "затінення" змінних.
#  -Wconversion: Попереджати про потенційно небезпечні неявні перетворення типів.
#  -Wno-<назва-попередження>: Вимкнути конкретне попередження (наприклад, -Wno-unused-variable).
#
# MSVC (Visual Studio Compiler):
#  /W0, /W1, /W2, /W3, /W4: Рівні попереджень (від найменшого до найбільшого). /W3 — типовий, /W4 — рекомендований для високої якості коду.
#  /Wall: Увімкнути всі попередження, включаючи ті, що за замовчуванням вимкнені (може бути дуже "шумно").
#  /WX: Трактувати попередження як помилки.
#  /wd<номер-попередження>: Вимкнути конкретне попередження (наприклад, /wd4100 для невикористаного параметра).

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wshadow -Wconversion")
    message(STATUS "GCC CXX Flags: ${CMAKE_CXX_FLAGS}")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wshadow -Wconversion")
    message(STATUS "Clang CXX Flags: ${CMAKE_CXX_FLAGS}")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /permissive- /Zc:__cplusplus")
    message(STATUS "MSVC CXX Flags: ${CMAKE_CXX_FLAGS}")

endif()

# ======================================================================
# ===               ВИЗНАЧЕННЯ БІБЛІОТЕКИ JIZHAK                    ===
# ======================================================================

# КРОК 1: Створюємо бібліотеку з назвою Jizhak.
add_library(Jizhak)

# КРОК 2: Знаходимо файли згідно з новою архітектурою.
file(GLOB_RECURSE JIZHAK_MODULES "src/jizhak/*.cppm")
file(GLOB_RECURSE JIZHAK_SOURCES "src/jizhak/*.cpp")

# КРОК 3: Підключаємо файли до бібліотеки.
# .cpp файли є приватною реалізацією.
target_sources(Jizhak PRIVATE
    ${JIZHAK_SOURCES}
)

# .cppm файли є публічним інтерфейсом.
# PUBLIC: щоб проєкти, які використовують Jizhak, могли їх імпортувати.
# BASE_DIRS: щоб перетворити шлях "src/jizhak/print.cppm" на логічне ім'я "jizhak.print".
target_sources(Jizhak PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/jizhak"
    FILES ${JIZHAK_MODULES}
)

# КРОК 4: Налаштовуємо публічні властивості бібліотеки.
# Будь-який проєкт, що підключить Jizhak, успадкує ці налаштування.
target_compile_features(Jizhak PUBLIC cxx_std_23)

target_include_directories(Jizhak PUBLIC
    # Для будь-яких публічних .hpp файлів.
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    # Для скомпільованих інтерфейсів модулів (.ifc/.gcm), які CMake розміщує тут.
    "${CMAKE_CURRENT_BINARY_DIR}"
)


# ======================================================================
# ===           НАЛАШТУВАННЯ ЗАЛЕЖНОСТЕЙ (через vcpkg)            ===
# ======================================================================
#
# Щоб це працювало, ви маєте створити файл vcpkg.json у корені
# і налаштувати проєкт командою:
# cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[шлях до vcpkg]/scripts/buildsystems/vcpkg.cmake
#
# Приклад, як додати бібліотеку 'fmt':
# 1. Додайте "fmt" у ваш vcpkg.json
# 2. Розкоментуйте ці рядки:
#
# find_package(fmt CONFIG REQUIRED)
# target_link_libraries(Jizhak PUBLIC fmt::fmt)


# ======================================================================
# ===               ПІДПРОЄКТИ (ТЕСТИ ТА ПРИКЛАДИ)                  ===
# ======================================================================

# Дозволяє вмикати/вимикати збірку тестів та прикладів
option(JIZHAK_BUILD_TESTS "Build the tests for Jizhak" OFF) # За замовчуванням вимкнені
option(JIZHAK_BUILD_EXAMPLE "Build the example for Jizhak" ON) # За замовчуванням увімкнені

# Якщо опція JIZHAK_BUILD_TESTS увімкнена, CMake шукатиме CMakeLists.txt у папці /tests
if(JIZHAK_BUILD_TESTS)
    # Для тестів зазвичай потрібен GoogleTest. vcpkg встановить його,
    # а find_package знайде.
    # find_package(GTest CONFIG REQUIRED)
    add_subdirectory(tests)
endif()

# Якщо опція JIZHAK_BUILD_EXAMPLE увімкнена, CMake шукатиме CMakeLists.txt у папці /example
if(JIZHAK_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()