#ifndef _CHIPTUNE_COMMON_INTERNAL_H_
#define _CHIPTUNE_COMMON_INTERNAL_H_

#include <stdbool.h>
#include <stdint.h>
//#define _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING

//#define _INCREMENTAL_SAMPLE_INDEX
//#define _RIGHT_SHIFT_FOR_NORMALIZING_AMPLITUDE

#define _PRINT_DEVELOPING
#define _PRINT_MIDI_SETUP
#define _PRINT_NOTE_OPERATION
//#define _PRINT_OSCILLATOR_TRANSITION

//#define _CHECK_OCCUPIED_OSCILLATOR_LIST
//#define _CHECK_EVENT_LIST

enum
{
	WAVEFORM_SILENCE		= -1,
	WAVEFORM_SQUARE			= 0,
	WAVEFORM_TRIANGLE		= 1,
	WAVEFORM_SAW			= 2,
	WAVEFORM_NOISE			= 3,
};

enum
{
	DUTY_CYLCE_125_CRITICAL_PHASE	= (UINT16_MAX + 1) >> 3,
	DUTY_CYLCE_25_CRITICAL_PHASE	= (UINT16_MAX + 1) >> 2,
	DUTY_CYLCE_50_CRITICAL_PHASE	= (UINT16_MAX + 1) >> 1,
	DUTY_CYLCE_75_CRITICAL_PHASE	= (UINT16_MAX + 1) - ((UINT16_MAX + 1) >> 2),
};


typedef struct _channel_controller
{
	int8_t		tuning_in_semitones;

	int8_t		max_volume;
	int8_t		playing_volume;
	int8_t		pan;

	uint8_t		waveform;
	uint8_t		: 8;
	uint16_t	duty_cycle_critical_phase;

	uint16_t	pitch_wheel_bend_range_in_semitones;
	uint16_t	pitch_wheel;

	bool		is_damper_pedal_on;
	uint8_t		: 8;
	int8_t		modulation_wheel;
	int8_t		chorus;

	uint16_t	registered_parameter_number;
	uint16_t	registered_parameter_value;
} channel_controller_t;

#define MIDI_MAX_CHANNEL_NUMBER						(16)
#define MIDI_CC_CENTER_VALUE						(64)
#define MIDI_PITCH_WHEEL_CENTER						(0x2000)
#define MIDI_DEFAULT_PITCH_WHEEL_BEND_RANGE_IN_SEMITONES	\
															(2 * 2)

#define MIDI_CC_RPN_NULL							((127 << 8) + 127)

void reset_channel_controller_from_index(int8_t const index);
void reset_all_reset_channel_controller(void);
channel_controller_t * const get_channel_controller_pointer_from_index(int8_t const index);

#endif // _CHIPTUNE_COMMON_INTERNAL_H_
