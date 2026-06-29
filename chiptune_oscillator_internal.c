// NOLINTBEGIN(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
#include <string.h>
#include <math.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_midi_effect_internal.h"

#include "chiptune_oscillator_internal.h"

#ifdef _DEBUG
#define _ENABLE_CHECK_OCCUPIED_OSCILLATOR_LIST
#endif

static int16_t s_pitch_shift_in_semitones = 0;

void set_pitch_shift_in_semitones(int16_t const pitch_shift_in_semitones)
{
	s_pitch_shift_in_semitones = pitch_shift_in_semitones;
}

/**********************************************************************************/

int16_t get_pitch_shift_in_semitones(void)
{
	return s_pitch_shift_in_semitones;
}

/**********************************************************************************/

void update_oscillator_phase_increment(oscillator_t * const p_oscillator)
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
}

/**********************************************************************************/

typedef struct _oscillator_node
{
	oscillator_t oscillator;
	int16_t previous_index;
	int16_t next_index;
} oscillator_node_t;

#ifdef _USE_STATIC_RESOURCE_ALLOCATION
	#define OSCILLATOR_POOL_CAPACITY			(512)
#else
	#define OSCILLATOR_POOL_CAPACITY			(64)
#endif

typedef struct _oscillator_node_pool
{
	oscillator_node_t oscillator_nodes[OSCILLATOR_POOL_CAPACITY];
} oscillator_node_pool_t;

#ifdef _USE_STATIC_RESOURCE_ALLOCATION
static oscillator_node_pool_t	s_oscillator_node_pool;

static oscillator_node_pool_t *	const s_oscillator_node_pool_pointer_table[1] = {&s_oscillator_node_pool};
static int16_t const			s_number_of_oscillator_node_pool = 1;
#else
static oscillator_node_pool_t *	s_oscillator_node_pool_pointer_table[(INT16_MAX+1) / OSCILLATOR_POOL_CAPACITY] = {NULL};
static int16_t					s_number_of_oscillator_node_pool = 0;
#endif

static int16_t s_occupied_oscillator_node_number = 0;
int16_t s_occupied_oscillator_node_head_index = UNOCCUPIED_OSCILLATOR;
int16_t s_occupied_oscillator_node_last_index = UNOCCUPIED_OSCILLATOR;

/**********************************************************************************/
static inline int16_t const get_oscillator_node_capacity()
{
	return s_number_of_oscillator_node_pool * OSCILLATOR_POOL_CAPACITY;
}

/**********************************************************************************/
static inline oscillator_node_t * const get_oscillator_node_address_from_index(int16_t const oscillator_index)
{
	return &s_oscillator_node_pool_pointer_table[oscillator_index / OSCILLATOR_POOL_CAPACITY]
				->oscillator_nodes[oscillator_index % OSCILLATOR_POOL_CAPACITY];
}

#ifdef _USE_STATIC_RESOURCE_ALLOCATION
/**********************************************************************************/
static inline bool is_to_append_oscillator_node_pool_successfully(void){ return false;}

#else
/**********************************************************************************/
static int allocate_append_oscillator_node_pool(void)
{
	int ret = 0;
	do
	{
		oscillator_node_pool_t *p_new_appending_oscillator_node_pool
			= (oscillator_node_pool_t*)chiptune_malloc(1 * sizeof(oscillator_node_pool_t));
		if(NULL == p_new_appending_oscillator_node_pool){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: allocate oscillator_node_pool_t fail\r\n");
			ret = -1;
			break;
		}
		for(int16_t i = 0; i < OSCILLATOR_POOL_CAPACITY; i++){
			p_new_appending_oscillator_node_pool->oscillator_nodes[i].oscillator.voice = UNOCCUPIED_OSCILLATOR;
			p_new_appending_oscillator_node_pool->oscillator_nodes[i].previous_index = UNOCCUPIED_OSCILLATOR;
			p_new_appending_oscillator_node_pool->oscillator_nodes[i].next_index = UNOCCUPIED_OSCILLATOR;
		}

		s_oscillator_node_pool_pointer_table[s_number_of_oscillator_node_pool] = p_new_appending_oscillator_node_pool;
		s_number_of_oscillator_node_pool += 1;
	}while(0);

	return ret;
}

/**********************************************************************************/
static inline bool is_to_append_oscillator_node_pool_successfully(void)
{
	bool ret = true;
	do
	{
		if(INT16_MAX == s_occupied_oscillator_node_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_node_number"
										 " reaches the CAP INT16_MAX\r\n");
			ret = false;
			break;
		}
		if(0 > allocate_append_oscillator_node_pool()){
			ret = false;
			break;
		}
	}while(0);

	return ret;
}
#endif

/**********************************************************************************/
static bool is_unoccupied_oscillator_node_available()
{
	bool ret = true;
	if(get_oscillator_node_capacity() == s_occupied_oscillator_node_number){
		ret = is_to_append_oscillator_node_pool_successfully();
	}
	return ret;
}

/**********************************************************************************/
static void mark_all_oscillator_nodes_unused(void)
{
	for(int j = 0; j < s_number_of_oscillator_node_pool; j++){
		oscillator_node_pool_t *p_oscillator_node_pool = s_oscillator_node_pool_pointer_table[j];
		for(int16_t i = 0; i < OSCILLATOR_POOL_CAPACITY; i++){
			p_oscillator_node_pool->oscillator_nodes[i].oscillator.voice = UNOCCUPIED_OSCILLATOR;
			p_oscillator_node_pool->oscillator_nodes[i].previous_index = UNOCCUPIED_OSCILLATOR;
			p_oscillator_node_pool->oscillator_nodes[i].next_index = UNOCCUPIED_OSCILLATOR;
		}
	}

	s_occupied_oscillator_node_head_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_node_last_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_node_number = 0;
}

/**********************************************************************************/
static void release_all_oscillator_nodes()
{
#ifdef _USE_STATIC_RESOURCE_ALLOCATION
	mark_all_oscillator_nodes_unused();
#else
	for(int j = 0; j < s_number_of_oscillator_node_pool; j++){
		chiptune_free(s_oscillator_node_pool_pointer_table[j]);
		s_oscillator_node_pool_pointer_table[j] = NULL;
	}
	s_number_of_oscillator_node_pool = 0;

	s_occupied_oscillator_node_head_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_node_last_index = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_node_number = 0;
#endif
}

#ifdef _ENABLE_CHECK_OCCUPIED_OSCILLATOR_LIST
/**********************************************************************************/
static int check_occupied_oscillator_node_list(void)
{
	int ret = 0;
	do {
		if(0 > s_occupied_oscillator_node_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_node_number = %d\r\n",
							s_occupied_oscillator_node_number);
			ret = -1;
			break;
		}

		if(0 == s_occupied_oscillator_node_number){
			if(UNOCCUPIED_OSCILLATOR != s_occupied_oscillator_node_head_index){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_node_number = 0"
											 " but s_occupied_oscillator_node_head_index is not UNOCCUPIED_OSCILLATOR\r\n");
				ret = -2;
			}
			break;
		}

		int16_t current_index;
		int16_t counter;

		current_index = s_occupied_oscillator_node_head_index;
		counter= 1;
		while(UNOCCUPIED_OSCILLATOR != get_oscillator_node_address_from_index(current_index)->next_index)
		{
			counter += 1;
			current_index = get_oscillator_node_address_from_index(current_index)->next_index;
		}
		if(counter != s_occupied_oscillator_node_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: FORWARDING occupied_oscillator_nodes list length = %d"
							", not matches s_occupied_oscillator_node_number = %d\r\n", counter, s_occupied_oscillator_node_number);
			ret = -3;
			break;
		}

		current_index = s_occupied_oscillator_node_last_index;
		counter = 1;
		while(UNOCCUPIED_OSCILLATOR != get_oscillator_node_address_from_index(current_index)->previous_index)
		{
			counter += 1;
			current_index = get_oscillator_node_address_from_index(current_index)->previous_index;
		}

		if(counter != s_occupied_oscillator_node_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: BACKWARDING occupied_oscillator_nodes list length = %d"
							", not matches s_occupied_oscillator_node_number = %d\r\n", counter, s_occupied_oscillator_node_number);

			ret = -4;
			break;
		}
	} while(0);
	return ret;
}
#endif

/**********************************************************************************/

typedef struct _midi_effect_association_link
{
	uint8_t midi_effect_type;
	int16_t associate_oscillator_indexes[SINGLE_EFFECT_MAX_ASSOCIATE_OSCILLATOR_NUMBER];
} midi_effect_association_link_t;

typedef struct _midi_effect_association_link_node
{
	midi_effect_association_link_t midi_effect_association_link;
	int16_t next_node_index;
} midi_effect_association_link_node_t;

#define NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX				(-1)
#define IS_MIDI_EFFECT_TYPE_MATCHED(TYPE_A, TYPE_B)	( ((TYPE_A) & (TYPE_B)) ? true : false)

#if 1
	#define MIDI_EFFECT_ASSOCIATION_LINK_NODE_POOL_CAPACITY (256)
#else
	#define MIDI_EFFECT_ASSOCIATION_LINK_NODE_POOL_CAPACITY	(32)
#endif

typedef struct _midi_effect_association_link_node_pool
{
	midi_effect_association_link_node_t midi_effect_association_link_nodes[MIDI_EFFECT_ASSOCIATION_LINK_NODE_POOL_CAPACITY];
} midi_effect_association_link_node_pool_t;


#ifdef _USE_STATIC_RESOURCE_ALLOCATION
static midi_effect_association_link_node_pool_t	s_midi_effect_association_link_node_pool;
static midi_effect_association_link_node_pool_t *	const s_midi_effect_association_link_node_pool_pointer_table[1]
											= {&s_midi_effect_association_link_node_pool};
static int16_t const	s_number_of_midi_effect_association_link_node_pool = 1;
#else
static midi_effect_association_link_node_pool_t *	s_midi_effect_association_link_node_pool_pointer_table[(INT16_MAX + 1)/MIDI_EFFECT_ASSOCIATION_LINK_NODE_POOL_CAPACITY];
static int16_t			s_number_of_midi_effect_association_link_node_pool = 0;
#endif

int16_t s_used_midi_effect_association_link_node_number = 0;

/**********************************************************************************/
static inline int16_t const get_midi_effect_association_link_node_capacity()
{
	return s_number_of_midi_effect_association_link_node_pool * MIDI_EFFECT_ASSOCIATION_LINK_NODE_POOL_CAPACITY;
}

/**********************************************************************************/
static midi_effect_association_link_node_t * const get_midi_effect_association_link_node_from_index(
		int16_t const node_index)
{
	return &s_midi_effect_association_link_node_pool_pointer_table[
			node_index / MIDI_EFFECT_ASSOCIATION_LINK_NODE_POOL_CAPACITY]
				->midi_effect_association_link_nodes[node_index % MIDI_EFFECT_ASSOCIATION_LINK_NODE_POOL_CAPACITY];
}

/**********************************************************************************/

#ifdef _USE_STATIC_RESOURCE_ALLOCATION
/**********************************************************************************/
static inline bool is_append_to_midi_effect_association_link_node_pool_successfully(void){ return false; }
#else

/**********************************************************************************/
static int allocate_and_append_midi_effect_association_link_node_pool(void)
{
	int ret = 0;
	do
	{
		midi_effect_association_link_node_pool_t * const p_new_midi_effect_association_link_node_pool
			= (midi_effect_association_link_node_pool_t*)chiptune_malloc(1 * sizeof(midi_effect_association_link_node_pool_t));

		if(NULL == p_new_midi_effect_association_link_node_pool){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: allocate midi_effect_association_link_node_pool_t fail\r\n");
			ret = -1;
			break;
		}
		for(int16_t i = 0; i < MIDI_EFFECT_ASSOCIATION_LINK_NODE_POOL_CAPACITY; i++){
			p_new_midi_effect_association_link_node_pool
					->midi_effect_association_link_nodes[i]
							.midi_effect_association_link.midi_effect_type = MidiEffectNone;
		}

		s_midi_effect_association_link_node_pool_pointer_table[s_number_of_midi_effect_association_link_node_pool]
				= p_new_midi_effect_association_link_node_pool;
		s_number_of_midi_effect_association_link_node_pool += 1;
	}while(0);

	return ret;
}

/**********************************************************************************/
static inline bool is_append_to_midi_effect_association_link_node_pool_successfully(void)
{
	int ret = true;
	do
	{
		if(INT16_MAX == s_used_midi_effect_association_link_node_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_used_midi_effect_association_link_node_number"
										 " reaches the CAP INT16_MAX\r\n");
			ret = false;
			break;
		}
		if(0 > allocate_and_append_midi_effect_association_link_node_pool()){
			ret = false;
			break;
		}
	}while(0);
	return ret;
}
#endif

/**********************************************************************************/
static bool is_unused_midi_effect_association_link_node_available()
{
	bool ret = true;
	if(get_midi_effect_association_link_node_capacity() == s_used_midi_effect_association_link_node_number){
		ret = is_append_to_midi_effect_association_link_node_pool_successfully();
	}
	return ret;
}

/**********************************************************************************/
static void mark_all_midi_effect_association_link_nodes_unused(void)
{
	for(int16_t j = 0; j < s_number_of_midi_effect_association_link_node_pool; j++){
		midi_effect_association_link_node_pool_t * const p_midi_effect_association_link_node_pool
				= s_midi_effect_association_link_node_pool_pointer_table[j];
		for(int16_t i = 0; i < MIDI_EFFECT_ASSOCIATION_LINK_NODE_POOL_CAPACITY; i++){
			p_midi_effect_association_link_node_pool->midi_effect_association_link_nodes[i]
					.midi_effect_association_link.midi_effect_type = MidiEffectNone;
		}
	}
	s_used_midi_effect_association_link_node_number = 0;
}

/**********************************************************************************/
static void release_all_midi_effect_association_link_nodes(void)
{
#ifdef _USE_STATIC_RESOURCE_ALLOCATION
	mark_all_midi_effect_association_link_nodes_unused();
#else
	for(int16_t j = 0; j < s_number_of_midi_effect_association_link_node_pool; j++){
		chiptune_free(s_midi_effect_association_link_node_pool_pointer_table[j]);
		s_midi_effect_association_link_node_pool_pointer_table[j] = NULL;
	}
	s_number_of_midi_effect_association_link_node_pool = 0;
	s_used_midi_effect_association_link_node_number = 0;
#endif
}

/**********************************************************************************/
static midi_effect_association_link_node_t * acquire_midi_effect_association_link_node(
		int16_t * const p_midi_effect_association_link_node_index)
{
	*p_midi_effect_association_link_node_index = NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX;
	if(false == is_unused_midi_effect_association_link_node_available()){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: all midi_effect_association_link_node are used\r\n");
		return NULL;
	}

	int16_t i;
	for(i = 0; i < get_midi_effect_association_link_node_capacity(); i++){
		if(MidiEffectNone == get_midi_effect_association_link_node_from_index(i)
				->midi_effect_association_link.midi_effect_type){
			break;
		}
	}

	midi_effect_association_link_node_t * p_midi_effect_association_link_node = NULL;
	do
	{
		if(get_midi_effect_association_link_node_capacity() == i){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: available midi_effect_association_link_node is not found\r\n");
			break;
		}
		p_midi_effect_association_link_node = get_midi_effect_association_link_node_from_index(i);
		s_used_midi_effect_association_link_node_number += 1;
		*p_midi_effect_association_link_node_index = i;
		memset(p_midi_effect_association_link_node, 0, sizeof(midi_effect_association_link_node_t));
		p_midi_effect_association_link_node->next_node_index = NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX;
	} while(0);

	return p_midi_effect_association_link_node;
}

/**********************************************************************************/
static midi_effect_association_link_node_t * append_midi_effect_association_link_node(
		oscillator_t * const p_oscillator)
{
	int16_t new_node_index = NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX;
	midi_effect_association_link_node_t * const p_new_node
			= acquire_midi_effect_association_link_node(&new_node_index);
	do
	{
		if(NULL == p_new_node){
			break;
		}
		p_new_node->next_node_index = NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX;

		int16_t current_node_index = p_oscillator->midi_effect_association_link_node_index;
		if(NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX == current_node_index){
			p_oscillator->midi_effect_association_link_node_index = new_node_index;
			break;
		}

		while(1)
		{
			midi_effect_association_link_node_t * p_current_node
					= get_midi_effect_association_link_node_from_index(current_node_index);
			if(NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX == p_current_node->next_node_index){
				p_current_node->next_node_index = new_node_index;
				break;
			}
			current_node_index = p_current_node->next_node_index;
		}
	} while(0);

	return p_new_node;
}

/**********************************************************************************/
static void discard_midi_effect_association_link_nodes(oscillator_t * const p_oscillator)
{
	int16_t midi_effect_association_link_node_index = p_oscillator->midi_effect_association_link_node_index;
	while(NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX != midi_effect_association_link_node_index)
	{
		midi_effect_association_link_node_t * const p_midi_effect_association_link_node
				= get_midi_effect_association_link_node_from_index(midi_effect_association_link_node_index);
		int16_t const next_node_index = p_midi_effect_association_link_node->next_node_index;
		p_midi_effect_association_link_node->midi_effect_association_link.midi_effect_type = MidiEffectNone;
		p_midi_effect_association_link_node->next_node_index = NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX;
		s_used_midi_effect_association_link_node_number -= 1;
		midi_effect_association_link_node_index = next_node_index;
	}
	p_oscillator->midi_effect_association_link_node_index = NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX;
}

/**********************************************************************************/

#define NO_PHASER_FILTER_STATE_INDEX				(-1)
#define PHASER_FILTER_STATE_UNUSED					(UINT16_MAX)
#define SET_PHASER_FILTER_STATE_UNUSED(PHASER_FILTER_STATE_POINTER) \
	((PHASER_FILTER_STATE_POINTER)->table_index = PHASER_FILTER_STATE_UNUSED)
#define IS_PHASER_FILTER_STATE_UNUSED(PHASER_FILTER_STATE_POINTER) \
	(PHASER_FILTER_STATE_UNUSED == (PHASER_FILTER_STATE_POINTER)->table_index)

#define PHASER_FILTER_STATE_POOL_CAPACITY			(256)

typedef struct _phaser_filter_state_pool
{
	phaser_filter_state_t phaser_filter_states[PHASER_FILTER_STATE_POOL_CAPACITY];
} phaser_filter_state_pool_t;

#ifdef _USE_STATIC_RESOURCE_ALLOCATION
static phaser_filter_state_pool_t	s_phaser_filter_state_pool;
static phaser_filter_state_pool_t *	const s_phaser_filter_state_pool_pointer_table[1]
											= {&s_phaser_filter_state_pool};
static int16_t const	s_number_of_phaser_filter_state_pool = 1;
#else
static phaser_filter_state_pool_t *	s_phaser_filter_state_pool_pointer_table[(INT16_MAX + 1)/PHASER_FILTER_STATE_POOL_CAPACITY];
static int16_t			s_number_of_phaser_filter_state_pool = 0;
#endif

int16_t s_used_phaser_filter_state_number = 0;

/**********************************************************************************/
static void mark_phaser_filter_state_unused(phaser_filter_state_t * const p_phaser_filter)
{
	SET_PHASER_FILTER_STATE_UNUSED(p_phaser_filter);
}

/**********************************************************************************/
static inline int16_t const get_phaser_filter_state_capacity()
{
	return s_number_of_phaser_filter_state_pool * PHASER_FILTER_STATE_POOL_CAPACITY;
}

/**********************************************************************************/
static phaser_filter_state_t * const get_phaser_filter_state_from_index(
		int16_t const phaser_filter_state_index)
{
	return &s_phaser_filter_state_pool_pointer_table[
			phaser_filter_state_index / PHASER_FILTER_STATE_POOL_CAPACITY]
				->phaser_filter_states[phaser_filter_state_index % PHASER_FILTER_STATE_POOL_CAPACITY];
}

/**********************************************************************************/

#ifdef _USE_STATIC_RESOURCE_ALLOCATION
/**********************************************************************************/
static inline bool is_append_to_phaser_filter_state_pool_successfully(void){ return false; }
#else

/**********************************************************************************/
static int allocate_and_append_phaser_filter_state_pool(void)
{
	int ret = 0;
	do
	{
		phaser_filter_state_pool_t * const p_new_phaser_filter_pool
			= (phaser_filter_state_pool_t*)chiptune_malloc(1 * sizeof(phaser_filter_state_pool_t));

		if(NULL == p_new_phaser_filter_pool){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: allocate phaser_filter_state_pool_t fail\r\n");
			ret = -1;
			break;
		}
		for(int16_t i = 0; i < PHASER_FILTER_STATE_POOL_CAPACITY; i++){
			mark_phaser_filter_state_unused(&p_new_phaser_filter_pool->phaser_filter_states[i]);
		}

		s_phaser_filter_state_pool_pointer_table[s_number_of_phaser_filter_state_pool]
				= p_new_phaser_filter_pool;
		s_number_of_phaser_filter_state_pool += 1;
	}while(0);

	return ret;
}

/**********************************************************************************/
static inline bool is_append_to_phaser_filter_state_pool_successfully(void)
{
	int ret = true;
	do
	{
		if(INT16_MAX == s_used_phaser_filter_state_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_used_phaser_filter_state_number"
										 " reaches the CAP INT16_MAX\r\n");
			ret = false;
			break;
		}
		if(0 > allocate_and_append_phaser_filter_state_pool()){
			ret = false;
			break;
		}
	}while(0);
	return ret;
}
#endif

/**********************************************************************************/
static bool is_unused_phaser_filter_state_available()
{
	bool ret = true;
	if(get_phaser_filter_state_capacity() == s_used_phaser_filter_state_number){
		ret = is_append_to_phaser_filter_state_pool_successfully();
	}
	return ret;
}

/**********************************************************************************/
static void mark_all_phaser_filter_states_unused(void)
{
	for(int16_t j = 0; j < s_number_of_phaser_filter_state_pool; j++){
		phaser_filter_state_pool_t * const p_phaser_filter_state_pool
				= s_phaser_filter_state_pool_pointer_table[j];
		for(int16_t i = 0; i < PHASER_FILTER_STATE_POOL_CAPACITY; i++){
			mark_phaser_filter_state_unused(&p_phaser_filter_state_pool->phaser_filter_states[i]);
		}
	}
	s_used_phaser_filter_state_number = 0;
}

/**********************************************************************************/
static void release_all_phaser_filter_states(void)
{
#ifdef _USE_STATIC_RESOURCE_ALLOCATION
	mark_all_phaser_filter_states_unused();
#else
	for(int16_t j = 0; j < s_number_of_phaser_filter_state_pool; j++){
		chiptune_free(s_phaser_filter_state_pool_pointer_table[j]);
		s_phaser_filter_state_pool_pointer_table[j] = NULL;
	}
	s_number_of_phaser_filter_state_pool = 0;
	s_used_phaser_filter_state_number = 0;
#endif
}

/**********************************************************************************/
static phaser_filter_state_t * acquire_phaser_filter_state_index(int16_t * const p_phaser_filter_state_index)
{
	*p_phaser_filter_state_index = NO_PHASER_FILTER_STATE_INDEX;
	if(false == is_unused_phaser_filter_state_available()){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: all phaser_filters are used\r\n");
		return NULL;
	}

	int16_t i;
	for(i = 0; i < get_phaser_filter_state_capacity(); i++){
		if(true == IS_PHASER_FILTER_STATE_UNUSED(get_phaser_filter_state_from_index(i))){
			break;
		}
	}

	phaser_filter_state_t * p_phaser_filter = NULL;
	do
	{
		if(get_phaser_filter_state_capacity() == i){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: available phaser_filter is not found\r\n");
			break;
		}
		p_phaser_filter = get_phaser_filter_state_from_index(i);
		s_used_phaser_filter_state_number += 1;
		*p_phaser_filter_state_index = i;
		memset(p_phaser_filter, 0, sizeof(phaser_filter_state_t));
	} while(0);

	return p_phaser_filter;
}

/**********************************************************************************/
static int discard_phaser_filter_state_index(int16_t const phaser_filter_state_index)
{
	if(NO_PHASER_FILTER_STATE_INDEX == phaser_filter_state_index){
		return 0;
	}
	phaser_filter_state_t * const p_phaser_filter
			= get_phaser_filter_state_from_index(phaser_filter_state_index);
	if(true == IS_PHASER_FILTER_STATE_UNUSED(p_phaser_filter)){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: phaser_filter index = %d is not used \r\n",
						phaser_filter_state_index);
		return -2;
	}
	mark_phaser_filter_state_unused(p_phaser_filter);
	s_used_phaser_filter_state_number -= 1;
	return 0;
}

/**********************************************************************************/
int attach_phaser_filter_state(oscillator_t * const p_oscillator)
{
	int ret = 0;
	do
	{
		if(MidiEffectPhaser != p_oscillator->midi_effect_association){
			ret = -1;
			break;
		}
		if(NO_PHASER_FILTER_STATE_INDEX != p_oscillator->phaser_filter_state_index){
			ret = -2;
			break;
		}
		if(NULL == acquire_phaser_filter_state_index(&p_oscillator->phaser_filter_state_index)){
			ret = -3;
			break;
		}
	} while(0);

	return ret;
}

/**********************************************************************************/
phaser_filter_state_t * get_phaser_filter_state_pointer(oscillator_t * p_oscillator)
{
	if(NO_PHASER_FILTER_STATE_INDEX == p_oscillator->phaser_filter_state_index){
		return NULL;
	}

	if(0 > p_oscillator->phaser_filter_state_index
			|| get_phaser_filter_state_capacity() <= p_oscillator->phaser_filter_state_index){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: phaser_filter index = %d, out of range \r\n",
						p_oscillator->phaser_filter_state_index);
		return NULL;
	}

	phaser_filter_state_t * const p_phaser_filter
			= get_phaser_filter_state_from_index(p_oscillator->phaser_filter_state_index);
	if(true == IS_PHASER_FILTER_STATE_UNUSED(p_phaser_filter)){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: phaser_filter index = %d is not used \r\n",
						p_oscillator->phaser_filter_state_index);
		return NULL;
	}

	return p_phaser_filter;
}

/**********************************************************************************/
static oscillator_node_t * occupy_one_oscillator_node(int16_t * const p_oscillator_index)
{
	*p_oscillator_index = UNOCCUPIED_OSCILLATOR;

	oscillator_node_t * p_oscillator_node = NULL;
	int16_t oscillator_index;
	for(oscillator_index = 0; oscillator_index < get_oscillator_node_capacity(); oscillator_index++){
		if(UNOCCUPIED_OSCILLATOR == get_oscillator_node_address_from_index(oscillator_index)->oscillator.voice){
			p_oscillator_node = get_oscillator_node_address_from_index(oscillator_index);
			break;
		}
	}

	if(NULL == p_oscillator_node){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: available oscillator_node is not found\r\n");
		return NULL;
	}

	do {
		if(0 == s_occupied_oscillator_node_number){
			p_oscillator_node->previous_index = UNOCCUPIED_OSCILLATOR;
			p_oscillator_node->next_index = UNOCCUPIED_OSCILLATOR;
			s_occupied_oscillator_node_head_index = oscillator_index;
			break;
		}

		get_oscillator_node_address_from_index(s_occupied_oscillator_node_last_index)->next_index
				= oscillator_index;
		p_oscillator_node->previous_index = s_occupied_oscillator_node_last_index;
		p_oscillator_node->next_index = UNOCCUPIED_OSCILLATOR;
	} while(0);
	s_occupied_oscillator_node_last_index = oscillator_index;
	s_occupied_oscillator_node_number += 1;
	*p_oscillator_index = oscillator_index;
#ifdef _ENABLE_CHECK_OCCUPIED_OSCILLATOR_LIST
	check_occupied_oscillator_node_list();
#endif
	return p_oscillator_node;
}

/**********************************************************************************/
oscillator_t * acquire_oscillator(int16_t * const p_oscillator_index)
{
	*p_oscillator_index = UNOCCUPIED_OSCILLATOR;
	if(false == is_unoccupied_oscillator_node_available()){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: all oscillators are used\r\n");
		return NULL;
	}

	oscillator_t * p_oscillator = NULL;
	do
	{
		oscillator_node_t * const p_oscillator_node = occupy_one_oscillator_node(p_oscillator_index);
		if(NULL == p_oscillator_node){
			break;
		}
		p_oscillator = &p_oscillator_node->oscillator;
		memset(p_oscillator, 0, sizeof(oscillator_t));
		p_oscillator->midi_effect_association_link_node_index = NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX;
		p_oscillator->phaser_filter_state_index = NO_PHASER_FILTER_STATE_INDEX;
	} while(0);

	return p_oscillator;
}

/**********************************************************************************/
oscillator_t * replicate_oscillator(int16_t const original_oscillator_index,
										  int16_t * const p_replicated_oscillator_index)
{
	oscillator_t * const p_replicated_oscillator = acquire_oscillator(p_replicated_oscillator_index);
	do
	{
		if(NULL == p_replicated_oscillator){
			break;
		}
		oscillator_t * const p_original_oscillator
				= &get_oscillator_node_address_from_index(original_oscillator_index)->oscillator;
		memcpy(p_replicated_oscillator, p_original_oscillator, sizeof(oscillator_t));
		p_replicated_oscillator->midi_effect_association_link_node_index = NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX;
		p_replicated_oscillator->phaser_filter_state_index = NO_PHASER_FILTER_STATE_INDEX;
	}while(0);

	return p_replicated_oscillator;
}

/**********************************************************************************/
static int vacate_occupied_oscillator_node(int16_t const oscillator_index)
{
	oscillator_node_t * const p_oscillator_node
			= get_oscillator_node_address_from_index(oscillator_index);
	int16_t const previous_index = p_oscillator_node->previous_index;
	int16_t const next_index = p_oscillator_node->next_index;

	do {
		if(0 == s_occupied_oscillator_node_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: all oscillators have been discarded\r\n");
			return -1;
		}

		if(1 != s_occupied_oscillator_node_number){
			if(UNOCCUPIED_OSCILLATOR == previous_index && UNOCCUPIED_OSCILLATOR == next_index){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: oscillator %d is not on the occupied list\r\n", oscillator_index);
				return -2;
			}
		}

		if(1 == s_occupied_oscillator_node_number){
			s_occupied_oscillator_node_head_index = UNOCCUPIED_OSCILLATOR;
			s_occupied_oscillator_node_last_index = UNOCCUPIED_OSCILLATOR;
			break;
		}

		if(oscillator_index == s_occupied_oscillator_node_head_index){
			s_occupied_oscillator_node_head_index = next_index;
			get_oscillator_node_address_from_index(s_occupied_oscillator_node_head_index)->previous_index
					= UNOCCUPIED_OSCILLATOR;
			break;
		}

		if(oscillator_index == s_occupied_oscillator_node_last_index){
			s_occupied_oscillator_node_last_index = previous_index;
			get_oscillator_node_address_from_index(s_occupied_oscillator_node_last_index)->next_index
					= UNOCCUPIED_OSCILLATOR;
			break;
		}

		get_oscillator_node_address_from_index(previous_index)->next_index = next_index;
		get_oscillator_node_address_from_index(next_index)->previous_index = previous_index;
	} while (0);
	p_oscillator_node->previous_index = UNOCCUPIED_OSCILLATOR;
	p_oscillator_node->next_index = UNOCCUPIED_OSCILLATOR;
	p_oscillator_node->oscillator.voice = UNOCCUPIED_OSCILLATOR;
	s_occupied_oscillator_node_number -= 1;
#ifdef _ENABLE_CHECK_OCCUPIED_OSCILLATOR_LIST
	check_occupied_oscillator_node_list();
#endif
	return 0;
}

/**********************************************************************************/
int discard_oscillator(int16_t const oscillator_index)
{
	oscillator_t * const p_oscillator = &get_oscillator_node_address_from_index(oscillator_index)->oscillator;
	bool const is_percussion_oscillator = IS_PERCUSSION_OSCILLATOR(p_oscillator);
	int ret = 0;
	do
	{
		ret = vacate_occupied_oscillator_node(oscillator_index);
		if(0 != ret){
			break;
		}

		if(false == is_percussion_oscillator){
			discard_midi_effect_association_link_nodes(p_oscillator);
			if(NO_PHASER_FILTER_STATE_INDEX != p_oscillator->phaser_filter_state_index){
				discard_phaser_filter_state_index(p_oscillator->phaser_filter_state_index);
				p_oscillator->phaser_filter_state_index = NO_PHASER_FILTER_STATE_INDEX;
			}
		}
	} while(0);

	return ret;
}

/**********************************************************************************/
int16_t get_occupied_oscillator_number(void)
{
	return s_occupied_oscillator_node_number;
}

/**********************************************************************************/
int16_t get_occupied_oscillator_head_index()
{
	if(-1 == s_occupied_oscillator_node_head_index
			&& 0 != s_occupied_oscillator_node_number){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_occupied_oscillator_node_head_index = -1:: but s_occupied_oscillator_node_number = %d\r\n",
						s_occupied_oscillator_node_number);
	}
	return s_occupied_oscillator_node_head_index;
}

/**********************************************************************************/
static inline bool is_occupied_oscillator_node_index_out_of_range(int16_t const oscillator_index)
{
	bool is_out_of_range = true;
	do {
		if(oscillator_index < 0){
			break;
		}
		if(oscillator_index >= get_oscillator_node_capacity()){
			break;
		}
		is_out_of_range = false;
	}while(0);

	return is_out_of_range;
}

/**********************************************************************************/

int16_t get_occupied_oscillator_next_index(int16_t const oscillator_index)
{
	if(true == is_occupied_oscillator_node_index_out_of_range(oscillator_index)){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: oscillator index = %d, out of range \r\n", oscillator_index);
		return UNOCCUPIED_OSCILLATOR;
	}

	return get_oscillator_node_address_from_index(oscillator_index)->next_index;
}

/**********************************************************************************/

oscillator_t * get_oscillator_pointer_from_index(int16_t const oscillator_index)
{
	if(true == is_occupied_oscillator_node_index_out_of_range(oscillator_index)){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: oscillator index = %d, out of range \r\n", oscillator_index);
		return NULL;
	}

	return &get_oscillator_node_address_from_index(oscillator_index)->oscillator;
}

/**********************************************************************************/
void clear_all_oscillators(void)
{
	mark_all_midi_effect_association_link_nodes_unused();
	mark_all_phaser_filter_states_unused();
	mark_all_oscillator_nodes_unused();
}

/**********************************************************************************/
void destroy_all_oscillators(void)
{
	release_all_midi_effect_association_link_nodes();
	release_all_phaser_filter_states();
	release_all_oscillator_nodes();
}

/**********************************************************************************/

static inline int16_t get_single_effect_associate_number(uint8_t const midi_effect_type)
{
	int16_t associate_number = SINGLE_EFFECT_MAX_ASSOCIATE_OSCILLATOR_NUMBER;
	do
	{
		if(MidiEffectReverb == midi_effect_type){
			associate_number = REVERB_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER;
			break;
		}
		if(MidiEffectChorus == midi_effect_type){
			associate_number = CHORUS_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER;
			break;
		}
		if(MidiEffectDetune == midi_effect_type){
			associate_number = DETUNE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER;
			break;
		}
		if(MidiEffectPhaser == midi_effect_type){
			associate_number = PHASER_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER;
			break;
		}
	}while(0);

	return associate_number;
}

/**********************************************************************************/

int store_associate_oscillator_indexes(uint8_t const midi_effect_type, int16_t const primary_oscillator_index,
									  int16_t const * const p_associate_oscillator_indexes)
{
	int ret = 0;
	oscillator_t  * const p_primary_oscillator
			= &get_oscillator_node_address_from_index(primary_oscillator_index)->oscillator;
	do
	{
		if(NULL == p_primary_oscillator){
			ret = -1;
			break;
		}

		if(MidiEffectAll == midi_effect_type || MidiEffectNone == midi_effect_type){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: %s :: midi_effect_type could not be"
										 " MidiEffectAll or MidiEffectNone\r\n", __func__);
			ret = -2;
			break;
		}

		midi_effect_association_link_node_t * const p_new_node
				= append_midi_effect_association_link_node(p_primary_oscillator);
		if(NULL == p_new_node){
			break;
		}
		p_new_node->midi_effect_association_link.midi_effect_type = midi_effect_type;
		int16_t const associate_oscillator_number = get_single_effect_associate_number(midi_effect_type);
		for(int16_t i = 0; i < associate_oscillator_number; i++){
			p_new_node->midi_effect_association_link.associate_oscillator_indexes[i]
					= p_associate_oscillator_indexes[i];
		}
	}while(0);

	return ret;
}

/**********************************************************************************/

int16_t calculate_all_subordinate_oscillator_number(uint8_t const midi_effect_type, int16_t const root_oscillator_index)
{
	return collect_subordinate_oscillator_indexes(midi_effect_type, root_oscillator_index, NULL);
}

/**********************************************************************************/

int collect_subordinate_oscillator_indexes(uint8_t const midi_effect_type, int16_t const root_oscillator_index,
										   int16_t * const p_subordinate_indexes)
{
	oscillator_t * const p_root_oscillator
			= &get_oscillator_node_address_from_index(root_oscillator_index)->oscillator;
	int16_t subordinate_number = 0;
	do{
		if(NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX == p_root_oscillator->midi_effect_association_link_node_index){
			break;
		}
		int16_t midi_effect_association_link_node_index = p_root_oscillator->midi_effect_association_link_node_index;

		while(1)
		{
			midi_effect_association_link_node_t * const p_midi_effect_association_link_node
					= get_midi_effect_association_link_node_from_index(midi_effect_association_link_node_index);
			midi_effect_association_link_t const * const p_midi_effect_association_link
					= &p_midi_effect_association_link_node->midi_effect_association_link;
			if(MidiEffectNone == p_midi_effect_association_link->midi_effect_type){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: midi_effect_association_link_node index = %d is labeled as MidiEffectNone\r\n",
								midi_effect_association_link_node_index);
				break;
			}

			int16_t const associate_oscillator_number
					= get_single_effect_associate_number(p_midi_effect_association_link->midi_effect_type);
			do
			{
				if(false == IS_MIDI_EFFECT_TYPE_MATCHED(midi_effect_type,
													   p_midi_effect_association_link->midi_effect_type)){
					break;
				}

				if(NULL != p_subordinate_indexes){
					for(int16_t i = 0; i < associate_oscillator_number; i++){
						p_subordinate_indexes[subordinate_number + i]
								= p_midi_effect_association_link->associate_oscillator_indexes[i];
					}
				}
				subordinate_number += associate_oscillator_number;
			} while(0);
			for(int16_t i = 0; i < associate_oscillator_number; i++){
				int16_t * p_moving_subordinate_indexes
						= p_subordinate_indexes + subordinate_number * ( NULL != p_subordinate_indexes);
				subordinate_number +=
						collect_subordinate_oscillator_indexes(midi_effect_type,
															   p_midi_effect_association_link->associate_oscillator_indexes[i],
															   p_moving_subordinate_indexes);
			}

			if(NO_MIDI_EFFECT_ASSOCIATION_LINK_NODE_INDEX == p_midi_effect_association_link_node->next_node_index){
				break;
			}
			midi_effect_association_link_node_index = p_midi_effect_association_link_node->next_node_index;
		}
	}while(0);

	return subordinate_number;
}

// NOLINTEND(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
