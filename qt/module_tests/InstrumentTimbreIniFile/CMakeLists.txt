cmake_minimum_required(VERSION 3.14)
project(InstrumentTimbreIniFileTest)

set(CMAKE_CXX_STANDARD 11)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

add_executable(InstrumentTimbreIniFileTest
    main.cpp
    ../../InstrumentTimbreIniFile.cpp
)

target_include_directories(InstrumentTimbreIniFileTest PRIVATE
    ../..
    ../../..
)

target_link_libraries(InstrumentTimbreIniFileTest
    PRIVATE Qt${QT_VERSION_MAJOR}::Core
)
