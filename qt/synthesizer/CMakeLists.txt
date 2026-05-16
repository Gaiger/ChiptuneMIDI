cmake_minimum_required(VERSION 3.14)
project(ChiptuneMIDISynthesizer)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CHIPTUNE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
set(CHIPTUNE_QT_DIR "${CHIPTUNE_ROOT_DIR}/qt")
set(CHIPTUNE_QT_SYNTHESIZER_DIR "${CHIPTUNE_QT_DIR}/synthesizer")
set(RTMIDI_DIR "${CHIPTUNE_QT_SYNTHESIZER_DIR}/rtmidi")
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CHIPTUNE_QT_DIR} ${CHIPTUNE_QT_SYNTHESIZER_DIR})

#find_package(Qt5 REQUIRED COMPONENTS Widgets Charts Multimedia WinExtras)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Charts Multimedia REQUIRED)

if (QT_VERSION_MAJOR EQUAL 5)
    if(WIN32)
       find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WinExtras REQUIRED)
    endif()
endif()

set(RTMIDI_BUILD_STATIC_LIBS ON CACHE BOOL "Build RtMidi as a static library" FORCE)
set(RTMIDI_BUILD_TESTING OFF CACHE BOOL "Disable RtMidi tests" FORCE)
set(RTMIDI_BUILD_EXAMPLES OFF CACHE BOOL "Disable RtMidi examples" FORCE)
add_subdirectory(${RTMIDI_DIR} RtMidi)

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
    ${CHIPTUNE_QT_DIR}/ChiptuneMidiValues.cpp
    ${CHIPTUNE_QT_DIR}/ChannelListWidget.cpp
    ${CHIPTUNE_QT_DIR}/ChannelNodeWidget.cpp
    ${CHIPTUNE_QT_DIR}/InstrumentTimbreIniFile.cpp
    ${CHIPTUNE_QT_DIR}/MelodicTimbreFrame.cpp
    ${CHIPTUNE_QT_DIR}/TuneManager.cpp
    ${CHIPTUNE_QT_DIR}/AudioPlayerPrivate.cpp
    ${CHIPTUNE_QT_DIR}/AudioPlayer.cpp
    ${CHIPTUNE_QT_DIR}/WaveChartView.cpp

    ${CHIPTUNE_QT_DIR}/ChiptuneMidiValues.h
    ${CHIPTUNE_QT_DIR}/ChannelListWidget.h
    ${CHIPTUNE_QT_DIR}/ChannelNodeWidget.h
    ${CHIPTUNE_QT_DIR}/InstrumentTimbreIniFile.h
    ${CHIPTUNE_QT_DIR}/MelodicTimbreFrame.h
    ${CHIPTUNE_QT_DIR}/TuneManager.h
    ${CHIPTUNE_QT_DIR}/AudioPlayerPrivate.h
    ${CHIPTUNE_QT_DIR}/AudioPlayer.h
    ${CHIPTUNE_QT_DIR}/WaveChartView.h
)

set(APPLICATION_SYNTHESIZER
    ${CHIPTUNE_QT_SYNTHESIZER_DIR}/ChiptuneMidiSynthesizerWidget.cpp
    ${CHIPTUNE_QT_SYNTHESIZER_DIR}/ChiptuneMidiSynthesizerWidget.h
    ${CHIPTUNE_QT_SYNTHESIZER_DIR}/MidiInputManager.cpp
    ${CHIPTUNE_QT_SYNTHESIZER_DIR}/MidiInputManager.h
    ${CHIPTUNE_QT_SYNTHESIZER_DIR}/main.cpp
)

set(MID_SONG
    ${CHIPTUNE_ROOT_DIR}/mid_reader/mid_reader.c
    ${CHIPTUNE_ROOT_DIR}/mid_reader/mid_reader.h
    ${CHIPTUNE_ROOT_DIR}/mid_reader/qt/MidSong.cpp
    ${CHIPTUNE_ROOT_DIR}/mid_reader/qt/MidSong.h
)


set(SYNTHESIZER_FORMS
    ${CHIPTUNE_QT_SYNTHESIZER_DIR}/ChiptuneMidiSynthesizerWidgetForm.ui
)

set(COMMON_FORMS
    ${CHIPTUNE_QT_DIR}/ChannelListWidgetForm.ui
    ${CHIPTUNE_QT_DIR}/ChannelNodeWidgetForm.ui
    ${CHIPTUNE_QT_DIR}/MelodicTimbreFrameForm.ui
)


if(WIN32)
    set(RESOURCES ${CHIPTUNE_QT_SYNTHESIZER_DIR}/ChiptuneMIDISynthesizer.rc)
endif()


source_group("Engine" FILES ${ENGINE})
source_group("Application/Common" FILES ${APPLICATION_COMMON})
source_group("Application/Synthesizer" FILES ${APPLICATION_SYNTHESIZER})
source_group("MidSong" FILES ${MID_SONG})
source_group("Forms/Common" FILES ${COMMON_FORMS})
source_group("Forms/Synthesizer" FILES ${SYNTHESIZER_FORMS})
if(WIN32)
    source_group("Resources" FILES ${RESOURCES})
endif()

add_executable(ChiptuneMIDISynthesizer WIN32
    ${ENGINE} ${APPLICATION_COMMON} ${APPLICATION_SYNTHESIZER} ${MID_SONG} ${SYNTHESIZER_FORMS} ${COMMON_FORMS})

if(WIN32)
    target_sources(ChiptuneMIDISynthesizer PRIVATE ${RESOURCES})
endif()

target_include_directories(ChiptuneMIDISynthesizer PRIVATE
    ${CHIPTUNE_QT_SYNTHESIZER_DIR}
    ${CHIPTUNE_QT_DIR}
    ${CHIPTUNE_ROOT_DIR})

target_link_libraries(ChiptuneMIDISynthesizer
    PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts Qt${QT_VERSION_MAJOR}::Multimedia rtmidi)
if (QT_VERSION_MAJOR EQUAL 5)
    if(WIN32)
        target_link_libraries(ChiptuneMIDISynthesizer PRIVATE Qt5::WinExtras)
    endif()
endif()

add_custom_command(TARGET ChiptuneMIDISynthesizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CHIPTUNE_QT_DIR}/icons
        $<TARGET_FILE_DIR:ChiptuneMIDISynthesizer>/icons
)
