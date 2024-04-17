QT += widgets
QT += charts
QT += multimedia
win32 {
QT += winextras
}

CONFIG += c++11 #console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(QMidi/src/QMidi.pri)

SOURCES += \
        ../chiptune.c \
        ../chiptune_channel_controller_internal.c \
        ../chiptune_midi_control_change_internal.c \
        ../chiptune_event_internal.c \
        ../chiptune_midi_effect_internal.c \
        ../chiptune_oscillator_internal.c \
        ../chiptune_printf_internal.c \
        ChannelListWidget.cpp \
        ChannelNodeWidget.cpp \
        ChiptuneMidiWidget.cpp \
        GetInstrumentNameString.cpp \
        PitchTimbreFrame.cpp \
        ProgressSlider.cpp \
        SequencerWidget.cpp \
        TuneManager.cpp \
        AudioPlayer.cpp \
        WaveChartViewchartview.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ../chiptune.h \
    ../chiptune_channel_controller_internal.h \
    ../chiptune_common_internal.h \
    ../chiptune_midi_control_change_internal.h \
    ../chiptune_event_internal.h \
    ../chiptune_midi_define_internal.h \
    ../chiptune_midi_effect_internal.h \
    ../chiptune_oscillator_internal.h \
    ../chiptune_printf_internal.h \
    ChannelListWidget.h \
    ChannelNodeWidget.h \
    ChiptuneMidiWidget.h \
    GetInstrumentNameString.h \
    MidiPlayer.h \
    PitchTimbreFrame.h \
    ProgressSlider.h \
    SequencerWidget.h \
    TuneManager.h \
    AudioPlayer.h \
    WaveChartView.h

FORMS += \
    ChannelListWidgetForm.ui \
    ChannelNodeWidgetForm.ui \
    PitchTimbreFrameForm.ui \
    ChiptuneMidiWidgetForm.ui

win32{
    RC_ICONS = chiptune.ico
    VERSION = 0.0.0.2
    QMAKE_TARGET_PRODUCT = "CHiptuneMIDI"
    QMAKE_TARGET_DESCRIPTION = "Covert .Mid file into chiptune"
    QMAKE_TARGET_COPYRIGHT = "Copyright 2024 by Chen Gaiger"
}
