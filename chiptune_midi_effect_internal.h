#ifndef _CHIPTUNE_MIDI_EFFECT_H_
#define _CHIPTUNE_MIDI_EFFECT_H_

#include <stdint.h>

void update_effect_tick(void);

int process_reverb_effect(uint32_t const tick, int8_t const event_type,
						  int8_t const voice, int8_t const note, int8_t const velocity,
						  int16_t const native_oscillator_index);

int process_chorus_effect(uint32_t const tick, int8_t const event_type,
						  int8_t const voice, int8_t const note, int8_t const velocity,
						  int16_t const native_oscillator_index);
#endif // _CHIPTUNE_MIDI_EFFECT_H_
