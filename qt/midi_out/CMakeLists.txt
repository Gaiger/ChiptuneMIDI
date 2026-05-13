cmake_minimum_required(VERSION 3.14)
project(ChiptuneMidiOut)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_AUTOMOC ON)

set(CHIPTUNE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
set(CHIPTUNE_QT_DIR "${CHIPTUNE_ROOT_DIR}/qt")
set(CHIPTUNE_QT_MIDI_OUT_DIR "${CHIPTUNE_QT_DIR}/midi_out")

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Multimedia REQUIRED)

set(ENGINE
    ${CHIPTUNE_ROOT_DIR}/chiptune.c
    ${CHIPTUNE_ROOT_DIR}/chiptune_midi_note_internal.c
    ${CHIPTUNE_ROOT_DIR}/chiptune_midi_effect_internal.c
    ${CHIPTUNE_ROOT_DIR}/chiptune_midi_control_change_internal.c
    ${CHIPTUNE_ROOT_DIR}/chiptune_envelope_internal.c
    ${CHIPTUNE_ROOT_DIR}/chiptune_event_internal.c
    ${CHIPTUNE_ROOT_DIR}/chiptune_oscillator_internal.c
    ${CHIPTUNE_ROOT_DIR}/chiptune_channel_controller_internal.c
    ${CHIPTUNE_ROOT_DIR}/chiptune_printf_internal.c
    ${CHIPTUNE_ROOT_DIR}/chiptune_common_internal.c

    ${CHIPTUNE_ROOT_DIR}/chiptune.h
    ${CHIPTUNE_ROOT_DIR}/chiptune_midi_note_internal.h
    ${CHIPTUNE_ROOT_DIR}/chiptune_midi_effect_internal.h
    ${CHIPTUNE_ROOT_DIR}/chiptune_midi_control_change_internal.h
    ${CHIPTUNE_ROOT_DIR}/chiptune_envelope_internal.h
    ${CHIPTUNE_ROOT_DIR}/chiptune_event_internal.h
    ${CHIPTUNE_ROOT_DIR}/chiptune_oscillator_internal.h
    ${CHIPTUNE_ROOT_DIR}/chiptune_channel_controller_internal.h
    ${CHIPTUNE_ROOT_DIR}/chiptune_printf_internal.h
    ${CHIPTUNE_ROOT_DIR}/chiptune_common_internal.h
    ${CHIPTUNE_ROOT_DIR}/chiptune_midi_define.h
)

set(APPLICATION_COMMON
    ${CHIPTUNE_QT_DIR}/TuneManager.cpp
    ${CHIPTUNE_QT_DIR}/AudioPlayerPrivate.cpp
    ${CHIPTUNE_QT_DIR}/AudioPlayer.cpp

    ${CHIPTUNE_QT_DIR}/TuneManager.h
    ${CHIPTUNE_QT_DIR}/AudioPlayerPrivate.h
    ${CHIPTUNE_QT_DIR}/AudioPlayer.h
)

set(CHIPTUNE_MIDI_OUT
    ${CHIPTUNE_QT_MIDI_OUT_DIR}/ChiptuneMidiOut.cpp
    ${CHIPTUNE_QT_MIDI_OUT_DIR}/ChiptuneMidiOut.h
)

source_group("Engine" FILES ${ENGINE})
source_group("Application/Common" FILES ${APPLICATION_COMMON})
source_group("Application/ChiptuneMidiOut" FILES ${CHIPTUNE_MIDI_OUT})

add_library(ChiptuneMidiOut STATIC
    ${ENGINE} ${APPLICATION_COMMON} ${CHIPTUNE_MIDI_OUT})

target_include_directories(ChiptuneMidiOut PUBLIC
    ${CHIPTUNE_QT_MIDI_OUT_DIR}
    ${CHIPTUNE_QT_DIR}
    ${CHIPTUNE_ROOT_DIR})

target_link_libraries(ChiptuneMidiOut
    PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Multimedia)
