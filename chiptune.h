#ifndef _CHIPTUNE_H_
#define _CHIPTUNE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

void chiptune_set_midi_message_callback(
		int(*handler_get_midi_message)(uint32_t const index, uint32_t * const p_tick, uint32_t * const p_message) );

void chiptune_initialize(bool const is_stereo, uint32_t const sampling_rate, uint32_t const resolution);
void chiptune_set_next_midi_message_index(uint32_t const next_midi_message_index);

void chiptune_set_tempo(float const tempo);

uint8_t chiptune_fetch_8bit_wave(void);
int16_t chiptune_fetch_16bit_wave(void);

uint32_t chiptune_get_current_tick(void);
bool chiptune_is_tune_ending(void);

#ifdef __cplusplus
}
#endif

#endif /* _CHIPTUNE_H_ */
