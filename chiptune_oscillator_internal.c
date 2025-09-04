#include <math.h>
#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_oscillator_internal.h"

static int16_t s_pitch_shift_in_semitones = 0;

void set_pitch_shift_in_semitones(int16_t pitch_shift_in_semitones)
{
	s_pitch_shift_in_semitones = pitch_shift_in_semitones;
}

/**********************************************************************************/

int16_t get_pitch_shift_in_semitones(void)
{
	return s_pitch_shift_in_semitones;
}
/**********************************************************************************/

uint16_t const calculate_oscillator_delta_phase(int8_t const voice,
												int16_t const note, float const pitch_chorus_bend_in_semitones)
{
	// TO DO : too many float variable
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);

	float corrected_note = (float)(note + s_pitch_shift_in_semitones) + p_channel_controller->tuning_in_semitones
			+ p_channel_controller->pitch_wheel_bend_in_semitones + pitch_chorus_bend_in_semitones;
	/*
	 * freq = 440 * 2**((note - 69)/12)
	*/
	float frequency = 440.0f * powf(2.0f, (corrected_note - 69.0f)/12.0f);
	frequency = roundf(frequency * 100.0f + 0.5f)/100.0f;
	/*
	 * sampling_rate/frequency = samples_per_cycle  = (UINT16_MAX + 1)/phase
	*/
	uint16_t delta_phase = (uint16_t)((UINT16_MAX + 1) * frequency / get_sampling_rate());
	return delta_phase;
}

/**********************************************************************************/

//xor-shift pesudo random https://en.wikipedia.org/wiki/Xorshift
static uint32_t s_chorus_random_seed = 20240129;

static uint16_t obtain_chorus_random(void)
{
	s_chorus_random_seed ^= s_chorus_random_seed << 13;
	s_chorus_random_seed ^= s_chorus_random_seed >> 17;
	s_chorus_random_seed ^= s_chorus_random_seed << 5;
	return (uint16_t)(s_chorus_random_seed);
}

/**********************************************************************************/
#define DIVIDE_BY_2(VALUE)							((VALUE) >> 1)
#define RAMDON_RANGE_TO_PLUS_MINUS_ONE(VALUE)	\
												(((DIVIDE_BY_2(UINT16_MAX) + 1) - (VALUE))/(float)(DIVIDE_BY_2(UINT16_MAX) + 1))

float const obtain_oscillator_pitch_chorus_bend_in_semitones(int8_t const chorus,
															float const max_pitch_chorus_bend_in_semitones)
{
	if(0 == chorus){
		return 0.0;
	}

	uint16_t random = obtain_chorus_random();
	float pitch_chorus_bend_in_semitones;
	pitch_chorus_bend_in_semitones = RAMDON_RANGE_TO_PLUS_MINUS_ONE(random) * chorus/(float)INT8_MAX;
	pitch_chorus_bend_in_semitones *= max_pitch_chorus_bend_in_semitones;
	//CHIPTUNE_PRINTF(cDeveloping, "pitch_chorus_bend_in_semitones = %3.2f\r\n", pitch_chorus_bend_in_semitones);
	return pitch_chorus_bend_in_semitones;
}

/**********************************************************************************/

int setup_envelope_state(oscillator_t *p_oscillator, uint8_t evelope_state){

	if(ENVELOPE_STATE_MAX <= evelope_state){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: undefined state number = %u in %s\r\n",
						evelope_state, __func__);
		return -1;
	}

	p_oscillator->envelope_state = evelope_state;
	p_oscillator->envelope_table_index = 0;
	p_oscillator->envelope_same_index_count = 0;
	return 0;
}

/**********************************************************************************/

typedef struct _oscillator_link
{
	int16_t previous;
	int16_t next;
} oscillator_link_t;

#ifdef _FIXED_OSCILLATOR_AND_EVENT_CAPACITY
static oscillator_t s_oscillators[OCCUPIABLE_OSCILLATOR_CAPACITY];
static oscillator_link_t s_oscillator_links[OCCUPIABLE_OSCILLATOR_CAPACITY];
#else
#define OSCILLATOR_POOL_CAPACITY			(64)

typedef struct _oscillator_pool
{
	oscillator_t oscillators[OSCILLATOR_POOL_CAPACITY];
	oscillator_link_t oscillator_links[OSCILLATOR_POOL_CAPACITY];
} oscillator_pool_t;

static oscillator_pool_t * s_oscillator_pool_pointer_table[(INT16_MAX+1) / OSCILLATOR_POOL_CAPACITY] = {NULL};
static int16_t s_number_of_oscillator_pool = 0;
#endif

static int16_t s_occupied_oscillator_number = 0;
int16_t s_occupied_oscillator_head_index = UNOCCUPIED_OSCILLATOR;
int16_t s_occupied_oscillator_last_index = UNOCCUPIED_OSCILLATOR;

#ifdef _FIXED_OSCILLATOR_AND_EVENT_CAPACITY
/**********************************************************************************/

static inline int16_t const get_occupiable_oscillator_capacity()
{
	return OCCUPIABLE_OSCILLATOR_CAPACITY;
}

/**********************************************************************************/

static inline oscillator_link_t * const get_oscillator_link_address_from_index(int16_t const index)
{
	return &s_oscillator_links[index];
}

/**********************************************************************************/

static inline oscillator_t * const get_oscillator_address_from_index(int16_t const index)
{
	return &s_oscillators[index];
}

/**********************************************************************************/

static inline bool is_unoccupied_oscillator_available()
{
	if(get_occupiable_oscillator_capacity() == s_occupied_oscillator_number){
		return false;
	}
	return true;
}

/**********************************************************************************/

static int mark_all_oscillators_and_links_unused(void)
{
	for(int16_t i = 0; i < get_occupiable_oscillator_capacity(); i++){
		get_oscillator_address_from_index(i)->voice = UNOCCUPIED_OSCILLATOR;
		oscillator_link_t * const p_oscillator_link = get_oscillator_link_address_from_index(i);
		p_oscillator_link->previous = UNOCCUPIED_OSCILLATOR;
		p_oscillator_link->next = UNOCCUPIED_OSCILLATOR;
	}
	s_occupied_oscillator_head_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_last_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_number = 0;
	return 0;
}

/**********************************************************************************/

static int release_all_oscillators_and_links(void) { return mark_all_oscillators_and_links_unused(); }
#else
/**********************************************************************************/

static inline int16_t const get_occupiable_oscillator_capacity()
{
	return s_number_of_oscillator_pool * OSCILLATOR_POOL_CAPACITY;
}

/**********************************************************************************/

static inline oscillator_link_t * const get_oscillator_link_address_from_index(int16_t const index)
{
	return &s_oscillator_pool_pointer_table[index / OSCILLATOR_POOL_CAPACITY]
				->oscillator_links[index % OSCILLATOR_POOL_CAPACITY];
}

/**********************************************************************************/

static inline oscillator_t * const get_oscillator_address_from_index(int16_t const index)
{
	return &s_oscillator_pool_pointer_table[index / OSCILLATOR_POOL_CAPACITY]
				->oscillators[index % OSCILLATOR_POOL_CAPACITY];
}

/**********************************************************************************/

static int append_new_oscillator_pool(void)
{
	int ret = 0;
	do
	{
		oscillator_pool_t *p_new_appending_oscillator_pool
			= (oscillator_pool_t*)chiptune_malloc(1 * sizeof(oscillator_pool_t));
		if(NULL == p_new_appending_oscillator_pool){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: allocate oscillator_pool_t fail\r\n");
			ret = -1;
			break;
		}
		for(int16_t i = 0; i < OSCILLATOR_POOL_CAPACITY; i++){
			p_new_appending_oscillator_pool->oscillators[i].voice = UNOCCUPIED_OSCILLATOR;
			p_new_appending_oscillator_pool->oscillator_links[i].previous = UNOCCUPIED_OSCILLATOR;
			p_new_appending_oscillator_pool->oscillator_links[i].next = UNOCCUPIED_OSCILLATOR;
		}

		s_oscillator_pool_pointer_table[s_number_of_oscillator_pool] = p_new_appending_oscillator_pool;
		s_number_of_oscillator_pool += 1;
	}while(0);

	return ret;
}

/**********************************************************************************/

static bool is_unoccupied_oscillator_available()
{
	bool ret = true;
	if(get_occupiable_oscillator_capacity() == s_occupied_oscillator_number){
		do
		{
			if(INT16_MAX == s_occupied_oscillator_number){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_number"
											 " reaches the CAP INT16_MAX\r\n");
				ret = false;
				break;
			}
			if(0 > append_new_oscillator_pool()){
				ret = false;
				break;
			}
		}while(0);
	}
	return ret;
}

/**********************************************************************************/

static int mark_all_oscillators_and_links_unused(void)
{
	for(int j = 0; j < s_number_of_oscillator_pool; j++){
		oscillator_pool_t *p_oscillator_pool = s_oscillator_pool_pointer_table[j];
		for(int16_t i = 0; i < OSCILLATOR_POOL_CAPACITY; i++){
			p_oscillator_pool->oscillators[i].voice = UNOCCUPIED_OSCILLATOR;
			p_oscillator_pool->oscillator_links[i].previous = UNOCCUPIED_OSCILLATOR;
			p_oscillator_pool->oscillator_links[i].next = UNOCCUPIED_OSCILLATOR;
		}
	}

	s_occupied_oscillator_head_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_last_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_number = 0;
	return 0;
}

/**********************************************************************************/

static int release_all_oscillators_and_links(void)
{
	for(int j = 0; j < s_number_of_oscillator_pool; j++){
		chiptune_free(s_oscillator_pool_pointer_table[j]);
		s_oscillator_pool_pointer_table[j] = NULL;
	}
	s_number_of_oscillator_pool = 0;

	s_occupied_oscillator_head_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_last_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_number = 0;
	return 0;
}
#endif

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
		while(UNOCCUPIED_OSCILLATOR != get_oscillator_link_address_from_index(current_index)->next)
		{
			counter += 1;
			current_index = get_oscillator_link_address_from_index(current_index)->next;
		}
		if(counter != s_occupied_oscillator_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: FORWARDING occupied_oscillators list length = %d"
							", not matches s_occupied_oscillator_number = %d\r\n", counter, s_occupied_oscillator_number);
			ret = -3;
			break;
		}

		current_index = s_occupied_oscillator_last_index;
		counter = 1;
		while(UNOCCUPIED_OSCILLATOR != get_oscillator_link_address_from_index(current_index)->previous)
		{
			counter += 1;
			current_index = get_oscillator_link_address_from_index(current_index)->previous;
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
	do {
		oscillator_link_t * const p_this_index_oscillator_link = get_oscillator_link_address_from_index(index);
		if(0 == s_occupied_oscillator_number){
			p_this_index_oscillator_link->previous = UNOCCUPIED_OSCILLATOR;
			p_this_index_oscillator_link->next = UNOCCUPIED_OSCILLATOR;
			s_occupied_oscillator_head_index = index;
			break;
		}

		get_oscillator_link_address_from_index(s_occupied_oscillator_last_index)->next = index;
		p_this_index_oscillator_link->previous = s_occupied_oscillator_last_index;
		p_this_index_oscillator_link->next = UNOCCUPIED_OSCILLATOR;
	} while(0);
	s_occupied_oscillator_last_index = index;
	s_occupied_oscillator_number += 1;
	CHECK_OCCUPIED_OSCILLATOR_LIST();
	return 0;
}

/**********************************************************************************/

oscillator_t * const acquire_freed_oscillator(int16_t * const p_index)
{
	if(false == is_unoccupied_oscillator_available()){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR::all oscillators are used\r\n");
		return NULL;
	}
	for(int16_t i = 0; i < get_occupiable_oscillator_capacity(); i++){
		if(UNOCCUPIED_OSCILLATOR == get_oscillator_address_from_index(i)->voice){
			*p_index = i;
			occupy_oscillator(i);
			return get_oscillator_address_from_index(i);
		}
	}
	CHIPTUNE_PRINTF(cDeveloping, "ERROR::available oscillator is not found\r\n");
	*p_index = UNOCCUPIED_OSCILLATOR;
	return NULL;
}

/**********************************************************************************/

int discard_oscillator(int16_t const index)
{
	oscillator_link_t * const p_this_index_oscillator_link = get_oscillator_link_address_from_index(index);
	int16_t previous_index = p_this_index_oscillator_link->previous;
	int16_t next_index = p_this_index_oscillator_link->next;

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
			get_oscillator_link_address_from_index(s_occupied_oscillator_head_index)->previous = UNOCCUPIED_OSCILLATOR;
			break;
		}

		if(index == s_occupied_oscillator_last_index){
			s_occupied_oscillator_last_index = previous_index;
			get_oscillator_link_address_from_index(s_occupied_oscillator_last_index)->next = UNOCCUPIED_OSCILLATOR;
			break;
		}

		get_oscillator_link_address_from_index(previous_index)->next = next_index;
		get_oscillator_link_address_from_index(next_index)->previous = previous_index;
	} while (0);

	p_this_index_oscillator_link->previous = UNOCCUPIED_OSCILLATOR;
	p_this_index_oscillator_link->next = UNOCCUPIED_OSCILLATOR;
	get_oscillator_address_from_index(index)->voice = UNOCCUPIED_OSCILLATOR;
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

int16_t const get_occupied_oscillator_head_index()
{
	if(-1 == s_occupied_oscillator_head_index
			&& 0 != s_occupied_oscillator_number){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_head_index = -1:: but s_occupied_oscillator_number = %d\r\n",
						s_occupied_oscillator_number);
	}
	return s_occupied_oscillator_head_index;
}

/**********************************************************************************/

static inline bool is_occupied_oscillator_index_out_of_range(int16_t const index)
{
	bool is_out_of_range = true;
	do {
		if(index < 0){
			break;
		}
		if(index >= get_occupiable_oscillator_capacity()){
			break;
		}
		is_out_of_range = false;
	}while(0);

	return is_out_of_range;
}

/**********************************************************************************/

int16_t const get_occupied_oscillator_next_index(int16_t const index)
{
	if(true == is_occupied_oscillator_index_out_of_range(index)){
		CHIPTUNE_PRINTF(cDeveloping, "oscillator index = %d, out of range \r\n", index);
		return UNOCCUPIED_OSCILLATOR;
	}

	return get_oscillator_link_address_from_index(index)->next;
}

/**********************************************************************************/

oscillator_t * const get_oscillator_pointer_from_index(int16_t const index)
{
	if(true == is_occupied_oscillator_index_out_of_range(index)){
		CHIPTUNE_PRINTF(cDeveloping, "oscillator index = %d, out of range \r\n", index);
		return NULL;
	}

	return get_oscillator_address_from_index(index);
}

/**********************************************************************************/

int mark_all_oscillators_unused(void)
{
	return mark_all_oscillators_and_links_unused();
}

/**********************************************************************************/

int release_all_oscillators(void)
{
	return release_all_oscillators_and_links();
}
