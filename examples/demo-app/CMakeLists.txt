cmake_minimum_required(VERSION 3.21)

project(QtAutoTestDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(QtAutoTest REQUIRED CONFIG)

add_executable(QtAutoTestDemo
    demo_visualizer.cpp
    demo_visualizer.h
    main.cpp
    main_window.cpp
    main_window.h
)

target_include_directories(QtAutoTestDemo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(QtAutoTestDemo PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    QtAutoTest::Runtime
)
