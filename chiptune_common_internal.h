#ifndef _CHIPTUNE_COMMON_INTERNAL_H_
#define _CHIPTUNE_COMMON_INTERNAL_H_

#include <stdbool.h>
#include <stdint.h>
//#define _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING

//#define _INCREMENTAL_SAMPLE_INDEX
//#define _RIGHT_SHIFT_FOR_NORMALIZING_AMPLITUDE

#define _PRINT_MIDI_DEVELOPING
#define _PRINT_MIDI_SETUP
#define _PRINT_MOTE_OPERATION



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


struct _channel_controller
{
	int8_t		tuning_in_semitones;

	uint8_t		max_volume;
	uint8_t		playing_volume;
	uint8_t		pan;

	uint8_t		waveform;
	uint8_t		: 8;
	uint16_t	duty_cycle_critical_phase;

	uint16_t	pitch_bend_range_in_semitones;
	uint16_t	pitch_wheel;

	uint8_t		modulation_wheel;

	uint8_t		chorus;

	uint16_t	registered_parameter_number;
	uint16_t	registered_parameter_value;
};

struct _oscillator
{
	uint8_t		state_bits;
	uint8_t		: 8;

	int8_t		voice;

	uint8_t		note;
	uint16_t	delta_phase;
	uint16_t	current_phase;

	uint16_t	volume;

	uint8_t		waveform;
	uint8_t		: 8;
	uint16_t	duty_cycle_critical_phase;

	uint16_t	delta_vibration_phase;
	uint16_t	vibration_table_index;
	uint32_t	vibration_same_index_count;
};

#define MAX_VOICE_NUMBER							(16)
#define MAX_OSCILLATOR_NUMBER						(MAX_VOICE_NUMBER * 2)

#define UNUSED_OSCILLATOR							(-1)

#define RESET_STATE_BITES(STATE_BITES)				(STATE_BITES = 0)

#define STATE_NOTE_BIT								(0)
#define SET_NOTE_ON(STATE_BITS)						(STATE_BITS |= (0x01 << STATE_NOTE_BIT) )
#define SET_NOTE_OFF(STATE_BITS)					(STATE_BITS &= (~(0x01 << STATE_NOTE_BIT)) )
#define IS_NOTE_ON(STATE_BITS)						(((0x01 << STATE_NOTE_BIT) & STATE_BITS) ? true : false)

#define STATE_DAMPER_PEDAL_BIT						(1)
#define SET_DAMPER_PEDAL_ON(STATE_BITS)				(STATE_BITS |= ((0x01)<< STATE_DAMPER_PEDAL_BIT) )
#define SET_DAMPER_PEDAL_OFF(STATE_BITS)			(STATE_BITS &= (~((0x01)<< STATE_DAMPER_PEDAL_BIT)))
#define IS_DAMPER_PEDAL_ON(STATE_BITS)				(((0x01 << STATE_DAMPER_PEDAL_BIT) & STATE_BITS) ? true : false)

#define STATE_CHURUS_OSCILLATOR_BIT					(2)
#define SET_CHURUS_OSCILLATOR(STATE_BITS)			(STATE_BITS |= ((0x01)<< STATE_CHURUS_OSCILLATOR_BIT))
#define RESET_CHURUS_OSCILLATOR(STATE_BITS)			(STATE_BITS &= (~((0x01)<< STATE_CHURUS_OSCILLATOR_BIT)))
#define IS_CHURUS_OSCILLATOR(STATE_BITS)			(((0x01 << STATE_CHURUS_OSCILLATOR_BIT) & STATE_BITS) ? true : false)

#define MIDI_CC_CENTER_VALUE						(64)
#define MIDI_PITCH_WHEEL_CENTER						(0x2000)
#define MIDI_DEFAULT_PITCH_BEND_RANGE_IN_SEMITONES	(2 * 2)

#endif // _CHIPTUNE_COMMON_INTERNAL_H_
