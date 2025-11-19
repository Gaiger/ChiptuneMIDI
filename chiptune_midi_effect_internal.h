#ifndef _CHIPTUNE_MIDI_EFFECT_H_
#define _CHIPTUNE_MIDI_EFFECT_H_

#include <stdint.h>
#include "chiptune_common_internal.h"
void update_effect_tick(void);

int process_effects(uint32_t const tick, int8_t const event_type, int8_t const voice, midi_value_t const note,
					normalized_midi_level_t const normalized_velocity, int16_t const native_oscillator_index);
#endif // _CHIPTUNE_MIDI_EFFECT_H_
