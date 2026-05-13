cmake_minimum_required(VERSION 3.14)
project(ChiptuneMidiOutExample)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

add_subdirectory(.. ChiptuneMidiOut)

set(CHIPTUNE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

set(MID_SONG
    ${CHIPTUNE_ROOT_DIR}/mid_reader/mid_reader.c
    ${CHIPTUNE_ROOT_DIR}/mid_reader/mid_reader.h
    ${CHIPTUNE_ROOT_DIR}/mid_reader/qt/MidSong.cpp
    ${CHIPTUNE_ROOT_DIR}/mid_reader/qt/MidSong.h
)

source_group("MidSong" FILES ${MID_SONG})

add_executable(ChiptuneMidiOutExample
    main.cpp
    ${MID_SONG})

target_link_libraries(ChiptuneMidiOutExample
    PRIVATE ChiptuneMidiOut Qt${QT_VERSION_MAJOR}::Core)
