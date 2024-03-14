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
        ChiptuneMidiWidget.cpp \
        ProgressSlider.cpp \
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
    ChiptuneMidiWidget.h \
    MidiPlayer.h \
    ProgressSlider.h \
    TuneManager.h \
    AudioPlayer.h \
    WaveChartView.h

FORMS += \
    ChiptuneMidiWidgetForm.ui

RC_ICONS = chiptune.ico
