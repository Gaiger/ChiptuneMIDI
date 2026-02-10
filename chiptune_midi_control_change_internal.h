#ifndef _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_
#define _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_

#include <stdint.h>
#include "chiptune_common_internal.h"

uint16_t compute_loudness(normalized_midi_level_t const velocity,
						  normalized_midi_level_t const volume, normalized_midi_level_t const pressure,
						  normalized_midi_level_t const expression, normalized_midi_level_t const breath);

int process_control_change_message(uint32_t const tick, int8_t const voice,
								   midi_value_t const number, midi_value_t const value);
int process_channel_pressure_message(uint32_t const tick, int8_t const voice, midi_value_t const value);

#endif // _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_
