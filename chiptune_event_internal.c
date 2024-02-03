#include  <stdio.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_oscillator_internal.h"

#include "chiptune_event_internal.h"

#define MAX_EVENT_NUMBER							(256 * MIDI_MAX_CHANNEL_NUMBER)

enum
{
	UNUSED_EVENT =  -1,
	EVENT_DISCARD = (EVENT_TYPE_MAX + 1),
};

struct _event
{
	int8_t	type;
	uint8_t : 8;
	int16_t oscillator;
	uint32_t triggerring_tick;
	int16_t next_event;
} s_events[MAX_EVENT_NUMBER];

#define NO_EVENT									(-1)

int16_t s_upcoming_event_number = 0;
int16_t s_event_head_index = NO_EVENT;

#ifdef _CHECK_EVENT_LIST
/**********************************************************************************/

static void check_upcoming_events(uint32_t const tick)
{
	int16_t index = s_event_head_index;
	do {
		if(NO_EVENT != s_event_head_index){
			break;
		}
		if(0 != s_upcoming_event_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: tick = %u,"
										 " event head is NO_EVENT but s_upcoming_event_number = %d\r\n",
										s_upcoming_event_number);
			return ;
		}
	} while(0);
	bool is_error_occur = false;
	uint32_t previous_tick = 0;
	for(int16_t i = 0; i < s_upcoming_event_number; i++){

		if(UNUSED_EVENT == s_events[index].type){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR:: event element type error\r\n");
			is_error_occur = true;
		}

		if(previous_tick > s_events[index].triggerring_tick){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR:: event is not in time order\r\n");
			is_error_occur = true;
		}

		if(UNUSED_OSCILLATOR == s_events[index].oscillator){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR:: oscillator is UNUSED_OSCILLATOR\r\n");
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
		for(int16_t i = 0; i < s_upcoming_event_number; i++){

			CHIPTUNE_PRINTF(cDeveloping, "type = %d, oscillator = %d, triggerring_tick = %u\r\n",
							s_events[index].type, s_events[index].oscillator, s_events[index].triggerring_tick);
			index = s_events[index].next_event;
		}

		CHIPTUNE_PRINTF(cDeveloping, "-------------------------------------------------\r\n");
		CHIPTUNE_PRINTF(cDeveloping, "-------------------------------------------------\r\n");
	} while(0);
	return ;
}
#define CHECK_UPCOMING_EVENTS(TICK)					\
													do { \
														check_upcoming_events((TICK)); \
													} while(0)

#else
#define CHECK_UPCOMING_EVENTS(TICK)					\
													do { \
														(void)0; \
													} while(0)
#endif

/**********************************************************************************/

int put_event(int8_t type, int16_t oscillator_index, uint32_t triggerring_tick)
{
	if(MAX_EVENT_NUMBER == s_upcoming_event_number){
		CHIPTUNE_PRINTF(cDeveloping, "No unused event are available\r\n");
		return -1;
	}

	do {
		int16_t current_index;
		for(current_index = 0; current_index < MAX_EVENT_NUMBER; current_index++){
			if(UNUSED_EVENT == s_events[current_index].type){
				break;
			}
		}
		s_events[current_index].type = type;
		s_events[current_index].oscillator = oscillator_index;
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

		if(s_events[current_index].triggerring_tick < s_events[s_event_head_index].triggerring_tick){
			s_events[current_index].next_event = s_event_head_index;
			s_event_head_index = current_index;
			break;
		}

		int16_t previous_index = s_event_head_index;
		int16_t kk;
		for(kk = 1; kk < s_upcoming_event_number; kk++){
			int16_t next_index = s_events[previous_index].next_event;
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

	CHECK_UPCOMING_EVENTS(s_events[s_event_head_index].triggerring_tick);
	return 0;
}

/**********************************************************************************/

void process_events(uint32_t const tick)
{
	while(NO_EVENT != s_event_head_index)
	{
		if(s_events[s_event_head_index].triggerring_tick > tick){
			break;
		}

		oscillator_t *p_oscillator = get_oscillator_pointer_from_index(s_events[s_event_head_index].oscillator);
		char addition_string[16] = "";
		if(true == IS_CHORUS_ASSOCIATE(p_oscillator->state_bits)){
			snprintf(&addition_string[0], sizeof(addition_string), "(chorus)");
		}
		int8_t const event_type = s_events[s_event_head_index].type;
		switch(event_type)
		{
		case EVENT_ACTIVATE:
			CHIPTUNE_PRINTF(cEventTriggering, "tick = %u, ACTIVATE oscillator = %d, voice = %d, note = %d, loudness = 0x%04x %s\r\n",
							tick, s_events[s_event_head_index].oscillator,
							p_oscillator->voice, p_oscillator->note, p_oscillator->loudness, &addition_string[0]);
			if(true == IS_ACTIVATED(p_oscillator->state_bits)){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: activate an activated oscillator = %d\r\n",
								s_events[s_event_head_index].oscillator);
				return ;
			}
			SET_ACTIVATED(p_oscillator->state_bits);
			p_oscillator->envelope_state = ENVELOPE_ATTACK;
			break;

		case EVENT_FREE:
			CHIPTUNE_PRINTF(cEventTriggering, "tick = %u, FREE oscillator = %d, voice = %d, note = %d, loudness = 0x%04x %s\r\n",
							tick, s_events[s_event_head_index].oscillator,
							p_oscillator->voice, p_oscillator->note, p_oscillator->loudness, &addition_string[0]);
			if(true == IS_FREEING(p_oscillator->state_bits)) {
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: free a freeing oscillator = %d\r\n",
							s_events[s_event_head_index].oscillator);
				return ;
			}
			do {
				int16_t release_tick_number =
						get_channel_controller_pointer_from_index(p_oscillator->voice)->envelope_release_tick_number;
				if(false == IS_RESTING(p_oscillator->state_bits)){
					release_tick_number = 0;
				}
				SET_FREEING(p_oscillator->state_bits);
				p_oscillator->envelope_state = ENVELOPE_RELEASE;
				put_event(EVENT_DISCARD, s_events[s_event_head_index].oscillator,
					tick + release_tick_number);
			} while(0);
			break;

		case EVENT_REST:
			CHIPTUNE_PRINTF(cEventTriggering, "tick = %u, REST oscillator = %d, voice = %d, note = %d, loudness = 0x%04x %s\r\n",
							tick, s_events[s_event_head_index].oscillator,
							p_oscillator->voice, p_oscillator->note, p_oscillator->loudness, &addition_string[0]);

			if(true == IS_RESTING(p_oscillator->state_bits)){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: rest a resting oscillator = %d\r\n",
							s_events[s_event_head_index].oscillator);
				return ;
			}
			SET_RESTING(p_oscillator->state_bits);
			p_oscillator->envelope_state = ENVELOPE_RELEASE;
			break;

		case EVENT_DISCARD:
			CHIPTUNE_PRINTF(cEventTriggering, "tick = %u, DISCARD oscillator = %d, voice = %d, note = %d, amplitude = 0x%04x(%3.2f%%) %s\r\n",
							tick, s_events[s_event_head_index].oscillator,
							p_oscillator->voice, p_oscillator->note, p_oscillator->amplitude, p_oscillator->amplitude/(float)p_oscillator->loudness , &addition_string[0]);
			discard_oscillator(s_events[s_event_head_index].oscillator);
			break;

		default:
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: UNKOWN event type = %d\r\n");
			break;
		}
		s_events[s_event_head_index].type = UNUSED_EVENT;
		s_event_head_index = s_events[s_event_head_index].next_event;
		s_upcoming_event_number -= 1;
	}
	CHECK_UPCOMING_EVENTS(tick);
}

/**********************************************************************************/

void clean_all_events(void)
{
	for(int16_t i = 0; i < MAX_EVENT_NUMBER; i++){
		s_events[i].type = UNUSED_EVENT;
	}
	s_upcoming_event_number = 0;
}

/**********************************************************************************/

#ifndef NULL_TICK
	#define NULL_TICK								(UINT32_MAX)
#endif

uint32_t get_next_event_triggering_tick(void)
{
	if(0 == s_upcoming_event_number){
			return NULL_TICK;
	}

	return s_events[s_event_head_index].triggerring_tick;
}

/**********************************************************************************/

uint32_t get_upcoming_event_number(void)
{
	return s_upcoming_event_number;
}
