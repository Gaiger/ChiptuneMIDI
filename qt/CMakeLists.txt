cmake_minimum_required(VERSION 3.14)
project(ChiptuneMIDI)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CHIPTUNE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(CHIPTUNE_QT_DIR "${CHIPTUNE_ROOT_DIR}/qt")

#find_package(Qt5 REQUIRED COMPONENTS Widgets Charts Multimedia WinExtras)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Charts Multimedia REQUIRED)

if (QT_VERSION_MAJOR EQUAL 5)
    if(WIN32)
       find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WinExtras REQUIRED)
    endif()
endif()

add_subdirectory(${CHIPTUNE_QT_DIR}/QMidi ${CMAKE_CURRENT_BINARY_DIR}/QMidi-build)

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


set(APPILCATION
    ${CHIPTUNE_QT_DIR}/ChannelListWidget.cpp
    ${CHIPTUNE_QT_DIR}/ChannelNodeWidget.cpp
    ${CHIPTUNE_QT_DIR}/ChiptuneMidiWidget.cpp
    ${CHIPTUNE_QT_DIR}/GetInstrumentNameString.cpp
    ${CHIPTUNE_QT_DIR}/PitchTimbreFrame.cpp
    ${CHIPTUNE_QT_DIR}/ProgressSlider.cpp
    ${CHIPTUNE_QT_DIR}/SequencerWidget.cpp
    ${CHIPTUNE_QT_DIR}/TuneManager.cpp
    ${CHIPTUNE_QT_DIR}/AudioPlayerPrivate.cpp
    ${CHIPTUNE_QT_DIR}/AudioPlayer.cpp
    ${CHIPTUNE_QT_DIR}/WaveChartView.cpp

    ${CHIPTUNE_QT_DIR}/ChannelListWidget.h
    ${CHIPTUNE_QT_DIR}/ChannelNodeWidget.h
    ${CHIPTUNE_QT_DIR}/ChiptuneMidiWidget.h
    ${CHIPTUNE_QT_DIR}/GetInstrumentNameString.h
    ${CHIPTUNE_QT_DIR}/MidiPlayer.h
    ${CHIPTUNE_QT_DIR}/PitchTimbreFrame.h
    ${CHIPTUNE_QT_DIR}/ProgressSlider.h
    ${CHIPTUNE_QT_DIR}/SequencerWidget.h
    ${CHIPTUNE_QT_DIR}/TuneManager.h
    ${CHIPTUNE_QT_DIR}/AudioPlayerPrivate.h
    ${CHIPTUNE_QT_DIR}/AudioPlayer.h
    ${CHIPTUNE_QT_DIR}/WaveChartView.h

    ${CHIPTUNE_QT_DIR}/main.cpp
)


set(FORMS
    ${CHIPTUNE_QT_DIR}/ChannelListWidgetForm.ui
    ${CHIPTUNE_QT_DIR}/ChannelNodeWidgetForm.ui
    ${CHIPTUNE_QT_DIR}/PitchTimbreFrameForm.ui
    ${CHIPTUNE_QT_DIR}/ChiptuneMidiWidgetForm.ui
)

if(WIN32)
    set(RESOURCES ${CHIPTUNE_QT_DIR}/ChiptuneMIDI.rc)
endif()


source_group("Engine" FILES ${ENGINE})
source_group("Application" FILES ${APPILCATION})
source_group("Forms" FILES ${FORMS})
if(WIN32)
    source_group("Resources" FILES ${RESOURCES})
endif()

add_executable(ChiptuneMIDI WIN32
    ${ENGINE} ${APPILCATION} ${FORMS})

target_include_directories(ChiptuneMIDI PRIVATE
    ${CHIPTUNE_QT_DIR}/QMidi/src
    ${CHIPTUNE_ROOT_DIR})

target_link_libraries(ChiptuneMIDI
    PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts Qt${QT_VERSION_MAJOR}::Multimedia  QMidi)

if (QT_VERSION_MAJOR EQUAL 5)
    if(WIN32)
        target_link_libraries(ChiptuneMIDI PRIVATE Qt5::WinExtras)
    endif()
endif()

if(WIN32)
    target_sources(ChiptuneMIDI PRIVATE ${RESOURCES})
endif()

add_custom_command(TARGET ChiptuneMIDI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CHIPTUNE_QT_DIR}/icons
        $<TARGET_FILE_DIR:ChiptuneMIDI>/icons
)
