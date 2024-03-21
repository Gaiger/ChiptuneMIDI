#ifndef _CHIPTUNE_COMMON_INTERNAL_H_
#define _CHIPTUNE_COMMON_INTERNAL_H_

#include <stdint.h>

#include "chiptune_midi_define_internal.h"

#define _INCREMENTAL_SAMPLE_INDEX
//#define _RIGHT_SHIFT_FOR_NORMALIZING_LOUNDNESS

#define _PRINT_DEVELOPING
//#define _PRINT_MIDI_SETUP
//#define _PRINT_NOTE_OPERATION
//#define _PRINT_EVENT_TRIGGERING

#define _CHECK_OCCUPIED_OSCILLATOR_LIST
#define _CHECK_EVENT_LIST


#define NULL_TICK									(UINT32_MAX)

uint32_t const get_sampling_rate(void);
uint32_t const get_resolution(void);
float const get_tempo(void);

#endif // _CHIPTUNE_COMMON_INTERNAL_H_
