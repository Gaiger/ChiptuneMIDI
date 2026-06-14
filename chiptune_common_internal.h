#ifndef _CHIPTUNE_COMMON_INTERNAL_H_
#define _CHIPTUNE_COMMON_INTERNAL_H_

#include <stddef.h>
#include <stdint.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif

#ifdef __clang__
	#define MAYBE_UNUSED_FUNCTION __attribute__((unused))
#else
	#define MAYBE_UNUSED_FUNCTION
#endif

#include "chiptune_config_internal.h" // IWYU pragma: export
#include "chiptune_midi_define.h" // IWYU pragma: export

#define INT8_MAX_PLUS_1								(INT8_MAX + 1)
#define INT16_MAX_PLUS_1							(INT16_MAX + 1)

#define DIVIDE_BY_2(VALUE)							((VALUE) >> 1)
#define DIVIDE_BY_4(VALUE)							((VALUE) >> 2)
#define DIVIDE_BY_8(VALUE)							((VALUE) >> 3)
#define DIVIDE_BY_16(VALUE)							((VALUE) >> 4)
#define DIVIDE_BY_32(VALUE)							((VALUE) >> 5)
#define DIVIDE_BY_64(VALUE)							((VALUE) >> 6)
#define DIVIDE_BY_128(VALUE)						((VALUE) >> 7)
#define DIVIDE_BY_256(VALUE)						((VALUE) >> 8)

#define MULTIPLY_BY_2(VALUE)						((VALUE) << 1)
#define MULTIPLY_BY_128(VALUE)						((VALUE) << 7)

#define MULTIPLY_THEN_DIVIDE_BY_128(A, B) 			((int16_t)DIVIDE_BY_128((int32_t)(A) * (int32_t)(B)))

// Reference implementation for SATURATE_TO_INT8_MAX (for clarity)
#if 0
inline uint8_t saturate_to_int8_max(uint16_t const value){
	uint16_t const mask = (uint16_t)(0u - (uint16_t)!!(value >> 7));
	return (uint8_t)((value | mask) & INT8_MAX);
}
#endif
#define SATURATE_TO_INT8_MAX(VALUE)					((uint8_t)(((VALUE) | (uint16_t)(0u - (uint16_t)!!((VALUE) >> 7))) & INT8_MAX))

// Reference implementation for ONE_TO_ZERO (for clarity)
#if 0
inline uint8_t one_to_zero(uint8_t const x){
	uint8_t const u = x ^ 0x01;
	uint8_t const mask = (uint8_t)(0u - (uint8_t)!!u);
	return (uint8_t)(x & mask);
}
#endif
#define ONE_TO_ZERO(VALUE)							((VALUE) & (0 - !!((VALUE) ^ 0x01)))
#define ZERO_AS_ONE(VALUE)							((VALUE) + !((VALUE)))

#define NORMALIZE_MIDI_LEVEL(VALUE)					ONE_TO_ZERO((VALUE) + 1)

#define NULL_TICK									(UINT32_MAX)

typedef int8_t		midi_value_t;
typedef uint8_t		normalized_midi_level_t;

#ifndef _USE_STATIC_RESOURCE_ALLOCATION
	void* chiptune_malloc(size_t const size);
	void chiptune_free(void * const ptr);
#endif

#if defined(_MSC_VER)
	#if defined(_DEBUG)
	#include <assert.h>
	#define STACK_ARRAY_COUNT						(128)
	#define STACK_ARRAY(type, name, count) \
		do { assert(STACK_ARRAY_COUNT >= count); } while(0); \
		type name[STACK_ARRAY_COUNT]
	#else
	#include <malloc.h>
	#define STACK_ARRAY(type, name, count) \
		type * const name = (type*)_alloca(sizeof(type)*(count))
#endif
#else
	#define STACK_ARRAY(type, name, count) \
		type name[count]
#endif

#define EXPAND_ENUM(ITEM, VAL)						ITEM = VAL,

enum MidiInstrumentCode
{
	MIDI_INSTRUMENT_CODE_LIST(EXPAND_ENUM)
};

enum MidiPercussionKeyMap
{
	MIDI_PERCUSSION_KEY_MAP_LIST(EXPAND_ENUM)
};

/**********************************************************************************/

#define EXPAND_CASE_TO_STR(ENUMS_ELEMENT, DUMMY_VAR)			case ENUMS_ELEMENT:	return #ENUMS_ELEMENT;

MAYBE_UNUSED_FUNCTION static inline char const * const get_instrument_name_string(int8_t const instrument_code)
{
	switch (instrument_code)
	{
		MIDI_INSTRUMENT_CODE_LIST(EXPAND_CASE_TO_STR)
	}

	return "UnknownInstrument";
}

/**********************************************************************************/

MAYBE_UNUSED_FUNCTION static inline char const * const get_percussion_name_string(int8_t const percussion_key)
{
	switch (percussion_key)
	{
		MIDI_PERCUSSION_KEY_MAP_LIST(EXPAND_CASE_TO_STR)
	}

	return "UnknownPercussion";
}

/**********************************************************************************/

uint32_t get_sampling_rate(void);
uint32_t get_resolution(void);
float get_playing_tempo(void);

uint16_t calculate_phase_increment_from_pitch(float const pitch_in_semitones);

#endif // _CHIPTUNE_COMMON_INTERNAL_H_
