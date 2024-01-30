#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_oscillator_internal.h"

#define MAX_OSCILLATOR_NUMBER						(MIDI_MAX_CHANNEL_NUMBER * 8)

static struct _oscillator s_oscillators[MAX_OSCILLATOR_NUMBER];
static int16_t s_occupied_oscillator_number = 0;

struct _occupied_oscillator_node
{
	int16_t previous;
	int16_t next;
}s_occupied_oscillator_nodes[MAX_OSCILLATOR_NUMBER];

int16_t s_head_occupied_oscillator_index = UNUSED_OSCILLATOR;
int16_t s_last_occupied_oscillator_index = UNUSED_OSCILLATOR;

#ifdef _CHECK_OCCUPIED_OSCILLATOR_LIST
void check_occupied_oscillator_list(void)
{
	if(0 > s_occupied_oscillator_number){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_number = %d", s_occupied_oscillator_number);
		return ;
	}

	if(0 == s_occupied_oscillator_number){
		if(UNUSED_OSCILLATOR != s_head_occupied_oscillator_index){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_number = 0"
										 " but s_head_occupied_oscillator_index is not UNUSED_OSCILLATOR\r\n");
		}
		return ;
	}

	int16_t current_index;
	int16_t counter;

	current_index = s_head_occupied_oscillator_index;
	counter= 1;
	while(UNUSED_OSCILLATOR != s_occupied_oscillator_nodes[current_index].next)
	{
		counter += 1;
		current_index = s_occupied_oscillator_nodes[current_index].next;
	}
	if(counter != s_occupied_oscillator_number){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: FORWARDING occupied_oscillators list length = %d"
						", not matches s_occupied_oscillator_number = %d\r\n", counter, s_occupied_oscillator_number);

		return ;
	}

	current_index = s_last_occupied_oscillator_index;
	counter = 1;
	while(UNUSED_OSCILLATOR != s_occupied_oscillator_nodes[current_index].previous)
	{
		counter += 1;
		current_index = s_occupied_oscillator_nodes[current_index].previous;
	}

	if(counter != s_occupied_oscillator_number){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: BACKWARDING occupied_oscillators list length = %d"
						", not matches s_occupied_oscillator_number = %d\r\n", counter, s_occupied_oscillator_number);

		return ;
	}
}
#define CHECK_OCCUPIED_OSCILLATOR_LIST()			do { \
														check_occupied_oscillator_list(); \
													} while(0)
#else
#define CHECK_OCCUPIED_OSCILLATOR_LIST()			do { \
														(void)0; \
													} while(0)
#endif
/**********************************************************************************/

struct _oscillator * const acquire_oscillator(int16_t * const p_index)
{
	if(MAX_OSCILLATOR_NUMBER == s_occupied_oscillator_number){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR::all oscillators are used\r\n");
		return NULL;
	}

	int16_t i;
	do {
		if(0 == s_occupied_oscillator_number){

			s_occupied_oscillator_nodes[0].previous = UNUSED_OSCILLATOR;
			s_occupied_oscillator_nodes[0].next = UNUSED_OSCILLATOR;
			s_head_occupied_oscillator_index = 0;
			s_last_occupied_oscillator_index = 0;

			i = 0;
			break;
		}


		bool is_found = false;
		for(i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
			if(UNUSED_OSCILLATOR == s_oscillators[i].voice){
				s_occupied_oscillator_nodes[s_last_occupied_oscillator_index].next = i;
				s_occupied_oscillator_nodes[i].previous = s_last_occupied_oscillator_index;
				s_occupied_oscillator_nodes[i].next = UNUSED_OSCILLATOR;
				s_last_occupied_oscillator_index = i;
				is_found = true;
				break;
			}
		}
		if(false == is_found){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR::available oscillator is not found\r\n");
			*p_index = UNUSED_OSCILLATOR;
		}
	} while(0);

	s_occupied_oscillator_number += 1;
	CHECK_OCCUPIED_OSCILLATOR_LIST();
	*p_index = i;
	return &s_oscillators[i];
}

/**********************************************************************************/

int discard_oscillator(int16_t const index)
{
	int16_t previous_index = s_occupied_oscillator_nodes[index].previous;
	int16_t next_index = s_occupied_oscillator_nodes[index].next;

	do {
		if(0 == s_occupied_oscillator_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: all oscillators have been discarded\r\n");
			return -1;
		}

		if(1 != s_occupied_oscillator_number
				&& (UNUSED_OSCILLATOR == previous_index && UNUSED_OSCILLATOR == next_index)){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: oscillator %d is not in the occupied list\r\n", index);
			return -2;
		}
	} while(0);

	do {
		if(index == s_head_occupied_oscillator_index){
			s_head_occupied_oscillator_index = next_index;
			s_occupied_oscillator_nodes[s_head_occupied_oscillator_index].previous = UNUSED_OSCILLATOR;
			break;
		}

		if(index == s_last_occupied_oscillator_index){
			s_last_occupied_oscillator_index = previous_index;
			s_occupied_oscillator_nodes[s_last_occupied_oscillator_index].next = UNUSED_OSCILLATOR;
			break;
		}

		s_occupied_oscillator_nodes[previous_index].next = next_index;
		s_occupied_oscillator_nodes[next_index].previous = previous_index;
	} while (0);

	s_occupied_oscillator_nodes[index].previous = UNUSED_OSCILLATOR;
	s_occupied_oscillator_nodes[index].next = UNUSED_OSCILLATOR;
	s_oscillators[index].voice = UNUSED_OSCILLATOR;
	s_occupied_oscillator_number -= 1;

	CHECK_OCCUPIED_OSCILLATOR_LIST();
	return 0;
}

/**********************************************************************************/

int16_t const get_occupied_oscillator_number(void)
{
	return s_occupied_oscillator_number;
}

/**********************************************************************************/

int16_t get_head_occupied_oscillator_index()
{
	if(-1 == s_head_occupied_oscillator_index && 0 != s_occupied_oscillator_number){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_head_occupied_oscillator_index = -1:: but s_occupied_oscillator_number = %d\r\n",
						s_occupied_oscillator_number);
	}
	return s_head_occupied_oscillator_index;
}

/**********************************************************************************/

int16_t get_next_occupied_oscillator_index(int16_t const index)
{
	if(false == (index >= 0 && index < MAX_OSCILLATOR_NUMBER)){
		CHIPTUNE_PRINTF(cDeveloping, "oscillator index = %d out of range \r\n", index);
		return UNUSED_OSCILLATOR;
	}

	return 	s_occupied_oscillator_nodes[index].next;
}

/**********************************************************************************/

struct _oscillator * const get_oscillator_pointer_from_index(int16_t const index)
{
	if(false == (index >= 0 && index < MAX_OSCILLATOR_NUMBER)){
		CHIPTUNE_PRINTF(cDeveloping, "oscillator index = %u out of range \r\n", index);
		return NULL;
	}

	return &s_oscillators[index];
}

/**********************************************************************************/

void reset_all_oscillators(void)
{
	for(int16_t i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		s_oscillators[i].voice = UNUSED_OSCILLATOR;
		s_occupied_oscillator_nodes[i]
				= (struct _occupied_oscillator_node){.previous = UNUSED_OSCILLATOR, .next = UNUSED_OSCILLATOR};
	}
	s_head_occupied_oscillator_index = UNUSED_OSCILLATOR;
	s_last_occupied_oscillator_index = UNUSED_OSCILLATOR;
	s_occupied_oscillator_number = 0;
}

/**********************************************************************************/
