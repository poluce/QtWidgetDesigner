cmake_minimum_required(VERSION 3.21)

project(QtAutoTest LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

include(CMakePackageConfigHelpers)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Test WebSockets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Test WebSockets)

add_library(QtAutoTestRuntime
    sdk/include/qtautotest/action_observer.h
    sdk/include/qtautotest/actions.h
    sdk/include/qtautotest/bridge_client.h
    sdk/include/qtautotest/bridge_stream_client.h
    sdk/include/qtautotest/harness.h
    sdk/include/qtautotest/qtautotest.h
    sdk/include/qtautotest/runtime.h
    sdk/include/qtautotest/selector.h
    sdk/include/qtautotest/snapshot.h
    sdk/include/qtautotest/version.h
    sdk/src/action_observer.cpp
    sdk/src/bridge_client.cpp
    sdk/src/bridge_stream_client.cpp
    sdk/src/harness.cpp
    sdk/src/runtime.cpp
    sdk/src/agent_bridge_server.cpp
    sdk/src/agent_bridge_server.h
    sdk/src/app_log_sink.cpp
    sdk/src/app_log_sink.h
    sdk/src/bridge_operations.cpp
    sdk/src/bridge_operations.h
    sdk/src/ui_event_monitor.cpp
    sdk/src/ui_event_monitor.h
    sdk/src/ui_action_executor.cpp
    sdk/src/ui_action_executor.h
    sdk/src/widget_introspection.cpp
    sdk/src/widget_introspection.h
)

target_include_directories(QtAutoTestRuntime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sdk/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/sdk/src
)

target_link_libraries(QtAutoTestRuntime
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::WebSockets
)

target_compile_features(QtAutoTestRuntime PUBLIC cxx_std_17)

target_compile_options(QtAutoTestRuntime PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

set_target_properties(QtAutoTestRuntime PROPERTIES
    EXPORT_NAME Runtime
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
add_library(QtAutoTest::Runtime ALIAS QtAutoTestRuntime)

add_executable(QtAgentMcpServer
    mcp/qtautotest-mcp/main.cpp
)

target_include_directories(QtAgentMcpServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mcp/qtautotest-mcp
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include
)

target_link_libraries(QtAgentMcpServer PRIVATE
    QtAutoTestRuntime
)

install(TARGETS QtAutoTestRuntime QtAgentMcpServer
    EXPORT QtAutoTestTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

install(DIRECTORY sdk/include/ DESTINATION include)

export(EXPORT QtAutoTestTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/QtAutoTestTargets.cmake
    NAMESPACE QtAutoTest::
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtAutoTestConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/QtAutoTestConfig.cmake
    INSTALL_DESTINATION lib/cmake/QtAutoTest
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/QtAutoTestConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT QtAutoTestTargets
    FILE QtAutoTestTargets.cmake
    NAMESPACE QtAutoTest::
    DESTINATION lib/cmake/QtAutoTest
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/QtAutoTestConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/QtAutoTestConfigVersion.cmake
    DESTINATION lib/cmake/QtAutoTest
)
