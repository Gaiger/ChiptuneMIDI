#ifndef _CHIPTUNE_PRINTF_INTERNAL_H_
#define _CHIPTUNE_PRINTF_INTERNAL_H_

#include <stdbool.h>
#include "chiptune_common_internal.h"

#if defined(_PRINT_DEVELOPING) \
		|| defined(_PRINT_MIDI_NOTE) \
		|| defined(_PRINT_MIDI_CONTROLCHANGE) \
		|| defined(_PRINT_MIDI_PROGRAMCHANGE) \
		|| defined(_PRINT_MIDI_CHANNELPRESSURE) \
		|| defined(_PRINT_MIDI_PITCH_WHEEL) \
		|| defined(_PRINT_EVENT_TRIGGERING)
#define CHIPTUNE_PRINTF(PRINT_TYPE, FMT, ...)		\
													do { \
														chiptune_printf(PRINT_TYPE, FMT, ##__VA_ARGS__); \
													}while(0)

#else
#define CHIPTUNE_PRINTF(PRINT_TYPE, FMT, ...)		\
													do { \
														(void)0; \
													}while(0)
#endif

#define SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(IS_ENABLED)		\
													do { \
														set_chiptune_processing_printf_enabled(IS_ENABLED); \
													} while(0)


enum
{
	cDeveloping				= 0,

	cMidiNote				= 1,
	cMidiControlChange		= 2,
	cMidiProgramChange		= 3,
	cMidiChannelPressure	= 4,
	cMidiPitchWheel			= 5,

	cEventTriggering		= 6,
};

void chiptune_printf(int const print_type, const char* fmt, ...);
void set_chiptune_processing_printf_enabled(bool is_enabled);


#endif // _CHIPTUNE_PRINTF_INTERNAL_H_
