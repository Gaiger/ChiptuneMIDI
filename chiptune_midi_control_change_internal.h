#ifndef _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_
#define _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_

#include <stdint.h>
#include "chiptune_common_internal.h"

#define NORMALIZE_PRESSURE(PRESSURE_VALUE)					DIVIDE_BY_16((PRESSURE_VALUE) + 15)

int process_control_change_message(uint32_t const tick, int8_t const voice, int8_t const number, int8_t const value);
void set_cc_expression(uint32_t const tick, int8_t const voice, int8_t const value);

enum
{
	LoundnessChangePressure,
	LoudnessChangeVolume,
	LoudnessChangeExpression,
	LoundessBreathController,
};
void process_loudness_change(uint32_t const tick, int8_t const voice, int8_t const value,
									int loudness_change_type);

#endif // _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_
