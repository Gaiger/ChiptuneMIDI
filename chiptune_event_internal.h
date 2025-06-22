#ifndef _CHIPTUNE_EVENT_INTERNAL_H_
#define _CHIPTUNE_EVENT_INTERNAL_H_

#include <stdint.h>
#include "chiptune_oscillator_internal.h"

enum EventType
{
	EVENT_ACTIVATE = 1,
	EVENT_FREE,
	EVENT_REST,
	EVENT_DEACTIVATE,
	EVENT_TYPE_MAX,
};


oscillator_t * const acquire_event_freed_oscillator(int16_t * const p_index);

int16_t const get_event_occupied_oscillator_number(void);
int16_t const get_event_occupied_oscillator_head_index();
int16_t const get_event_occupied_oscillator_next_index(int16_t const index);
oscillator_t * const get_event_oscillator_pointer_from_index(int16_t const index);

void reset_all_events(void);
void clean_all_events(void);
int put_event(int8_t const type, int16_t const oscillator_index, uint32_t const triggering_tick);
int process_events(uint32_t const tick);

uint32_t const get_next_event_triggering_tick(void);

int adjust_event_triggering_tick_by_playing_tempo(uint32_t const tick, float const new_playing_tempo);

#endif // _CHIPTUNE_EVENT_INTERNAL_H_
