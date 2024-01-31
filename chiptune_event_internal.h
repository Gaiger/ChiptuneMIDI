#ifndef _CHIPTUNE_EVENT_INTERNAL_H_
#define _CHIPTUNE_EVENT_INTERNAL_H_

#include <stdint.h>

enum TImeEventType
{
	UNUSED_EVENT = -1,
	READY_EVENT = 0,

	ACTIVATE_EVENT,
	RELEASE_EVENT,
	DISCARD_EVENT,
};

void clean_all_events(void);

int put_event(int8_t type, int16_t oscillator_index, uint32_t triggerring_tick);
void process_events(uint32_t const tick);

uint32_t get_next_event_triggering_tick(void);
uint32_t get_upcoming_event_number(void);

#endif // _CHIPTUNE_EVENT_INTERNAL_H_
