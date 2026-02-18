// NOLINTBEGIN(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
#include <string.h>
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

int const update_oscillator_phase_increment(oscillator_t * const p_oscillator)
{
	channel_controller_t const * const p_channel_controller
			= get_channel_controller_pointer_from_index(p_oscillator->voice);

	float corrected_pitch = (float)(p_oscillator->note)
			+ s_pitch_shift_in_semitones
			+ p_channel_controller->tuning_in_semitones
			+ p_channel_controller->pitch_wheel_bend_in_semitones
			+ p_oscillator->pitch_detune_in_semitones;
	p_oscillator->base_phase_increment = calculate_phase_increment_from_pitch(corrected_pitch);

	corrected_pitch += p_channel_controller->vibrato_depth_in_semitones;
	p_oscillator->max_vibrato_phase_increment =
			calculate_phase_increment_from_pitch(corrected_pitch) - p_oscillator->base_phase_increment;
	return 0;
}

/**********************************************************************************/

typedef struct _oscillator_link
{
	int16_t previous;
	int16_t next;
} oscillator_link_t;

#ifdef _USE_STATIC_RESOURCE_ALLOCATION
	#define OSCILLATOR_POOL_CAPACITY			(512)
#else
	#define OSCILLATOR_POOL_CAPACITY			(64)
#endif

typedef struct _oscillator_pool
{
	oscillator_t oscillators[OSCILLATOR_POOL_CAPACITY];
	oscillator_link_t oscillator_links[OSCILLATOR_POOL_CAPACITY];
} oscillator_pool_t;

#ifdef _USE_STATIC_RESOURCE_ALLOCATION
static oscillator_pool_t	s_oscillator_pool;

static oscillator_pool_t *	const s_oscillator_pool_pointer_table[1] = {&s_oscillator_pool};
static int16_t const		s_number_of_oscillator_pool = 1;
#else
static oscillator_pool_t *	s_oscillator_pool_pointer_table[(INT16_MAX+1) / OSCILLATOR_POOL_CAPACITY] = {NULL};
static int16_t				s_number_of_oscillator_pool = 0;
#endif

static int16_t s_occupied_oscillator_number = 0;
int16_t s_occupied_oscillator_head_index = UNOCCUPIED_OSCILLATOR;
int16_t s_occupied_oscillator_last_index = UNOCCUPIED_OSCILLATOR;

/**********************************************************************************/

static inline int16_t const get_oscillator_capacity()
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

#ifdef _USE_STATIC_RESOURCE_ALLOCATION
/**********************************************************************************/

static inline bool is_to_append_oscillator_pool_successfully(void){ return false;}

#else
/**********************************************************************************/

static int allocate_append_oscillator_pool(void)
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

static inline bool is_to_append_oscillator_pool_successfully(void)
{
	bool ret = true;
	do
	{
		if(INT16_MAX == s_occupied_oscillator_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_number"
										 " reaches the CAP INT16_MAX\r\n");
			ret = false;
			break;
		}
		if(0 > allocate_append_oscillator_pool()){
			ret = false;
			break;
		}
	}while(0);

	return ret;
}
#endif

/**********************************************************************************/

static bool is_unoccupied_oscillator_available()
{
	bool ret = true;
	if(get_oscillator_capacity() == s_occupied_oscillator_number){
		ret = is_to_append_oscillator_pool_successfully();
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
#ifdef _USE_STATIC_RESOURCE_ALLOCATION
	return mark_all_oscillators_and_links_unused();
#else
	for(int j = 0; j < s_number_of_oscillator_pool; j++){
		chiptune_free(s_oscillator_pool_pointer_table[j]);
		s_oscillator_pool_pointer_table[j] = NULL;
	}
	s_number_of_oscillator_pool = 0;

	s_occupied_oscillator_head_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_last_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_number = 0;
	return 0;
#endif
}

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

typedef struct _midi_effect_associate_link_t
{
	uint8_t midi_effect_type;
	int16_t associate_oscillator_indexes[SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER];
	int16_t next_link_index;
} midi_effect_associate_link_t;

#define NO_MIDI_EFFECT_ASSOCIATE_LINK				(-1)
#define IS_MIDI_EFFECT_TYPE_MATCHED(TYPE_A, TYPE_B)	( ((TYPE_A) & (TYPE_B)) ? true : false)

#if 1
	#define MIDI_EFFECT_ASSOCIATE_LINK_POOL_CAPACITY \
													(256)
#else
	#define MIDI_EFFECT_ASSOCIATE_LINK_POOL_CAPACITY	(32)
#endif

typedef struct _midi_effect_associate_link_pool
{
	midi_effect_associate_link_t midi_effect_associate_links[MIDI_EFFECT_ASSOCIATE_LINK_POOL_CAPACITY];
} midi_effect_associate_link_pool_t;


#ifdef _USE_STATIC_RESOURCE_ALLOCATION
static midi_effect_associate_link_pool_t	s_associate_midi_effect_associate_link_pool;
static midi_effect_associate_link_pool_t *	const s_midi_effect_associate_link_pool_t_pointer_table[1]
											= {&s_associate_midi_effect_associate_link_pool};
static int16_t const	s_number_of_associate_midi_effect_associate_link_pool = 1;
#else
static midi_effect_associate_link_pool_t *	s_midi_effect_associate_link_pool_t_pointer_table[(INT16_MAX + 1)/MIDI_EFFECT_ASSOCIATE_LINK_POOL_CAPACITY];
static int16_t			s_number_of_associate_midi_effect_associate_link_pool = 0;
#endif

int16_t s_used_midi_effect_associate_link_number = 0;

/**********************************************************************************/

static inline int16_t const get_midi_effect_associate_link_capacity()
{
	return s_number_of_associate_midi_effect_associate_link_pool * MIDI_EFFECT_ASSOCIATE_LINK_POOL_CAPACITY;
}

/**********************************************************************************/

static midi_effect_associate_link_t * const get_midi_effect_associate_link_from_index(int16_t const index)
{
	return &s_midi_effect_associate_link_pool_t_pointer_table[index / MIDI_EFFECT_ASSOCIATE_LINK_POOL_CAPACITY]
				->midi_effect_associate_links[index % MIDI_EFFECT_ASSOCIATE_LINK_POOL_CAPACITY];
}

/**********************************************************************************/

#ifdef _USE_STATIC_RESOURCE_ALLOCATION
/**********************************************************************************/
static inline bool is_to_midi_effect_associate_link_pool_successfully(void){ return false; }
#else

/**********************************************************************************/

static int allocate_and_append_midi_effect_associate_link_pool(void)
{
	int ret = 0;
	do
	{
		midi_effect_associate_link_pool_t * const p_new_midi_effect_associate_link_pool
			= (midi_effect_associate_link_pool_t*)chiptune_malloc(1 * sizeof(midi_effect_associate_link_pool_t));

		if(NULL == p_new_midi_effect_associate_link_pool){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: allocate midi_effect_associate_link_pool_t fail\r\n");
			ret = -1;
			break;
		}
		for(int16_t i = 0; i < MIDI_EFFECT_ASSOCIATE_LINK_POOL_CAPACITY; i++){
			p_new_midi_effect_associate_link_pool->midi_effect_associate_links[i].midi_effect_type = MidiEffectNone;
		}

		s_midi_effect_associate_link_pool_t_pointer_table[s_number_of_associate_midi_effect_associate_link_pool]
				= p_new_midi_effect_associate_link_pool;
		s_number_of_associate_midi_effect_associate_link_pool += 1;
	}while(0);

	return ret;
}

/**********************************************************************************/

static inline bool is_to_midi_effect_associate_link_pool_successfully(void)
{
	int ret = true;
	do
	{
		if(INT16_MAX == s_used_midi_effect_associate_link_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_used_midi_effect_associate_link_number"
										 " reaches the CAP INT16_MAX\r\n");
			ret = false;
			break;
		}
		if(0 > allocate_and_append_midi_effect_associate_link_pool()){
			ret = false;
			break;
		}
	}while(0);
	return ret;
}
#endif


static bool is_unused_midi_effect_associate_link_available()
{
	bool ret = true;
	if(get_midi_effect_associate_link_capacity() == s_used_midi_effect_associate_link_number){
		ret = is_to_midi_effect_associate_link_pool_successfully();
	}
	return ret;
}

/**********************************************************************************/

static void mark_all_midi_effect_associate_links_unused(void)
{
	for(int16_t j = 0; j < s_number_of_associate_midi_effect_associate_link_pool; j++){
		midi_effect_associate_link_pool_t * const p_midi_effect_associate_link_pool
				= s_midi_effect_associate_link_pool_t_pointer_table[j];
		for(int16_t i = 0; i < MIDI_EFFECT_ASSOCIATE_LINK_POOL_CAPACITY; i++){
			p_midi_effect_associate_link_pool->midi_effect_associate_links[i].midi_effect_type = MidiEffectNone;
		}
	}
	s_used_midi_effect_associate_link_number = 0;
}

/**********************************************************************************/

static void release_all_midi_effect_associate_links(void)
{
#ifdef _USE_STATIC_RESOURCE_ALLOCATION
	mark_all_midi_effect_associate_links_unused();
#else
	for(int16_t j = 0; j < s_number_of_associate_midi_effect_associate_link_pool; j++){
		chiptune_free(s_midi_effect_associate_link_pool_t_pointer_table[j]);
		s_midi_effect_associate_link_pool_t_pointer_table[j] = NULL;
	}
	s_number_of_associate_midi_effect_associate_link_pool = 0;
	s_used_midi_effect_associate_link_number = 0;
#endif
}

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

oscillator_t * const acquire_oscillator(int16_t * const p_index)
{
	*p_index = UNOCCUPIED_OSCILLATOR;
	if(false == is_unoccupied_oscillator_available()){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: all oscillators are used\r\n");
		return NULL;
	}

	int16_t i;
	for(i = 0; i < get_oscillator_capacity(); i++){
		if(UNOCCUPIED_OSCILLATOR == get_oscillator_address_from_index(i)->voice){
			break;
		}
	}

	oscillator_t * p_oscillator = NULL;
	do
	{
		if(get_oscillator_capacity() == i){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: available oscillator is not found\r\n");
			break;
		}
		occupy_oscillator(i);
		*p_index = i;
		p_oscillator = get_oscillator_address_from_index(i);
		memset(p_oscillator, 0, sizeof(oscillator_t));
		p_oscillator->midi_effect_aassociate_link_index = NO_MIDI_EFFECT_ASSOCIATE_LINK;
	} while(0);

	return p_oscillator;
}

/**********************************************************************************/

oscillator_t * const replicate_oscillator(int16_t const original_index, int16_t * const p_index)
{
	oscillator_t * const p_oscillator = acquire_oscillator(p_index);
	do
	{
		if(NULL == p_oscillator){
			break;
		}
		oscillator_t * const p_original_oscillator = get_oscillator_address_from_index(original_index);
		memcpy(p_oscillator, p_original_oscillator, sizeof(oscillator_t));
		p_oscillator->midi_effect_aassociate_link_index = NO_MIDI_EFFECT_ASSOCIATE_LINK;
	}while(0);

	return p_oscillator;
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

	oscillator_t * const p_oscillator = get_oscillator_address_from_index(index);

	if(false == (true == IS_PERCUSSION_OSCILLATOR(p_oscillator))){
		if(NO_MIDI_EFFECT_ASSOCIATE_LINK != p_oscillator->midi_effect_aassociate_link_index){
			midi_effect_associate_link_t *p_midi_effect_associate_link
					= get_midi_effect_associate_link_from_index(p_oscillator->midi_effect_aassociate_link_index);
			while(1)
			{
				p_midi_effect_associate_link->midi_effect_type = MidiEffectNone;
				s_used_midi_effect_associate_link_number -= 1;
				if(NO_MIDI_EFFECT_ASSOCIATE_LINK == p_midi_effect_associate_link->next_link_index){
					break;
				}
				p_midi_effect_associate_link
						= get_midi_effect_associate_link_from_index(p_midi_effect_associate_link->next_link_index);
			}
		}
		p_oscillator->midi_effect_aassociate_link_index = NO_MIDI_EFFECT_ASSOCIATE_LINK;
	}
	p_oscillator->voice = UNOCCUPIED_OSCILLATOR;
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
		if(index >= get_oscillator_capacity()){
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
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: oscillator index = %d, out of range \r\n", index);
		return UNOCCUPIED_OSCILLATOR;
	}

	return get_oscillator_link_address_from_index(index)->next;
}

/**********************************************************************************/

oscillator_t * const get_oscillator_pointer_from_index(int16_t const index)
{
	if(true == is_occupied_oscillator_index_out_of_range(index)){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: oscillator index = %d, out of range \r\n", index);
		return NULL;
	}

	return get_oscillator_address_from_index(index);
}

/**********************************************************************************/

int clear_all_oscillators(void)
{
	mark_all_midi_effect_associate_links_unused();
	mark_all_oscillators_and_links_unused();
	return 0;
}

/**********************************************************************************/

int destroy_all_oscillators(void)
{
	release_all_midi_effect_associate_links();
	release_all_oscillators_and_links();
	return 0;
}

/**********************************************************************************/

int store_associate_oscillator_indexes(uint8_t const midi_effect_type, int16_t const index,
									  int16_t const * const p_associate_indexes)
{
	int ret = 0;
	oscillator_t  * const p_oscillator = get_oscillator_address_from_index(index);
	do
	{
		if(NULL == p_oscillator){
			ret = -1;
			break;
		}

		if(MidiEffectAll == midi_effect_type || MidiEffectNone == midi_effect_type){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: %s :: midi_effect_type could not be"
										 " MidiEffectAll or MidiEffectNone\r\n", __func__);
			ret = -2;
			break;
		}

		if(false == is_unused_midi_effect_associate_link_available()){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: all midi_effect_associate_link are used\r\n");
			break;
		}

		int16_t new_link_index;
		for(new_link_index = 0; new_link_index < get_midi_effect_associate_link_capacity(); new_link_index++){
			if(MidiEffectNone == get_midi_effect_associate_link_from_index(new_link_index)->midi_effect_type){
				break;
			}
		}
		s_used_midi_effect_associate_link_number += 1;

		{
			midi_effect_associate_link_t * const p_new_link
					= get_midi_effect_associate_link_from_index(new_link_index);
			p_new_link->midi_effect_type = midi_effect_type;
			p_new_link->next_link_index = NO_MIDI_EFFECT_ASSOCIATE_LINK;
			for(int16_t i = 0; i < SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER; i++){
				p_new_link->associate_oscillator_indexes[i] = p_associate_indexes[i];
			}
		}

		{
			int16_t current_link_index = p_oscillator->midi_effect_aassociate_link_index;
			if(NO_MIDI_EFFECT_ASSOCIATE_LINK == current_link_index){
				p_oscillator->midi_effect_aassociate_link_index = new_link_index;
				break;
			}

			while(1)
			{
				midi_effect_associate_link_t * p_current_link
						= get_midi_effect_associate_link_from_index(current_link_index);
				if(NO_MIDI_EFFECT_ASSOCIATE_LINK == p_current_link->next_link_index){
					get_midi_effect_associate_link_from_index(current_link_index)->next_link_index = new_link_index;
					break;
				}
				current_link_index = p_current_link->next_link_index;
			}
		}
	}while(0);

	return ret;
}

/**********************************************************************************/

int16_t count_all_subordinate_oscillators(uint8_t const midi_effect_type, int16_t const root_index)
{
	return get_all_subordinate_oscillator_indexes(midi_effect_type, root_index, NULL);
}

/**********************************************************************************/

int get_all_subordinate_oscillator_indexes(uint8_t const midi_effect_type, int16_t const root_index,
										   int16_t * const p_associate_indexes)
{
	oscillator_t * const p_oscillator
			= get_oscillator_address_from_index(root_index);
	int16_t subordinate_number = 0;
	do{
		if(NO_MIDI_EFFECT_ASSOCIATE_LINK == p_oscillator->midi_effect_aassociate_link_index){
			break;
		}
		int16_t midi_effect_associate_link_index = p_oscillator->midi_effect_aassociate_link_index;

		while(1)
		{
			midi_effect_associate_link_t * const p_midi_effect_associate_link
					= get_midi_effect_associate_link_from_index(midi_effect_associate_link_index);
			if(MidiEffectNone == p_midi_effect_associate_link->midi_effect_type){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: midi_effect_associate_link index = %d is labeled as MidiEffectNone\r\n",
								midi_effect_associate_link_index);
				break;
			}

			do
			{
				if(true == IS_MIDI_EFFECT_TYPE_MATCHED(midi_effect_type,
													   p_midi_effect_associate_link->midi_effect_type)){
					if(NULL != p_associate_indexes){
						for(int16_t i = 0; i < SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER; i++){
							p_associate_indexes[i + subordinate_number]
									= p_midi_effect_associate_link->associate_oscillator_indexes[i];
						}
					}
					subordinate_number += SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER;
					break;
				}

				for(int16_t i = 0; i < SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER; i++){
					int16_t * p_moving_associate_indexes
							= p_associate_indexes + subordinate_number * ( NULL != p_associate_indexes);
					subordinate_number +=
							get_all_subordinate_oscillator_indexes(midi_effect_type,
																   p_midi_effect_associate_link->associate_oscillator_indexes[i],
																   p_moving_associate_indexes);
				}
			}while(0);
			if(NO_MIDI_EFFECT_ASSOCIATE_LINK == p_midi_effect_associate_link->next_link_index){
				break;
			}
			midi_effect_associate_link_index = p_midi_effect_associate_link->next_link_index;
		}
	}while(0);

	return subordinate_number;
}

// NOLINTEND(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
