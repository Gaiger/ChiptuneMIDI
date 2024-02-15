#ifndef _CHIPTUNE_COMMON_INTERNAL_H_
#define _CHIPTUNE_COMMON_INTERNAL_H_

#include <stdint.h>

//#define _INCREMENTAL_SAMPLE_INDEX
//#define _RIGHT_SHIFT_FOR_NORMALIZING_LOUNDNESS

#define _PRINT_DEVELOPING
//#define _PRINT_MIDI_SETUP
#define _PRINT_NOTE_OPERATION
//#define _PRINT_EVENT_TRIGGERING

#define _CHECK_OCCUPIED_OSCILLATOR_LIST
#define _CHECK_EVENT_LIST

#define MIDI_MAX_CHANNEL_NUMBER						(16)
#define MIDI_CC_CENTER_VALUE						(64)
#define MIDI_PITCH_WHEEL_CENTER						(0x2000)
#define MIDI_DEFAULT_PITCH_WHEEL_BEND_RANGE_IN_SEMITONES	\
															(2 * 2)

#define MIDI_CC_RPN_NULL							((127 << 8) + 127)

#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0		(9)
#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1		(10)

#define NULL_TICK									(UINT32_MAX)

uint32_t const get_sampling_rate(void);
uint32_t const get_resolution(void);
float const get_tempo(void);

#endif // _CHIPTUNE_COMMON_INTERNAL_H_
