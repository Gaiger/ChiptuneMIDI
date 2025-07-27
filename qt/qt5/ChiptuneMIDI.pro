QT += widgets
QT += charts
QT += multimedia
win32 {
QT += winextras
}

CONFIG += c++11 #console
CONFIG -= app_bundle

CONFIG(debug, release|debug) {
    DEFINES += _DEBUG
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CHIPTUNE_ROOT_DIR = ../..
CHIPTUNE_QT_DIR = $${CHIPTUNE_ROOT_DIR}/qt

include($${CHIPTUNE_QT_DIR}/QMidi/src/QMidi.pri)

SOURCES += \
        $${CHIPTUNE_ROOT_DIR}/chiptune.c \
        $${CHIPTUNE_ROOT_DIR}/chiptune_channel_controller_internal.c \
        $${CHIPTUNE_ROOT_DIR}/chiptune_common_internal.c \
        $${CHIPTUNE_ROOT_DIR}/chiptune_midi_control_change_internal.c \
        $${CHIPTUNE_ROOT_DIR}/chiptune_event_internal.c \
        $${CHIPTUNE_ROOT_DIR}/chiptune_midi_effect_internal.c \
        $${CHIPTUNE_ROOT_DIR}/chiptune_oscillator_internal.c \
        $${CHIPTUNE_ROOT_DIR}/chiptune_printf_internal.c \
        $${CHIPTUNE_QT_DIR}/ChannelListWidget.cpp \
        $${CHIPTUNE_QT_DIR}/ChannelNodeWidget.cpp \
        $${CHIPTUNE_QT_DIR}/ChiptuneMidiWidget.cpp \
        $${CHIPTUNE_QT_DIR}/GetInstrumentNameString.cpp \
        $${CHIPTUNE_QT_DIR}/PitchTimbreFrame.cpp \
        $${CHIPTUNE_QT_DIR}/ProgressSlider.cpp \
        $${CHIPTUNE_QT_DIR}/SequencerWidget.cpp \
        $${CHIPTUNE_QT_DIR}/TuneManager.cpp \
        $${CHIPTUNE_QT_DIR}/AudioPlayer.cpp \
        $${CHIPTUNE_QT_DIR}/AudioPlayerPrivateQt5.cpp \
        $${CHIPTUNE_QT_DIR}/WaveChartViewchartview.cpp \
        $${CHIPTUNE_QT_DIR}/main.cpp

HEADERS += \
    $${CHIPTUNE_ROOT_DIR}/chiptune.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_channel_controller_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_common_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_midi_control_change_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_event_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_midi_define_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_midi_effect_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_oscillator_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_printf_internal.h \
    $${CHIPTUNE_QT_DIR}/ChannelListWidget.h \
    $${CHIPTUNE_QT_DIR}/ChannelNodeWidget.h \
    $${CHIPTUNE_QT_DIR}/ChiptuneMidiWidget.h \
    $${CHIPTUNE_QT_DIR}/GetInstrumentNameString.h \
    $${CHIPTUNE_QT_DIR}/MidiPlayer.h \
    $${CHIPTUNE_QT_DIR}/PitchTimbreFrame.h \
    $${CHIPTUNE_QT_DIR}/ProgressSlider.h \
    $${CHIPTUNE_QT_DIR}/SequencerWidget.h \
    $${CHIPTUNE_QT_DIR}/TuneManager.h \
    $${CHIPTUNE_QT_DIR}/AudioPlayer.h \
    $${CHIPTUNE_QT_DIR}/AudioPlayerPrivateQt5.h \
    $${CHIPTUNE_QT_DIR}/WaveChartView.h \

FORMS += \
    $${CHIPTUNE_QT_DIR}/ChannelListWidgetForm.ui \
    $${CHIPTUNE_QT_DIR}/ChannelNodeWidgetForm.ui \
    $${CHIPTUNE_QT_DIR}/PitchTimbreFrameForm.ui \
    $${CHIPTUNE_QT_DIR}/ChiptuneMidiWidgetForm.ui


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32{
    RC_ICONS = $${CHIPTUNE_QT_DIR}/chiptune.ico
    VERSION = 0.0.0.3
    QMAKE_TARGET_PRODUCT = "ChiptuneMIDI"
    QMAKE_TARGET_DESCRIPTION = "ChiptuneMIDI: Convert .mid file into chiptune"
    QMAKE_TARGET_COPYRIGHT = "Copyright 2024 by Chen Gaiger"
}

win32 {
    QMAKE_POST_LINK += xcopy /E /Y /I \"$$CHIPTUNE_QT_DIR\\icons\" \"$$OUT_PWD\\icons\" & echo Copied icons directory
}
unix {
    QMAKE_POST_LINK += cp -r $$CHIPTUNE_QT_DIR/icons $$OUT_PWD/icons
}
