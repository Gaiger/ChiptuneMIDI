#ifndef _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_
#define _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_

#include <stdint.h>
#include "chiptune_common_internal.h"

#define EFFECTIVE_PRESSURE_LEVEL(NORMALIZED_PRESSURE_LEVEL)	\
	DIVIDE_BY_16(((NORMALIZED_PRESSURE_LEVEL) - 1) + 15)

int process_control_change_message(uint32_t const tick, int8_t const voice,
								   midi_value_t const number, midi_value_t const value);

enum
{
	LoundnessChangePressure,
	LoudnessChangeVolume,
	LoudnessChangeExpression,
	LoundessBreathController,
};
void process_loudness_change(uint32_t const tick, int8_t const voice, midi_value_t const value,
									int8_t loudness_change_type);

#endif // _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_
