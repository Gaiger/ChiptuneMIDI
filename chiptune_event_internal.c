#include <stdio.h>
#include  <string.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_oscillator_internal.h"

#include "chiptune_event_internal.h"


#define MAX_OSCILLATOR_NUMBER						(MIDI_MAX_CHANNEL_NUMBER * 24)

static oscillator_t s_oscillators[MAX_OSCILLATOR_NUMBER];
static int16_t s_occupied_oscillator_number = 0;

struct _occupied_oscillator_node
{
	int16_t previous;
	int16_t next;
}s_occupied_oscillator_nodes[MAX_OSCILLATOR_NUMBER];

int16_t s_occupied_oscillator_head_index = UNOCCUPIED_OSCILLATOR;
int16_t s_occupied_oscillator_last_index = UNOCCUPIED_OSCILLATOR;

#ifdef _CHECK_OCCUPIED_OSCILLATOR_LIST
/**********************************************************************************/

static int check_occupied_oscillator_list(void)
{
	int ret = 0;
	do {
		if(0 > s_occupied_oscillator_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_number = %d\r\n", s_occupied_oscillator_number);
			ret = -1;
			break;
		}

		if(0 == s_occupied_oscillator_number){
			if(UNOCCUPIED_OSCILLATOR != s_occupied_oscillator_head_index){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_number = 0"
											 " but s_occupied_oscillator_head_index is not UNOCCUPIED_OSCILLATOR\r\n");
				ret = -2;
			}
			break;
		}

		int16_t current_index;
		int16_t counter;

		current_index = s_occupied_oscillator_head_index;
		counter= 1;
		while(UNOCCUPIED_OSCILLATOR != s_occupied_oscillator_nodes[current_index].next)
		{
			counter += 1;
			current_index = s_occupied_oscillator_nodes[current_index].next;
		}
		if(counter != s_occupied_oscillator_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: FORWARDING occupied_oscillators list length = %d"
							", not matches s_occupied_oscillator_number = %d\r\n", counter, s_occupied_oscillator_number);
			ret = -3;
			break;
		}

		current_index = s_occupied_oscillator_last_index;
		counter = 1;
		while(UNOCCUPIED_OSCILLATOR != s_occupied_oscillator_nodes[current_index].previous)
		{
			counter += 1;
			current_index = s_occupied_oscillator_nodes[current_index].previous;
		}

		if(counter != s_occupied_oscillator_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: BACKWARDING occupied_oscillators list length = %d"
							", not matches s_occupied_oscillator_number = %d\r\n", counter, s_occupied_oscillator_number);

			ret = -4;
			break;
		}
	} while(0);
	return ret;
}

#define CHECK_OCCUPIED_OSCILLATOR_LIST()			\
													do { \
														check_occupied_oscillator_list(); \
													} while(0)
#else
#define CHECK_OCCUPIED_OSCILLATOR_LIST()			\
													do { \
														(void)0; \
													} while(0)
#endif

/**********************************************************************************/

static int occupy_oscillator(int16_t const index)
{
	do{
		if(0 == s_occupied_oscillator_number){
			s_occupied_oscillator_nodes[index].previous = UNOCCUPIED_OSCILLATOR;
			s_occupied_oscillator_nodes[index].next = UNOCCUPIED_OSCILLATOR;
			s_occupied_oscillator_head_index = index;
			break;
		}
		s_occupied_oscillator_nodes[s_occupied_oscillator_last_index].next = index;
		s_occupied_oscillator_nodes[index].previous = s_occupied_oscillator_last_index;
		s_occupied_oscillator_nodes[index].next = UNOCCUPIED_OSCILLATOR;
	} while(0);
	s_occupied_oscillator_last_index = index;
	s_occupied_oscillator_number += 1;
	CHECK_OCCUPIED_OSCILLATOR_LIST();
	return 0;
}

/**********************************************************************************/

oscillator_t * const acquire_event_freed_oscillator(int16_t * const p_index)
{
	if(MAX_OSCILLATOR_NUMBER == s_occupied_oscillator_number){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR::all oscillators are used\r\n");
		return NULL;
	}
	for(int16_t i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		if(UNOCCUPIED_OSCILLATOR == s_oscillators[i].voice){
			*p_index = i;
			occupy_oscillator(i);
			return &s_oscillators[i];
		}
	}
	CHIPTUNE_PRINTF(cDeveloping, "ERROR::available oscillator is not found\r\n");
	*p_index = UNOCCUPIED_OSCILLATOR;
	return NULL;
}

/**********************************************************************************/

static int discard_oscillator(int16_t const index)
{
	int16_t previous_index = s_occupied_oscillator_nodes[index].previous;
	int16_t next_index = s_occupied_oscillator_nodes[index].next;

	do {
		if(0 == s_occupied_oscillator_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: all oscillators have been discarded\r\n");
			return -1;
		}

		if(1 != s_occupied_oscillator_number
				&& (UNOCCUPIED_OSCILLATOR == previous_index && UNOCCUPIED_OSCILLATOR == next_index) ){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: oscillator %d is not on the occupied list\r\n", index);
			return -2;
		}
	} while(0);

	do {
		if(index == s_occupied_oscillator_head_index){
			s_occupied_oscillator_head_index = next_index;
			s_occupied_oscillator_nodes[s_occupied_oscillator_head_index].previous = UNOCCUPIED_OSCILLATOR;
			break;
		}

		if(index == s_occupied_oscillator_last_index){
			s_occupied_oscillator_last_index = previous_index;
			s_occupied_oscillator_nodes[s_occupied_oscillator_last_index].next = UNOCCUPIED_OSCILLATOR;
			break;
		}

		s_occupied_oscillator_nodes[previous_index].next = next_index;
		s_occupied_oscillator_nodes[next_index].previous = previous_index;
	} while (0);

	s_occupied_oscillator_nodes[index].previous = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_nodes[index].next = UNOCCUPIED_OSCILLATOR;
	s_oscillators[index].voice = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_number -= 1;
	CHECK_OCCUPIED_OSCILLATOR_LIST();
	return 0;
}

/**********************************************************************************/

int16_t const get_event_occupied_oscillator_number(void)
{
	return s_occupied_oscillator_number;
}

/**********************************************************************************/

int16_t const get_event_occupied_oscillator_head_index()
{
	if(-1 == s_occupied_oscillator_head_index
			&& 0 != s_occupied_oscillator_number){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_head_index = -1:: but s_occupied_oscillator_number = %d\r\n",
						s_occupied_oscillator_number);
	}
	return s_occupied_oscillator_head_index;
}

/**********************************************************************************/

int16_t const get_event_occupied_oscillator_next_index(int16_t const index)
{
	if(false == (index >= 0 && index < MAX_OSCILLATOR_NUMBER) ){
		CHIPTUNE_PRINTF(cDeveloping, "oscillator index = %d, out of range \r\n", index);
		return UNOCCUPIED_OSCILLATOR;
	}

	return 	s_occupied_oscillator_nodes[index].next;
}

/**********************************************************************************/

oscillator_t * const get_event_oscillator_pointer_from_index(int16_t const index)
{
	if(false == (index >= 0 && index < MAX_OSCILLATOR_NUMBER) ){
		CHIPTUNE_PRINTF(cDeveloping, "oscillator index = %d, out of range \r\n", index);
		return NULL;
	}

	return &s_oscillators[index];
}

/**********************************************************************************/

static void reset_all_event_oscillators(void)
{
	for(int16_t i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		s_oscillators[i].voice = UNOCCUPIED_OSCILLATOR;
		s_occupied_oscillator_nodes[i]
				= (struct _occupied_oscillator_node){.previous = UNOCCUPIED_OSCILLATOR, .next = UNOCCUPIED_OSCILLATOR};
	}
	s_occupied_oscillator_head_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_last_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_number = 0;
}

/**********************************************************************************/
/**********************************************************************************/

#define MAX_EVENT_NUMBER							(MAX_OSCILLATOR_NUMBER * 3 / 2)

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
	uint32_t triggering_tick;
	int16_t next_event;
} s_events[MAX_EVENT_NUMBER];

#define NO_EVENT									(-1)

int16_t s_upcoming_event_number = 0;
int16_t s_event_head_index = NO_EVENT;

#ifdef _CHECK_EVENT_LIST
/**********************************************************************************/

static int check_upcoming_events(uint32_t const tick)
{
	int ret = 0;
	int16_t index = s_event_head_index;
	do {
		if(NO_EVENT != s_event_head_index){
			break;
		}
		if(0 != s_upcoming_event_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: tick = %u,"
										 " event head is NO_EVENT but s_upcoming_event_number = %d\r\n",
										s_upcoming_event_number);
			ret = -1;
			break;
		}
	} while(0);

	bool is_listing_error_occur = false;
	do
	{
		if(-1 == ret){
			break;
		}
		uint32_t previous_tick = 0;
		for(int16_t i = 0; i < s_upcoming_event_number; i++){
			if(UNUSED_EVENT == s_events[index].type){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR:: event %d is UNUSED_EVENT but on the list\r\n", index);
				is_listing_error_occur = true;
			}
			if(previous_tick > s_events[index].triggering_tick){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR:: event is not in time order\r\n");
				is_listing_error_occur = true;
			}
			if(UNOCCUPIED_OSCILLATOR == s_events[index].oscillator){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR:: event %d oscillator is UNOCCUPIED_OSCILLATOR\r\n", index);
				is_listing_error_occur = true;
			}
			if(UNOCCUPIED_OSCILLATOR == get_event_oscillator_pointer_from_index(s_events[index].oscillator)->voice){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR:: oscillator %d is UNOCCUPIED_OSCILLATOR\r\n", s_events[index].oscillator);
				is_listing_error_occur = true;
			}

			if(true == is_listing_error_occur){
				break;
			}
			previous_tick = s_events[index].triggering_tick;
			index = s_events[index].next_event;
		}
	} while(0);

	do {
		if(false == is_listing_error_occur){
			break;
		}
		CHIPTUNE_PRINTF(cDeveloping, "tick = %u\r\n", tick);
		index = s_event_head_index;
		for(int16_t i = 0; i < s_upcoming_event_number; i++){
			CHIPTUNE_PRINTF(cDeveloping, "event = %d, type = %d, oscillator = %d, triggering_tick = %u\r\n",
							index, s_events[index].type, s_events[index].oscillator, s_events[index].triggering_tick);
			index = s_events[index].next_event;
		}
		CHIPTUNE_PRINTF(cDeveloping, "-------------------------------------------------\r\n");
		CHIPTUNE_PRINTF(cDeveloping, "-------------------------------------------------\r\n");
		ret = -2;
	} while(0);

	return ret;
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

int put_event(int8_t const type, int16_t const oscillator_index, uint32_t const triggering_tick)
{
	if(MAX_EVENT_NUMBER == s_upcoming_event_number){
		CHIPTUNE_PRINTF(cDeveloping, "No unused event is available\r\n");
		return -1;
	}

	if(UNOCCUPIED_OSCILLATOR == oscillator_index){
		return 1;
	}

	do {
		int16_t current_index;
		for(current_index = 0; current_index < MAX_EVENT_NUMBER; current_index++){
			if(UNUSED_EVENT == s_events[current_index].type){
				break;
			}
		}
		if(MAX_EVENT_NUMBER == current_index){
			CHIPTUNE_PRINTF(cDeveloping, "No available event is found\r\n");
			return -2;
		}

		s_events[current_index].type = type;
		s_events[current_index].oscillator = oscillator_index;
		s_events[current_index].triggering_tick = triggering_tick;
		s_events[current_index].next_event = NO_EVENT;

		if(0 == s_upcoming_event_number){
			s_event_head_index = current_index;
			break;
		}

		if(s_events[current_index].triggering_tick < s_events[s_event_head_index].triggering_tick){
			s_events[current_index].next_event = s_event_head_index;
			s_event_head_index = current_index;
			break;
		}

		int16_t previous_index = s_event_head_index;
		int16_t kk;
		for(kk = 1; kk < s_upcoming_event_number; kk++){
			int16_t next_index = s_events[previous_index].next_event;
			if(s_events[current_index].triggering_tick <= s_events[next_index].triggering_tick){
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

	CHECK_UPCOMING_EVENTS(s_events[s_event_head_index].triggering_tick);
	return 0;
}

/**********************************************************************************/
static char s_event_additional_string[32];

static inline char const * const event_additional_string(int16_t const event_index)
{
	oscillator_t const * const p_oscillator = get_event_oscillator_pointer_from_index(s_events[event_index].oscillator);
	channel_controller_t const * const p_channel_controller =
			get_channel_controller_pointer_from_index(p_oscillator->voice);
	snprintf(&s_event_additional_string[0], sizeof(s_event_additional_string), "");
	bool is_empty_string = false;
	do {
		if(true == IS_REVERB_ASSOCIATE(p_oscillator->state_bits)){
			break;
		}
		if(true == IS_CHORUS_ASSOCIATE(p_oscillator->state_bits)){
			break;
		}
		if(true == p_channel_controller->is_damper_pedal_on
				&& false == IS_NOTE_ON(p_oscillator->state_bits)){
			break;
		}
		is_empty_string = true;
	} while(0);

	do {
		if(true == is_empty_string){
			break;
		}

		snprintf(&s_event_additional_string[0], sizeof(s_event_additional_string), "(");
		bool is_empty_content = true;

		if(true == IS_REVERB_ASSOCIATE(p_oscillator->state_bits)){
			size_t event_addition_string_length = strlen(&s_event_additional_string[0]);
			snprintf(&s_event_additional_string[event_addition_string_length], sizeof(s_event_additional_string)
					 - event_addition_string_length, "reverb");
			is_empty_content = false;
		}

		if(true == IS_CHORUS_ASSOCIATE(p_oscillator->state_bits)){
			if(false == is_empty_content){
				size_t event_addition_string_length = strlen(&s_event_additional_string[0]);
				snprintf(&s_event_additional_string[event_addition_string_length],
						 sizeof(s_event_additional_string) - event_addition_string_length, "|");
			}
			size_t event_addition_string_length = strlen(&s_event_additional_string[0]);
			snprintf(&s_event_additional_string[event_addition_string_length], sizeof(s_event_additional_string)
					 - event_addition_string_length, "chorus");
			is_empty_content = false;
		}

		if(true == p_channel_controller->is_damper_pedal_on
				&& false == IS_NOTE_ON(p_oscillator->state_bits)){
			if(false == is_empty_content){
				size_t event_addition_string_length = strlen(&s_event_additional_string[0]);
				snprintf(&s_event_additional_string[event_addition_string_length],
						 sizeof(s_event_additional_string) - event_addition_string_length, "|");
			}
			size_t event_addition_string_length = strlen(&s_event_additional_string[0]);
			snprintf(&s_event_additional_string[event_addition_string_length],
					 sizeof(s_event_additional_string) - event_addition_string_length,"damper_on_note_off");
		}

		{
			size_t event_addition_string_length = strlen(&s_event_additional_string[0]);
			snprintf(&s_event_additional_string[event_addition_string_length],
				 sizeof(s_event_additional_string) - event_addition_string_length,")");
		}
	} while(0);
	return &s_event_additional_string[0];
}

/**********************************************************************************/

int process_events(uint32_t const tick)
{
	while(NO_EVENT != s_event_head_index)
	{
		if(s_events[s_event_head_index].triggering_tick > tick){
			break;
		}

		oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(s_events[s_event_head_index].oscillator);
		channel_controller_t const * const p_channel_controller =
				get_channel_controller_pointer_from_index(p_oscillator->voice);

		int8_t const event_type = s_events[s_event_head_index].type;
		switch(event_type)
		{
		case EVENT_ACTIVATE:
			CHIPTUNE_PRINTF(cEventTriggering,
							"tick = %u, ACTIVATE oscillator = %d, voice = %d, note = %d, loudness = 0x%04x %s\r\n",
							tick, s_events[s_event_head_index].oscillator,
							p_oscillator->voice, p_oscillator->note, p_oscillator->loudness,
							event_additional_string(s_event_head_index));
			if(true == IS_ACTIVATED(p_oscillator->state_bits)){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: activate an activated oscillator = %d\r\n",
								s_events[s_event_head_index].oscillator);
				return -1;
			}
			SET_ACTIVATED(p_oscillator->state_bits);
			p_oscillator->envelope_state = ENVELOPE_ATTACK;
			break;

		case EVENT_FREE:
			CHIPTUNE_PRINTF(cEventTriggering,
							"tick = %u, FREE oscillator = %d, voice = %d, note = %d, amplitude = %2.1f%% of loudness %s\r\n",
							tick, s_events[s_event_head_index].oscillator,
							p_oscillator->voice, p_oscillator->note,
							100.0f * p_oscillator->release_reference_amplitude/(float)p_oscillator->loudness,
							event_additional_string(s_event_head_index));
			if(true == IS_FREEING(p_oscillator->state_bits)) {
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: free a freeing oscillator = %d\r\n",
							s_events[s_event_head_index].oscillator);
				return -1;
			}
			SET_FREEING(p_oscillator->state_bits);
			do
			{
#if(0)
				if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == p_oscillator->voice){
					put_event(EVENT_DISCARD, s_events[s_event_head_index].oscillator, tick);
					break;
				}
#endif
				/*It does not a matter there is a postponement to discard the resting oscillator*/
				p_oscillator->envelope_state = ENVELOPE_RELEASE;
				put_event(EVENT_DISCARD, s_events[s_event_head_index].oscillator,
				tick + p_channel_controller->envelope_release_tick_number);
			} while(0);
			break;

		case EVENT_REST:
			CHIPTUNE_PRINTF(cEventTriggering,
							"tick = %u, REST oscillator = %d, voice = %d, note = %d, amplitude = %2.1f%% of loudness %s\r\n",
							tick, s_events[s_event_head_index].oscillator,
							p_oscillator->voice, p_oscillator->note,
							100.0f * p_oscillator->release_reference_amplitude/(float)p_oscillator->loudness,
							event_additional_string(s_event_head_index));
			if(true == IS_FREEING(p_oscillator->state_bits)
					&& true == IS_NATIVE_OSCILLATOR(p_oscillator->state_bits)){
				CHIPTUNE_PRINTF(cDeveloping, "WARNING :: rest a freeing native oscillator = %d\r\n",
							s_events[s_event_head_index].oscillator);
				break;
			}
			if(true == IS_RESTING(p_oscillator->state_bits)
					&& true == IS_NATIVE_OSCILLATOR(p_oscillator->state_bits)){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: rest a resting native oscillator = %d\r\n",
							s_events[s_event_head_index].oscillator);
				break;
			}
			SET_RESTING(p_oscillator->state_bits);
			p_oscillator->envelope_state = ENVELOPE_RELEASE;
			break;
		case EVENT_DEACTIVATE:
			CHIPTUNE_PRINTF(cEventTriggering,
							"tick = %u, DEACTIVATE oscillator = %d, voice = %d, note = %d, loudness = 0x%04x %s\r\n",
							tick, s_events[s_event_head_index].oscillator,
							p_oscillator->voice, p_oscillator->note, p_oscillator->loudness,
							event_additional_string(s_event_head_index));
			SET_DEACTIVATED(p_oscillator->state_bits);
			break;
		case EVENT_DISCARD:
			CHIPTUNE_PRINTF(cEventTriggering,
							"tick = %u, DISCARD oscillator = %d, voice = %d, note = %d, amplitude = %1.2f%% of loudness %s\r\n",
							tick, s_events[s_event_head_index].oscillator,
							p_oscillator->voice, p_oscillator->note,
							100.0f * p_oscillator->amplitude/(float)p_oscillator->loudness,
							event_additional_string(s_event_head_index));
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
	return 0;
}

/**********************************************************************************/

void clean_all_events(void)
{
	reset_all_event_oscillators();
	for(int16_t i = 0; i < MAX_EVENT_NUMBER; i++){
		s_events[i].type = UNUSED_EVENT;
	}
	s_upcoming_event_number = 0;
}

/**********************************************************************************/

uint32_t const get_next_event_triggering_tick(void)
{
	if(0 == s_upcoming_event_number){
			return NULL_TICK;
	}

	return s_events[s_event_head_index].triggering_tick;
}

/**********************************************************************************/

uint32_t const get_upcoming_event_number(void)
{
	return s_upcoming_event_number;
}

/**********************************************************************************/

int adjust_event_triggering_tick_by_tempo(uint32_t const tick, float const new_tempo)
{
	float tempo_ratio = new_tempo/get_tempo();
	uint16_t event_index = s_event_head_index;
	bool is_reported = false;
	for(int i = 0; i < s_upcoming_event_number; i++){
		do
		{
			if(tick >= s_events[event_index].triggering_tick){
				break;
			}

#if(0)
			oscillator_t const * const p_oscillator
					= get_event_oscillator_pointer_from_index(s_events[event_index].oscillator);
			if(true == IS_FREEING(p_oscillator->state_bits)
					|| true ==  IS_RESTING(p_oscillator->state_bits)){
				break;
			}
#endif
			uint32_t const triggering_tick =
					(uint32_t)((s_events[event_index].triggering_tick - tick) * tempo_ratio) + tick;
			if(triggering_tick == s_events[event_index].triggering_tick){
				break;
			}
			if(false == is_reported){
				CHIPTUNE_PRINTF(cEventTriggering, "tick = %u, tempo change from %3.1f to %3.1f\r\n",
								tick, get_tempo(), new_tempo);
			}
			is_reported = true;
			CHIPTUNE_PRINTF(cEventTriggering, "event = %d, oscillator = %d, triggering_tick %u ->%u\r\n",
							event_index, s_events[event_index].oscillator,
							s_events[event_index].triggering_tick, triggering_tick);
			s_events[event_index].triggering_tick = triggering_tick;
		} while(0);
		event_index = s_events[event_index].next_event;
	}

	if(true == is_reported){
		CHIPTUNE_PRINTF(cEventTriggering, "-------------------------------------------------\r\n");
		CHIPTUNE_PRINTF(cEventTriggering, "-------------------------------------------------\r\n");
	}

	return 0;
}
