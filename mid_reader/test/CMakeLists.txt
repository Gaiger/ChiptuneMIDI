cmake_minimum_required(VERSION 3.14)
project(mid_reader_test)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

set(MID_READER_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(CHIPTUNE_ROOT_DIR "${MID_READER_DIR}/..")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/QMidi" QMidi)

add_executable(mid_reader_test
    main.cpp
    "${MID_READER_DIR}/mid_reader.c"
    "${MID_READER_DIR}/qt/MidSong.cpp")

target_include_directories(mid_reader_test PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CHIPTUNE_ROOT_DIR}"
    "${MID_READER_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/QMidi/src")

target_link_libraries(mid_reader_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    QMidi)

target_compile_definitions(mid_reader_test PRIVATE _IDENTICAL_TO_QMIDI)
