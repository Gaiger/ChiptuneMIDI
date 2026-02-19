#ifndef _CHIPTUNE_COMMON_INTERNAL_H_
#define _CHIPTUNE_COMMON_INTERNAL_H_

#include <stdint.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif

#ifdef __clang__
	#define MAYBE_UNUSED_FUNCTION __attribute__((unused))
#else
	#define MAYBE_UNUSED_FUNCTION
#endif

#include "chiptune_midi_define.h" // IWYU pragma: export


#define _USE_SAMPLE_INDEX_AS_TIMEBASE
//#define _AMPLITUDE_NORMALIZATION_BY_RIGHT_SHIFT
//#define _KEEP_NOTELESS_CHANNELS
//#define _USE_STATIC_RESOURCE_ALLOCATION

#define _PRINT_DEVELOPING
//#define _PRINT_MIDI_NOTE
//#define _PRINT_MIDI_CONTROLCHANGE
//#define _PRINT_MIDI_PROGRAMCHANGE
//#define _PRINT_MIDI_CHANNELPRESSURE
//#define _PRINT_MIDI_PITCH_WHEEL
//#define _PRINT_EVENT_TRIGGERING

#define _CHECK_OCCUPIED_OSCILLATOR_LIST
#define _CHECK_EVENT_LIST

#define INT8_MAX_PLUS_1								(INT8_MAX + 1)
#define INT16_MAX_PLUS_1							(INT16_MAX + 1)

#define DIVIDE_BY_2(VALUE)							((VALUE) >> 1)
#define DIVIDE_BY_4(VALUE)							((VALUE) >> 2)
#define DIVIDE_BY_8(VALUE)							((VALUE) >> 3)
#define DIVIDE_BY_16(VALUE)							((VALUE) >> 4)
#define DIVIDE_BY_32(VALUE)							((VALUE) >> 5)
#define DIVIDE_BY_64(VALUE)							((VALUE) >> 6)
#define DIVIDE_BY_128(VALUE)						((VALUE) >> 7)

#define MULTIPLY_BY_2(VALUE)						((VALUE) << 1)
#define MULTIPLY_BY_128(VALUE)						((VALUE) << 7)

#define MULTIPLY_THEN_DIVIDE_BY_128(A, B) 			((int16_t)DIVIDE_BY_128((int32_t)(A) * (int32_t)(B)))

// Reference implementations (for clarity)
#if 0
inline uint8_t one_to_zero(uint8_t x){
	uint8_t u = x ^ 0x01;
	uint8_t mask = 0 - ((uint8_t)(u | (0 - u)) >> 7);
	return x & mask;
}
#endif

// Optimized version (matches the macro below)
#if 0
inline uint8_t one_to_zero(uint8_t x){
	uint32_t u = (uint32_t)x ^ 0x01;
	uint32_t mask = 0 - ((0 - u) >> 31);
	return x & mask;
}
#endif
#define ONE_TO_ZERO(VALUE)							((VALUE) & (0 - ((0 - ((uint32_t)(VALUE) ^ 0x01)) >> 31)))
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
	#define STACK_ARRAY(type, name, count) \
		type name[128];
	#else
	#include <malloc.h>
	#define STACK_ARRAY(type, name, count) \
		type *name = (type*)_alloca(sizeof(type)*(count))
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

uint32_t const get_sampling_rate(void);
uint32_t const get_resolution(void);
float const get_playing_tempo(void);

uint16_t const calculate_phase_increment_from_pitch(float const pitch_in_semitones);

#endif // _CHIPTUNE_COMMON_INTERNAL_H_
