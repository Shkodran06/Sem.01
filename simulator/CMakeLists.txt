cmake_minimum_required(VERSION 3.21)
project(SemaforiSimulator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

qt_add_executable(semafori-simulator main.cpp)
target_link_libraries(semafori-simulator PRIVATE Qt6::Widgets)

enable_testing()
qt_add_executable(safetylogic-test test_safetylogic.cpp)
target_link_libraries(safetylogic-test PRIVATE Qt6::Test)
add_test(NAME safetylogic-test COMMAND safetylogic-test)
