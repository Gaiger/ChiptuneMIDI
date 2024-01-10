#ifndef _CHIPTUNE_H_
#define _CHIPTUNE_H_

#include <stdint.h>

#define MIDI_FREQUENCY_TABLE_SIZE					(128)

#ifdef __cplusplus
extern "C"
{
#endif


void chiptune_set_midi_message_callback( int(*handler_get_next_midi_message)(uint32_t * const p_message, uint32_t * const p_tick) );
void chiptune_set_tune_ending_notfication_callback( void(*handler_tune_ending_notification)(void));

void chiptune_initialize(uint32_t const sampling_rate);
void chiptune_set_tempo(float const tempo);
void chiptune_set_resolution(uint32_t const resolution);

uint8_t chiptune_fetch_wave(void);
#ifdef __cplusplus
}
#endif

#endif /* _CHIPTUNE_H_ */
