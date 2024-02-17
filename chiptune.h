#ifndef _CHIPTUNE_H_
#define _CHIPTUNE_H_

#include <stdint.h>
#include <stdbool.h>

#define MIDI_FREQUENCY_TABLE_SIZE					(128)

#ifdef __cplusplus
extern "C"
{
#endif


void chiptune_set_midi_message_callback(
		int(*handler_get_midi_message)(uint32_t index, uint32_t * const p_tick, uint32_t * const p_message) );
void chiptune_initialize(bool is_stereo,
						 uint32_t const sampling_rate, uint32_t const resolution, uint32_t const total_message_number);
void chiptune_set_tempo(uint32_t const tick, float const tempo);
uint8_t chiptune_fetch_8bit_wave(void);
int16_t chiptune_fetch_16bit_wave(void);
bool chiptune_is_tune_ending(void)
;
#ifdef __cplusplus
}
#endif

#endif /* _CHIPTUNE_H_ */
