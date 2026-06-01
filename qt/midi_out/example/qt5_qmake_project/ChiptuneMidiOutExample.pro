TEMPLATE = app
TARGET = ChiptuneMidiOutExample

QT += core
QT += multimedia

CONFIG += c++11 console
CONFIG -= app_bundle

CONFIG(debug, release|debug) {
    DEFINES += _DEBUG
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32 {
    QMAKE_CFLAGS   += /wd4819
    QMAKE_CXXFLAGS += /wd4819
}

CHIPTUNE_ROOT_DIR = ../../../..
CHIPTUNE_QT_DIR = $${CHIPTUNE_ROOT_DIR}/qt
CHIPTUNE_QT_MIDI_OUT_DIR = $${CHIPTUNE_QT_DIR}/midi_out
CHIPTUNE_QT_MIDI_OUT_EXAMPLE_DIR = $${CHIPTUNE_QT_MIDI_OUT_DIR}/example

include($${CHIPTUNE_QT_MIDI_OUT_DIR}/qt5_qmake_project/ChiptuneMidiOut.pro)

SOURCES += \
    $${CHIPTUNE_ROOT_DIR}/mid_reader/mid_reader.c \
    $${CHIPTUNE_ROOT_DIR}/mid_reader/qt/MidSong.cpp \
    $${CHIPTUNE_QT_MIDI_OUT_EXAMPLE_DIR}/main.cpp \

HEADERS += \
    $${CHIPTUNE_ROOT_DIR}/mid_reader/mid_reader.h \
    $${CHIPTUNE_ROOT_DIR}/mid_reader/qt/MidSong.h \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
