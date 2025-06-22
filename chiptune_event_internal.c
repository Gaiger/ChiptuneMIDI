#include <stdio.h>
#include  <string.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_oscillator_internal.h"

#include "chiptune_event_internal.h"

#ifdef _FIXED_OSCILLATOR_AND_EVENT_CAPACITY
    #define OCCUPIABLE_OSCILLATOR_CAPACITY      (512)
#else
    #define OSCILLATOR_POOL_CAPACITY            (64)
#endif

typedef struct _oscillator_link
{
	int16_t previous;
	int16_t next;
} oscillator_link_t;

#ifdef _FIXED_OSCILLATOR_AND_EVENT_CAPACITY
static oscillator_t s_oscillators[OCCUPIABLE_OSCILLATOR_CAPACITY];
static oscillator_link_t s_oscillator_links[OCCUPIABLE_OSCILLATOR_CAPACITY];
#else
typedef struct _oscillator_pool
{
    oscillator_t oscillators[OSCILLATOR_POOL_CAPACITY];
    oscillator_link_t oscillator_links[OSCILLATOR_POOL_CAPACITY];
    struct _oscillator_pool *p_next;
} oscillator_pool_t;

static oscillator_pool_t *s_p_oscillator_pool_link_head = NULL;
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

static inline oscillator_link_t * const get_oscillator_link_pointer_from_index(int16_t const index)
{
    return &s_oscillator_links[index];
}

/**********************************************************************************/

static inline oscillator_t * const get_oscillator_pointer_from_index(int16_t const index)
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

static int reset_all_oscillators_and_links(void)
{
    for(int16_t i = 0; i < get_occupiable_oscillator_capacity(); i++){
        get_oscillator_pointer_from_index(i)->voice = UNOCCUPIED_OSCILLATOR;
        oscillator_link_t * const p_oscillator_link = get_oscillator_link_pointer_from_index(i);
        p_oscillator_link->previous = UNOCCUPIED_OSCILLATOR;
        p_oscillator_link->next = UNOCCUPIED_OSCILLATOR;
    }
    s_occupied_oscillator_head_index = UNOCCUPIED_OSCILLATOR;
    s_occupied_oscillator_last_index = UNOCCUPIED_OSCILLATOR;
    s_occupied_oscillator_number = 0;
}

#else
/**********************************************************************************/

static inline int16_t const get_occupiable_oscillator_capacity()
{
    return s_number_of_oscillator_pool * OSCILLATOR_POOL_CAPACITY;
}

/**********************************************************************************/

static oscillator_pool_t * const get_oscillator_pool_pointer_from_index(int16_t const index)
{

    oscillator_pool_t const * p_oscillator_pool = s_p_oscillator_pool_link_head;
    int16_t const pool_index = index/OSCILLATOR_POOL_CAPACITY;
    for(int16_t i = 0; i < pool_index; i++){
        p_oscillator_pool = p_oscillator_pool->p_next;
    }
    return (oscillator_pool_t * const)p_oscillator_pool;
}

/**********************************************************************************/

static oscillator_link_t * const get_oscillator_link_pointer_from_index(int16_t const index)
{
    if(get_occupiable_oscillator_capacity() <= index){
        CHIPTUNE_PRINTF(cDeveloping, "ERROR :: index out of bound : index = %d,"
                                     " get_occupiable_oscillator_capacity() = %d\r\n",
                        index, s_number_of_oscillator_pool * OSCILLATOR_POOL_CAPACITY);
        return NULL;
    }
    return &get_oscillator_pool_pointer_from_index(index)->oscillator_links[index % OSCILLATOR_POOL_CAPACITY];
}

/**********************************************************************************/

static inline oscillator_t * const get_oscillator_pointer_from_index(int16_t const index)
{
    if(get_occupiable_oscillator_capacity() <= index){
        CHIPTUNE_PRINTF(cDeveloping, "ERROR :: index out of bound : index = %d,"
                                     " get_occupiable_oscillator_capacity() = %d\r\n",
                        index, s_number_of_oscillator_pool * OSCILLATOR_POOL_CAPACITY);
        return NULL;
    }
    return &get_oscillator_pool_pointer_from_index(index)->oscillators[index % OSCILLATOR_POOL_CAPACITY];
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
            p_new_appending_oscillator_pool->p_next = NULL;
        }

        if(0 == s_number_of_oscillator_pool){
            s_p_oscillator_pool_link_head = p_new_appending_oscillator_pool;
            s_number_of_oscillator_pool = 1;
            break;
        }

        oscillator_pool_t *p_last_oscillator_pool = s_p_oscillator_pool_link_head;
        while(NULL != p_last_oscillator_pool->p_next){
            p_last_oscillator_pool = p_last_oscillator_pool->p_next;
        }
        p_last_oscillator_pool->p_next = p_new_appending_oscillator_pool;
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

static int reset_all_oscillators_and_links(void)
{
    oscillator_pool_t *p_oscillator_pool = s_p_oscillator_pool_link_head;

    while(1){
        if(NULL == p_oscillator_pool){
            break;
        }
        oscillator_pool_t *p_next = p_oscillator_pool->p_next;
        chiptune_free(p_oscillator_pool); p_oscillator_pool = NULL;
        p_oscillator_pool = p_next;
    }
    s_p_oscillator_pool_link_head = NULL;
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
        while(UNOCCUPIED_OSCILLATOR != get_oscillator_link_pointer_from_index(current_index)->next)
		{
            counter += 1;
            current_index = get_oscillator_link_pointer_from_index(current_index)->next;
		}
		if(counter != s_occupied_oscillator_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: FORWARDING occupied_oscillators list length = %d"
							", not matches s_occupied_oscillator_number = %d\r\n", counter, s_occupied_oscillator_number);
			ret = -3;
			break;
		}

		current_index = s_occupied_oscillator_last_index;
		counter = 1;
        while(UNOCCUPIED_OSCILLATOR != get_oscillator_link_pointer_from_index(current_index)->previous)
		{
			counter += 1;
            current_index = get_oscillator_link_pointer_from_index(current_index)->previous;
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
        oscillator_link_t * const p_this_index_oscillator_link = get_oscillator_link_pointer_from_index(index);
		if(0 == s_occupied_oscillator_number){
            p_this_index_oscillator_link->previous = UNOCCUPIED_OSCILLATOR;
            p_this_index_oscillator_link->next = UNOCCUPIED_OSCILLATOR;
			s_occupied_oscillator_head_index = index;
			break;
		}

        get_oscillator_link_pointer_from_index(s_occupied_oscillator_last_index)->next = index;
        p_this_index_oscillator_link->previous = s_occupied_oscillator_last_index;
        p_this_index_oscillator_link->next = UNOCCUPIED_OSCILLATOR;
	} while(0);
	s_occupied_oscillator_last_index = index;
	s_occupied_oscillator_number += 1;
	CHECK_OCCUPIED_OSCILLATOR_LIST();
	return 0;
}

/**********************************************************************************/

oscillator_t * const acquire_event_freed_oscillator(int16_t * const p_index)
{
    if(false == is_unoccupied_oscillator_available()){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR::all oscillators are used\r\n");
		return NULL;
	}
    for(int16_t i = 0; i < get_occupiable_oscillator_capacity(); i++){
        if(UNOCCUPIED_OSCILLATOR == get_oscillator_pointer_from_index(i)->voice){
			*p_index = i;
			occupy_oscillator(i);
            return get_oscillator_pointer_from_index(i);
		}
	}
	CHIPTUNE_PRINTF(cDeveloping, "ERROR::available oscillator is not found\r\n");
	*p_index = UNOCCUPIED_OSCILLATOR;
	return NULL;
}

/**********************************************************************************/

static int discard_oscillator(int16_t const index)
{
    oscillator_link_t * const p_this_index_oscillator_link = get_oscillator_link_pointer_from_index(index);
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
            get_oscillator_link_pointer_from_index(s_occupied_oscillator_head_index)->previous = UNOCCUPIED_OSCILLATOR;
			break;
		}

		if(index == s_occupied_oscillator_last_index){
			s_occupied_oscillator_last_index = previous_index;
            get_oscillator_link_pointer_from_index(s_occupied_oscillator_last_index)->next = UNOCCUPIED_OSCILLATOR;
			break;
		}

        get_oscillator_link_pointer_from_index(previous_index)->next = next_index;
        get_oscillator_link_pointer_from_index(next_index)->previous = previous_index;
	} while (0);

    p_this_index_oscillator_link->previous = UNOCCUPIED_OSCILLATOR;
    p_this_index_oscillator_link->next = UNOCCUPIED_OSCILLATOR;
    get_oscillator_pointer_from_index(index)->voice = UNOCCUPIED_OSCILLATOR;
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

static inline bool is_occupied_oscillator_index_out_of_range(int16_t const index)
{
    bool is_out_of_range = true;
    do
    {
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

int16_t const get_event_occupied_oscillator_next_index(int16_t const index)
{
    if(true == is_occupied_oscillator_index_out_of_range(index)){
		CHIPTUNE_PRINTF(cDeveloping, "oscillator index = %d, out of range \r\n", index);
		return UNOCCUPIED_OSCILLATOR;
	}

    return get_oscillator_link_pointer_from_index(index)->next;
}

/**********************************************************************************/

oscillator_t * const get_event_oscillator_pointer_from_index(int16_t const index)
{
    if(true == is_occupied_oscillator_index_out_of_range(index)){
		CHIPTUNE_PRINTF(cDeveloping, "oscillator index = %d, out of range \r\n", index);
		return NULL;
	}

    return get_oscillator_pointer_from_index(index);
}


/**********************************************************************************/
/**********************************************************************************/

#ifdef _FIXED_OSCILLATOR_AND_EVENT_CAPACITY
    #define QUEUABLE_EVENT_CAPACITY					(OCCUPIABLE_OSCILLATOR_CAPACITY * 3 / 2)
#endif
#define NO_EVENT									(-1)

enum
{
    UNUSED_EVENT =  -1,
    EVENT_DISCARD = (EVENT_TYPE_MAX + 1),
};

typedef struct _event
{
	int8_t	type;
	uint8_t : 8;
    int16_t oscillator_index;
	uint32_t triggering_tick;
    int16_t next_event_index;
} event_t;

#ifdef _FIXED_OSCILLATOR_AND_EVENT_CAPACITY
static event_t s_events[QUEUABLE_EVENT_CAPACITY];
#else

#define EVENT_POOL_CAPACITY                         (64)
typedef struct _event_pool
{
    event_t events[EVENT_POOL_CAPACITY];
    struct _event_pool *p_next;
} event_pool_t;

static event_pool_t *s_p_event_pool_link_head = NULL;

static int16_t s_number_of_event_pool = 0;
#endif
int16_t s_queued_event_number = 0;
int16_t s_queued_event_head_index = NO_EVENT;

#ifdef _FIXED_OSCILLATOR_AND_EVENT_CAPACITY
/**********************************************************************************/

static inline int16_t const get_queuable_event_capacity()
{
    return QUEUABLE_EVENT_CAPACITY;
}

/**********************************************************************************/

static inline event_t * const get_event_pointer_from_index(int16_t const index)
{
    return &s_events[index];
}

/**********************************************************************************/

static inline bool is_unqueued_event_available()
{
    if(get_queuable_event_capacity() == s_queued_event_number){
        return false;
    }
    return true;
}

/**********************************************************************************/

static inline int reset_all_events(void)
{
    for(int16_t i = 0; i < get_queuable_event_capacity(); i++){
        get_event_pointer_from_index(i)->type = UNUSED_EVENT;
    }
    s_queued_event_number = 0;
    return 0;
}
#else
/**********************************************************************************/

static inline int16_t const get_queuable_event_capacity()
{
    return s_number_of_event_pool * EVENT_POOL_CAPACITY;
}

/**********************************************************************************/

static event_t * const get_event_pointer_from_index(int16_t const index)
{
    if(get_queuable_event_capacity() < index){
        CHIPTUNE_PRINTF(cDeveloping, "ERROR :: index out of bound : index = %d,"
                                     " get_queuable_event_capacity() = %d\r\n",
                        index, get_queuable_event_capacity());
        return NULL;
    }

    event_pool_t *p_event_pool = s_p_event_pool_link_head;
    int16_t const pool_index = index/EVENT_POOL_CAPACITY;
    for(int16_t i = 0; i < pool_index; i++){
        p_event_pool = p_event_pool->p_next;
    }

    return &p_event_pool->events[index % EVENT_POOL_CAPACITY];
}

/**********************************************************************************/

static int append_new_event_pool(void)
{
    int ret = 0;
    do
    {
        event_pool_t *p_new_appending_event_pool
            = (event_pool_t*)chiptune_malloc(1 * sizeof(event_pool_t));

        if(NULL == p_new_appending_event_pool){
            CHIPTUNE_PRINTF(cDeveloping, "ERROR :: allocate event_pool_t fail\r\n");
            ret = -1;
            break;
        }
        for(int16_t i = 0; i < EVENT_POOL_CAPACITY; i++){
            p_new_appending_event_pool->events[i].type = UNUSED_EVENT;
        }
        p_new_appending_event_pool->p_next = NULL;
        if(0 == s_number_of_event_pool){
            s_p_event_pool_link_head = p_new_appending_event_pool;
            s_number_of_event_pool = 1;
            break;
        }

        event_pool_t *p_last_event_pool = s_p_event_pool_link_head;
        while(NULL != p_last_event_pool->p_next){
            p_last_event_pool = p_last_event_pool->p_next;
        }
        p_last_event_pool->p_next = p_new_appending_event_pool;
        s_number_of_event_pool += 1;
    }while(0);

    return ret;
}

/**********************************************************************************/

static bool is_unqueued_event_available()
{
    bool ret = true;
    if(get_queuable_event_capacity() == s_queued_event_number){
        do
        {
            if(INT16_MAX == s_queued_event_number){
                CHIPTUNE_PRINTF(cDeveloping, "ERROR :: s_queued_event_number"
                                             " reaches the CAP INT16_MAX\r\n");
                ret = false;
                break;
            }
            if(0 > append_new_event_pool()){
                ret = false;
                break;
            }
        }while(0);
    }
    return ret;
}

/**********************************************************************************/

static int reset_all_events(void)
{
    event_pool_t *p_event_pool = s_p_event_pool_link_head;

    while(1){
        if(NULL == p_event_pool){
            break;
        }
        event_pool_t *p_next = p_event_pool->p_next;
        chiptune_free(p_event_pool); p_event_pool = NULL;
        p_event_pool = p_next;
    }
    s_p_event_pool_link_head = NULL;
    s_number_of_event_pool = 0;

	s_queued_event_number = 0;
    return 0;
}
#endif

#ifdef _CHECK_EVENT_LIST
/**********************************************************************************/

static int check_queued_events(uint32_t const tick)
{
	int ret = 0;
    int16_t index = s_queued_event_head_index;
	do {
        if(NO_EVENT != s_queued_event_head_index){
			break;
		}
        if(0 != s_queued_event_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: tick = %u,"
                                         " event head is NO_EVENT but s_queued_event_number = %d\r\n",
                                        tick, s_queued_event_number);
			ret = -1;
			break;
		}
	} while(0);

	bool is_listing_error_occur = false;
	do {
		if(-1 == ret){
			break;
		}
		uint32_t previous_tick = 0;
        for(int16_t i = 0; i < s_queued_event_number; i++){
            event_t * const p_event = get_event_pointer_from_index(index);
            if(UNUSED_EVENT == p_event->type){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR:: event %d is UNUSED_EVENT but on the list\r\n", index);
				is_listing_error_occur = true;
			}
            if(previous_tick > p_event->triggering_tick){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR:: event is not in time order\r\n");
				is_listing_error_occur = true;
			}
            if(UNOCCUPIED_OSCILLATOR == p_event->oscillator_index){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR:: event %d oscillator is UNOCCUPIED_OSCILLATOR\r\n", index);
				is_listing_error_occur = true;
			}
            if(UNOCCUPIED_OSCILLATOR == get_event_oscillator_pointer_from_index(p_event->oscillator_index)->voice){
                CHIPTUNE_PRINTF(cDeveloping, "ERROR:: oscillator %d is UNOCCUPIED_OSCILLATOR\r\n",
                                p_event->oscillator_index);
				is_listing_error_occur = true;
			}

			if(true == is_listing_error_occur){
				break;
			}
            previous_tick = p_event->triggering_tick;
            index = p_event->next_event_index;
		}
	} while(0);

	do {
		if(false == is_listing_error_occur){
			break;
		}
		CHIPTUNE_PRINTF(cDeveloping, "tick = %u\r\n", tick);
        index = s_queued_event_head_index;
        for(int16_t i = 0; i < s_queued_event_number; i++){
            event_t * const p_event = get_event_pointer_from_index(index);
			CHIPTUNE_PRINTF(cDeveloping, "event = %d, type = %d, oscillator = %d, triggering_tick = %u\r\n",
                            index, p_event->type, p_event->oscillator_index, p_event->triggering_tick);
            index = p_event->next_event_index;
		}
		CHIPTUNE_PRINTF(cDeveloping, "-------------------------------------------------\r\n");
		CHIPTUNE_PRINTF(cDeveloping, "-------------------------------------------------\r\n");
		ret = -2;
	} while(0);

	return ret;
}
#define CHECK_QUEUED_EVENTS(TICK)					\
													do { \
                                                        check_queued_events((TICK)); \
													} while(0)

#else
#define CHECK_QUEUED_EVENTS(TICK)					\
													do { \
														(void)0; \
													} while(0)
#endif

/**********************************************************************************/

int put_event(int8_t const type, int16_t const oscillator_index, uint32_t const triggering_tick)
{
    if(false == is_unqueued_event_available()){
		CHIPTUNE_PRINTF(cDeveloping, "No unused event is available\r\n");
		return -1;
	}

	if(UNOCCUPIED_OSCILLATOR == oscillator_index){
		return 1;
	}

	do {
		int16_t current_index;
        for(current_index = 0; current_index < get_queuable_event_capacity(); current_index++){
            if(UNUSED_EVENT == get_event_pointer_from_index(current_index)->type){
				break;
			}
		}
        if(get_queuable_event_capacity() == current_index){
			CHIPTUNE_PRINTF(cDeveloping, "No available event is found\r\n");
			return -2;
		}

        event_t * const p_current_event = get_event_pointer_from_index(current_index);
        p_current_event->type = type;
        p_current_event->oscillator_index = oscillator_index;
        p_current_event->triggering_tick = triggering_tick;
        p_current_event->next_event_index = NO_EVENT;

        if(0 == s_queued_event_number){
            s_queued_event_head_index = current_index;
			break;
		}

        if(p_current_event->triggering_tick < get_event_pointer_from_index(s_queued_event_head_index)->triggering_tick){
            p_current_event->next_event_index = s_queued_event_head_index;
            s_queued_event_head_index = current_index;
			break;
		}

        int16_t previous_index = s_queued_event_head_index;
		int16_t kk;
        for(kk = 1; kk < s_queued_event_number; kk++){
            int16_t next_index = get_event_pointer_from_index(previous_index)->next_event_index;
            if(p_current_event->triggering_tick < get_event_pointer_from_index(next_index)->triggering_tick){
                get_event_pointer_from_index(previous_index)->next_event_index = current_index;
                p_current_event->next_event_index = next_index;
				break;
			}
			previous_index = next_index;
		}

        if(s_queued_event_number == kk){
            get_event_pointer_from_index(previous_index)->next_event_index = current_index;
		}
	} while(0);
    s_queued_event_number += 1;

    oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
	switch(type)
	{
	case EVENT_FREE:
		SET_PREPARE_TO_FREE(p_oscillator->state_bits);
		break;
	case EVENT_REST:
		SET_PREPARE_TO_REST(p_oscillator->state_bits);
		break;
	default:
		break;
	}

    CHECK_QUEUED_EVENTS(get_event_pointer_from_index(s_queued_event_head_index)->triggering_tick);
	return 0;
}

/**********************************************************************************/

static char s_event_additional_string[32];

static inline char const * const event_additional_string(int16_t const event_index)
{
    oscillator_t const * const p_oscillator
        = get_event_oscillator_pointer_from_index(get_event_pointer_from_index(event_index)->oscillator_index);
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
    while(NO_EVENT != s_queued_event_head_index)
	{
        event_t * const p_head_event = get_event_pointer_from_index(s_queued_event_head_index);
        if(p_head_event->triggering_tick > tick){
			break;
		}

        oscillator_t * const p_oscillator
            = get_event_oscillator_pointer_from_index(p_head_event->oscillator_index);
		channel_controller_t const * const p_channel_controller =
				get_channel_controller_pointer_from_index(p_oscillator->voice);

        int8_t const event_type = p_head_event->type;
		switch(event_type)
		{
		case EVENT_ACTIVATE:
			CHIPTUNE_PRINTF(cEventTriggering,
							"tick = %u, ACTIVATE oscillator = %d, voice = %d, note = %d, loudness = 0x%04x %s\r\n",
                            tick, p_head_event->oscillator_index,
							p_oscillator->voice, p_oscillator->note, p_oscillator->loudness,
                            event_additional_string(s_queued_event_head_index));
			if(true == IS_ACTIVATED(p_oscillator->state_bits)){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: activate an activated oscillator = %d\r\n",
                                p_head_event->oscillator_index);
				return -1;
			}
			SET_ACTIVATED(p_oscillator->state_bits);
			setup_envelope_state(p_oscillator, ENVELOPE_STATE_ATTACK);
			break;

		case EVENT_FREE:
			CHIPTUNE_PRINTF(cEventTriggering,
							"tick = %u, FREE oscillator = %d, voice = %d, note = %d, amplitude = %2.1f%% of loudness %s\r\n",
                            tick, p_head_event->oscillator_index,
							p_oscillator->voice, p_oscillator->note,
							100.0f * p_oscillator->release_reference_amplitude/(float)p_oscillator->loudness,
                            event_additional_string(s_queued_event_head_index));
			if(true == IS_FREEING(p_oscillator->state_bits)) {
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: free a freeing oscillator = %d\r\n",
                            p_head_event->oscillator_index);
				return -1;
			}
			SET_FREEING(p_oscillator->state_bits);
			do {
#if(0)
				if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == p_oscillator->voice){
                    put_event(EVENT_DISCARD, p_head_event->oscillator_index, tick);
					break;
				}
#endif
				/*It does not a matter there is a postponement to discard the resting oscillator*/
				setup_envelope_state(p_oscillator, ENVELOPE_STATE_RELEASE);
                put_event(EVENT_DISCARD, p_head_event->oscillator_index,
				tick + p_channel_controller->envelope_release_tick_number);
			} while(0);
			break;

		case EVENT_REST:
			CHIPTUNE_PRINTF(cEventTriggering,
                            "tick = %u, REST oscillator = %d, voice = %d, note = %d, amplitude = %2.1f%% of loudness %s\r\n",
                            tick, p_head_event->oscillator_index,
							p_oscillator->voice, p_oscillator->note,
							100.0f * p_oscillator->release_reference_amplitude/(float)p_oscillator->loudness,
                            event_additional_string(s_queued_event_head_index));
			if(true == IS_FREEING(p_oscillator->state_bits)
					&& true == IS_NATIVE_OSCILLATOR(p_oscillator->state_bits)){
                CHIPTUNE_PRINTF(cDeveloping, "WARNING :: rest a freeing native oscillator = %d\r\n",
                            p_head_event->oscillator_index);
				break;
			}
			if(true == IS_RESTING(p_oscillator->state_bits)
					&& true == IS_NATIVE_OSCILLATOR(p_oscillator->state_bits)){
                CHIPTUNE_PRINTF(cDeveloping, "WARNING :: rest a resting native oscillator = %d\r\n",
                            p_head_event->oscillator_index);
				break;
			}
			SET_RESTING(p_oscillator->state_bits);
			setup_envelope_state(p_oscillator, ENVELOPE_STATE_RELEASE);
			break;
		case EVENT_DEACTIVATE:
			CHIPTUNE_PRINTF(cEventTriggering,
							"tick = %u, DEACTIVATE oscillator = %d, voice = %d, note = %d, loudness = 0x%04x %s\r\n",
                            tick, p_head_event->oscillator_index,
							p_oscillator->voice, p_oscillator->note, p_oscillator->loudness,
                            event_additional_string(s_queued_event_head_index));
			SET_DEACTIVATED(p_oscillator->state_bits);
			break;
		case EVENT_DISCARD:
			CHIPTUNE_PRINTF(cEventTriggering,
							"tick = %u, DISCARD oscillator = %d, voice = %d, note = %d, amplitude = %1.2f%% of loudness %s\r\n",
                            tick, p_head_event->oscillator_index,
							p_oscillator->voice, p_oscillator->note,
							100.0f * p_oscillator->amplitude/(float)p_oscillator->loudness,
                            event_additional_string(s_queued_event_head_index));
            discard_oscillator(p_head_event->oscillator_index);
			break;
		default:
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: UNKOWN event type = %d\r\n");
			break;
		}
        p_head_event->type = UNUSED_EVENT;
        s_queued_event_head_index = p_head_event->next_event_index;
        s_queued_event_number -= 1;
	}
    CHECK_QUEUED_EVENTS(tick);
	return 0;
}

/**********************************************************************************/

void clean_all_events(void)
{
    reset_all_oscillators_and_links();
    reset_all_events();
}

/**********************************************************************************/

uint32_t const get_next_event_triggering_tick(void)
{
    if(0 == s_queued_event_number){
			return NULL_TICK;
	}

    return get_event_pointer_from_index(s_queued_event_head_index)->triggering_tick;
}

/**********************************************************************************/

int adjust_event_triggering_tick_by_playing_tempo(uint32_t const tick, float const new_playing_tempo)
{
	float tempo_ratio = new_playing_tempo/get_playing_tempo();
    uint16_t event_index = s_queued_event_head_index;
	bool is_reported = false;
    for(int16_t i = 0; i < s_queued_event_number; i++){
        event_t * const p_event = get_event_pointer_from_index(event_index);
		do
		{
            if(tick >= p_event->triggering_tick){
				break;
			}

			uint32_t const triggering_tick =
                    (uint32_t)((p_event->triggering_tick - tick) * tempo_ratio) + tick;
            if(triggering_tick == p_event->triggering_tick){
				break;
			}
			if(false == is_reported){
				CHIPTUNE_PRINTF(cEventTriggering, "tick = %u, tempo change from %3.1f to %3.1f\r\n",
								tick, get_playing_tempo(), new_playing_tempo);
			}
			is_reported = true;
			CHIPTUNE_PRINTF(cEventTriggering, "event = %d, oscillator = %d, triggering_tick %u ->%u\r\n",
                            event_index, p_event->oscillator_index,
                            p_event->triggering_tick, triggering_tick);
            p_event->triggering_tick = triggering_tick;
		} while(0);
        event_index = p_event->next_event_index;
	}

	if(true == is_reported){
		CHIPTUNE_PRINTF(cEventTriggering, "-------------------------------------------------\r\n");
		CHIPTUNE_PRINTF(cEventTriggering, "-------------------------------------------------\r\n");
	}

	return 0;
}
