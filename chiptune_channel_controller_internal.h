#ifndef _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_
#define _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_

#include <stdint.h>
#include <stdbool.h>

#define CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH		(64)

#define CHANNEL_CONTROLLER_DIVIDE_BY_128(VALUE)		((VALUE) >> 7)

#define NORMALIZE_VIBRTO_DELTA_PHASE(VALUE)			\
													CHANNEL_CONTROLLER_DIVIDE_BY_128(CHANNEL_CONTROLLER_DIVIDE_BY_128(((int32_t)(VALUE))))
#define REGULATE_MODULATION_WHEEL(VALUE)			((VALUE) + 1)

#define DELTA_VIBTRATO_PHASE(MODULATION_WHEEL, MAX_max_delta_vibrato_phase, VIBRATO_TABLE_VALUE) \
							NORMALIZE_VIBRTO_DELTA_PHASE( \
								((MAX_max_delta_vibrato_phase) * REGULATE_MODULATION_WHEEL(MODULATION_WHEEL)) * (VIBRATO_TABLE_VALUE) \
							)

#define REMAINDER_OF_DIVIDE_BY_CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH(INDEX)		\
															((INDEX) & (CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1))
#define SUSTAIN_AMPLITUDE(LOUNDNESS, SUSTAIN_LEVEL)	\
													((int16_t)CHANNEL_CONTROLLER_DIVIDE_BY_128((int32_t)(LOUNDNESS) * (SUSTAIN_LEVEL) * 16))

#define ENVELOPE_AMPLITUDE(AMPLITUDE, TABLE_VALUE)	\
													(CHANNEL_CONTROLLER_DIVIDE_BY_128((AMPLITUDE) * (int32_t)(TABLE_VALUE)))

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
	int8_t				tuning_in_semitones;

	int8_t				max_volume;
	int8_t				playing_volume;
	int8_t				pan;

	int8_t				waveform;
	uint8_t				: 8;
	uint16_t			duty_cycle_critical_phase;

	int8_t				pitch_wheel_bend_range_in_semitones;
	int16_t				pitch_wheel;

	bool				is_damper_pedal_on;
	uint8_t				: 8;

	int8_t				modulation_wheel;
	int8_t				vibrato_modulation_in_semitone;
	int8_t const *		p_vibrato_phase_table;
	uint16_t			vibrato_same_index_number;

	int8_t				chorus;
	float				max_pitch_chorus_bend_in_semitones;

	int8_t const *		p_envelope_attack_table;
	uint16_t			envelope_attack_tick_number;
	uint16_t			envelope_attack_same_index_number;

	int8_t const *		p_envelope_decay_table;
	uint16_t			envelope_decay_tick_number;
	uint16_t			envelope_decay_same_index_number;

	uint8_t				envelope_sustain_level;

	int8_t const *		p_envelope_release_table;
	uint16_t			envelope_release_tick_number;
	uint16_t			envelope_release_same_index_number;

	uint16_t			registered_parameter_number;
	uint16_t			registered_parameter_value;
} channel_controller_t;

void initialize_channel_controller(void);
void update_channel_controller_parameters_related_to_tempo(void);
channel_controller_t * const get_channel_controller_pointer_from_index(int8_t const index);

void reset_channel_controller_midi_parameters_from_index(int8_t const index);
void reset_channel_controller_all_parameters_from_index(int8_t const index);
#endif // _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_
