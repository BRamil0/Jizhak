add_executable(example main_example.cpp)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

target_link_libraries(example PRIVATE Jizhak)