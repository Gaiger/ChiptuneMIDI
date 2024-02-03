#ifndef _CHIPTUNE_EVENT_INTERNAL_H_
#define _CHIPTUNE_EVENT_INTERNAL_H_

#include <stdint.h>

enum EventType
{
	EVENT_ACTIVATE = 1,
	EVENT_FREE,
	EVENT_REST,
	EVENT_TYPE_MAX,
};

void clean_all_events(void);

int put_event(int8_t type, int16_t oscillator_index, uint32_t triggerring_tick);
int process_events(uint32_t const tick);

uint32_t get_next_event_triggering_tick(void);
uint32_t get_upcoming_event_number(void);

#endif // _CHIPTUNE_EVENT_INTERNAL_H_
