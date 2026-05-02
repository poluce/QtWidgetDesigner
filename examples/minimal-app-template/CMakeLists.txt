cmake_minimum_required(VERSION 3.21)

project(__PROJECT_NAME__ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(QtAutoTest REQUIRED CONFIG)

add_executable(__PROJECT_NAME__
    main.cpp
)

target_link_libraries(__PROJECT_NAME__ PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    QtAutoTest::Runtime
)
