#ifndef _CHIPTUNE_COMMON_INTERNAL_H_
#define _CHIPTUNE_COMMON_INTERNAL_H_

#include <stdint.h>

#include "chiptune_midi_define_internal.h"

//#define _INCREMENTAL_SAMPLE_INDEX
//#define _AMPLITUDE_NORMALIZATION_BY_RIGHT_SHIFT
//#define _KEEP_SET_BUT_EMPTY_NOTE_CHANNELS

#define _PRINT_DEVELOPING
//#define _PRINT_MIDI_NOTE
//#define _PRINT_MIDI_CONTROLCHANGE
//#define _PRINT_MIDI_PROGRAMCHANGE
//#define _PRINT_MIDI_CHANNELPRESSURE
//#define _PRINT_MIDI_PITCH_WHEEL
//#define _PRINT_EVENT_TRIGGERING

#define _CHECK_OCCUPIED_OSCILLATOR_LIST
#define _CHECK_EVENT_LIST


#define NULL_TICK									(UINT32_MAX)

uint32_t const get_sampling_rate(void);
uint32_t const get_resolution(void);
float const get_tempo_mutliply_playing_speed_ratio(void);

enum
{
	LoundnessChangePressure,
	LoudnessChangeVolume,
	LoudnessChangeExpression,
	LoundessBreathController,
};

void process_loudness_change(uint32_t const tick, int8_t const voice, int8_t const value,
									int loudness_change_type);
#endif // _CHIPTUNE_COMMON_INTERNAL_H_
