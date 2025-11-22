#ifndef _CHIPTUNE_EVENT_INTERNAL_H_
#define _CHIPTUNE_EVENT_INTERNAL_H_

#include <stdint.h>

enum EventType
{
	EventTypeNone = 0,
	EventTypeActivate  = 1,
	EventTypeFree,
	EventTypeRest,
	EventTypeDeactivate,
	EventTypeMax,
};

int put_event(int8_t const type, int16_t const oscillator_index, uint32_t const triggering_tick);
int process_events(uint32_t const tick);

int clear_all_events(void);
int destroy_all_events(void);

uint32_t const get_next_event_triggering_tick(void);
int adjust_event_triggering_tick_by_playing_tempo(uint32_t const tick, float const new_playing_tempo);

#endif // _CHIPTUNE_EVENT_INTERNAL_H_
