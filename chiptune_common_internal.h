#ifndef _CHIPTUNE_COMMON_INTERNAL_H_
#define _CHIPTUNE_COMMON_INTERNAL_H_

#include <stdint.h>

#include "chiptune_midi_define_internal.h" // IWYU pragma: export

//#define _INCREMENTAL_SAMPLE_INDEX
//#define _AMPLITUDE_NORMALIZATION_BY_RIGHT_SHIFT
//#define _KEEP_NOTELESS_CHANNELS
//#define _FIXED_OSCILLATOR_AND_EVENT_CAPACITY

#define _PRINT_DEVELOPING
//#define _PRINT_MIDI_NOTE
//#define _PRINT_MIDI_CONTROLCHANGE
//#define _PRINT_MIDI_PROGRAMCHANGE
//#define _PRINT_MIDI_CHANNELPRESSURE
//#define _PRINT_MIDI_PITCH_WHEEL
//#define _PRINT_EVENT_TRIGGERING

#define _CHECK_OCCUPIED_OSCILLATOR_LIST
#define _CHECK_EVENT_LIST

#ifdef _FIXED_OSCILLATOR_AND_EVENT_CAPACITY
	#define OCCUPIABLE_OSCILLATOR_CAPACITY			(512)
#endif

#define DIVIDE_BY_2(VALUE)							((VALUE) >> 1)
#define DIVIDE_BY_16(VALUE)							((VALUE) >> 4)
#define DIVIDE_BY_32(VALUE)							((VALUE) >> 5)
#define DIVIDE_BY_128(VALUE)						((VALUE) >> 7)

#define MULTIPLY_BY_2(VALUE)						((VALUE) << 1)

#define NULL_TICK									(UINT32_MAX)

#ifndef _FIXED_OSCILLATOR_AND_EVENT_CAPACITY
	void* chiptune_malloc(size_t size);
	void chiptune_free(void* ptr);
#endif

uint32_t const get_sampling_rate(void);
uint32_t const get_resolution(void);
float const get_playing_tempo(void);

#endif // _CHIPTUNE_COMMON_INTERNAL_H_
