#include  <stdio.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"

#include "chiptune_event_internal.h"

#define MAX_EVENT_NUMBER							(4 * MAX_OSCILLATOR_NUMBER)

struct _event
{
	int8_t	type;
	uint8_t : 8;
	int16_t oscillator;
	uint32_t triggerring_tick;
	int16_t next_event;
}s_events[MAX_EVENT_NUMBER];

#define NO_EVENT									(-1)

uint32_t s_upcoming_event_number = 0;
int16_t s_event_head_index = NO_EVENT;

/**********************************************************************************/

void check_waiting_events(uint32_t const tick)
{
	int16_t index = s_event_head_index;
	bool is_error_occur = false;
	uint32_t previous_tick = 0;
	for(uint32_t i = 0; i < s_upcoming_event_number; i++){

		if(UNUSED_EVENT == s_events[index].type){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR:: event element type error\r\n");
			is_error_occur = true;
		}

		if(previous_tick > s_events[index].triggerring_tick){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR:: event is not in time order\r\n");
			is_error_occur = true;
		}

		previous_tick = s_events[index].triggerring_tick;
		index = s_events[index].next_event;
	}

	do {
		if(false == is_error_occur){
			break;
		}

		CHIPTUNE_PRINTF(cDeveloping, "tick = %u\r\n", tick);
		index = s_event_head_index;
		for(uint32_t i = 0; i < s_upcoming_event_number; i++){

			CHIPTUNE_PRINTF(cDeveloping, "type = %d, oscillator = %u, triggerring_tick = %u\r\n",
							s_events[index].type, s_events[index].oscillator, s_events[index].triggerring_tick);
			index = s_events[index].next_event;
		}

		CHIPTUNE_PRINTF(cDeveloping, "-------------------------------------------------\r\n");
		CHIPTUNE_PRINTF(cDeveloping, "-------------------------------------------------\r\n");
	} while(0);
	return ;
}

/**********************************************************************************/

int put_event(int8_t type, int16_t oscillator, uint32_t triggerring_tick)
{
	if(MAX_EVENT_NUMBER == s_upcoming_event_number){
		CHIPTUNE_PRINTF(cDeveloping, "No event are available\r\n");
		return -1;
	}

	do {
		int current_index;
		for(current_index = 0; current_index < MAX_EVENT_NUMBER; current_index++){
			if(UNUSED_EVENT == s_events[current_index].type){
				break;
			}
		}
		s_events[current_index].type = type;
		s_events[current_index].oscillator = oscillator;
		s_events[current_index].triggerring_tick = triggerring_tick;
		s_events[current_index].next_event = NO_EVENT;

		if(0 == s_upcoming_event_number){
			s_event_head_index = current_index;
			break;
		}

		if(MAX_EVENT_NUMBER == current_index){
			CHIPTUNE_PRINTF(cDeveloping, "No available event is found\r\n");
			return -2;
		}

		if(s_events[current_index].triggerring_tick <= s_events[s_event_head_index].triggerring_tick){
			s_events[current_index].next_event = s_event_head_index;
			s_event_head_index = current_index;
			break;
		}

		int previous_index = s_event_head_index;
		uint32_t kk;
		for(kk = 1; kk < s_upcoming_event_number; kk++){
			int next_index = s_events[previous_index].next_event;
			if(s_events[current_index].triggerring_tick <= s_events[next_index].triggerring_tick){
				s_events[previous_index].next_event = current_index;
				s_events[current_index].next_event = next_index;
				break;
			}
			previous_index = next_index;
		}

		if(s_upcoming_event_number == kk){
			s_events[previous_index].next_event = current_index;
		}
	} while(0);
	s_upcoming_event_number += 1;

	check_waiting_events(s_events[s_event_head_index].triggerring_tick);
	return 0;
}

/**********************************************************************************/

void process_events(uint32_t const tick, struct _oscillator * const p_oscillators)
{
	int timely_event_number = 0;

	for(uint32_t i = 0; i < s_upcoming_event_number; i++){
		if(s_events[s_event_head_index].triggerring_tick > tick){
			break;
		}

		struct _oscillator *p_oscillator = &p_oscillators[s_events[s_event_head_index].oscillator];
		char addition_string[16] = "";
		if(IS_CHORUS_OSCILLATOR(p_oscillator->state_bits)){
			snprintf(&addition_string[0], sizeof(addition_string), "chorus");
		}
		switch(s_events[s_event_head_index].type)
		{
		case ACTIVATE_EVENT:
			CHIPTUNE_PRINTF(cOscillatorTransition, "tick = %u, ACTIVATE oscillator = %u, voice = %u, note = %u, volume = %u for %s\r\n",
							tick, s_events[s_event_head_index].oscillator,
							p_oscillator->voice, p_oscillator->note, p_oscillator->volume, &addition_string[0]);
			SET_ACTIVATED_ON(p_oscillator->state_bits);
			break;

		case RELEASE_EVENT:
			CHIPTUNE_PRINTF(cOscillatorTransition, "tick = %u, RELEASE oscillator = %u, voice = %u, note = %u, volume = %u for %s\r\n",
							tick, s_events[s_event_head_index].oscillator,
							p_oscillator->voice, p_oscillator->note, p_oscillator->volume,  &addition_string[0]);
			discard_oscillator(s_events[s_event_head_index].oscillator);
			break;
		default:
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: UNKOWN event type = %d\r\n");
			break;
		}

		s_events[s_event_head_index].type = UNUSED_EVENT;
		s_event_head_index = s_events[s_event_head_index].next_event;
		timely_event_number += 1;
	}

	s_upcoming_event_number -= timely_event_number;
	check_waiting_events(tick);
}

/**********************************************************************************/

void clean_all_events(void)
{
	for(int i = 0; i < MAX_EVENT_NUMBER; i++){
		s_events[i].type = UNUSED_EVENT;
	}
	s_upcoming_event_number = 0;
}

/**********************************************************************************/

uint32_t get_next_event_triggering_tick(void)
{
	if(0 == s_upcoming_event_number){
			return UINT32_MAX;
	}

	return s_events[s_event_head_index].triggerring_tick;
}

/**********************************************************************************/

uint32_t get_upcoming_event_number(void)
{
	return s_upcoming_event_number;
}
