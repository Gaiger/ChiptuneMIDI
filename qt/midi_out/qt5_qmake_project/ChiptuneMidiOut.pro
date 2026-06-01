QT += core
QT += multimedia

INCLUDEPATH += $${CHIPTUNE_ROOT_DIR}
INCLUDEPATH += $${CHIPTUNE_QT_DIR}
INCLUDEPATH += $${CHIPTUNE_QT_MIDI_OUT_DIR}

SOURCES += \
    $${CHIPTUNE_ROOT_DIR}/chiptune.c \
    $${CHIPTUNE_ROOT_DIR}/chiptune_midi_note_internal.c \
    $${CHIPTUNE_ROOT_DIR}/chiptune_midi_effect_internal.c \
    $${CHIPTUNE_ROOT_DIR}/chiptune_midi_control_change_internal.c \
    $${CHIPTUNE_ROOT_DIR}/chiptune_envelope_internal.c \
    $${CHIPTUNE_ROOT_DIR}/chiptune_event_internal.c \
    $${CHIPTUNE_ROOT_DIR}/chiptune_oscillator_internal.c \
    $${CHIPTUNE_ROOT_DIR}/chiptune_channel_controller_internal.c \
    $${CHIPTUNE_ROOT_DIR}/chiptune_printf_internal.c \
    $${CHIPTUNE_ROOT_DIR}/chiptune_common_internal.c \
    $${CHIPTUNE_QT_DIR}/TuneManager.cpp \
    $${CHIPTUNE_QT_DIR}/AudioPlayerPrivate.cpp \
    $${CHIPTUNE_QT_DIR}/AudioPlayer.cpp \
    $${CHIPTUNE_QT_MIDI_OUT_DIR}/ChiptuneMidiOut.cpp \

HEADERS += \
    $${CHIPTUNE_ROOT_DIR}/chiptune.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_midi_note_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_midi_effect_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_midi_control_change_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_envelope_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_event_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_oscillator_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_channel_controller_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_printf_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_common_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_version_internal.h \
    $${CHIPTUNE_ROOT_DIR}/chiptune_midi_define.h \
    $${CHIPTUNE_QT_DIR}/TuneManager.h \
    $${CHIPTUNE_QT_DIR}/AudioPlayerPrivate.h \
    $${CHIPTUNE_QT_DIR}/AudioPlayer.h \
    $${CHIPTUNE_QT_MIDI_OUT_DIR}/ChiptuneMidiOut.h \
