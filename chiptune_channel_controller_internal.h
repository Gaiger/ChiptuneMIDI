#ifndef _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_
#define _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_

#include <stdint.h>
#include <stdbool.h>


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

	int8_t		waveform;
	uint8_t		: 8;
	uint16_t	duty_cycle_critical_phase;

	int8_t		pitch_wheel_bend_range_in_semitones;
	int16_t		pitch_wheel;

	bool		is_damper_pedal_on;
	uint8_t		: 8;
	int8_t		modulation_wheel;
	int8_t		chorus;

	uint16_t	registered_parameter_number;
	uint16_t	registered_parameter_value;
} channel_controller_t;

void reset_channel_controller_from_index(int8_t const index);
void reset_all_reset_channel_controller(void);
channel_controller_t * const get_channel_controller_pointer_from_index(int8_t const index);

#endif // _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_
