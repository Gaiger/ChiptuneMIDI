#ifndef _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_
#define _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_

#include <stdint.h>
#include <stdbool.h>

#include "chiptune_common_internal.h"

#define CHANNEL_CONTROLLER_INSTRUMENT_NOT_SPECIFIED		(-1)
#define CHANNEL_CONTROLLER_INSTRUMENT_UNUSED_CHANNEL	(-2)

#define CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH		(64)

#define NORMALIZE_VIBRATO_PHASE_INCREMENT(VALUE)			DIVIDE_BY_128(DIVIDE_BY_128(((int32_t)(VALUE))))

#define VIBRATO_PHASE_INCREMENT(MODULATION_WHEEL, MAX_VIBRATO_PHASE_INCREMENT, VIBRATO_TABLE_VALUE) \
	NORMALIZE_VIBRATO_PHASE_INCREMENT( \
		((MAX_VIBRATO_PHASE_INCREMENT) * MIDI_VALUE_TO_LEVEL_0_128(MODULATION_WHEEL)) * (VIBRATO_TABLE_VALUE) \
	)


#define REMAINDER_OF_DIVIDE_BY_CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH(INDEX) \
	((INDEX) & (CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1))


#define LOUNDNESS_AS_DAMPING_PEDAL_ON_BUT_NOTE_OFF(NOTE_ON_LOUNDNESS, DAMPER_ON_BUT_NOTE_OFF_LOUDNESS_LEVEL) \
						(\
							(int16_t)DIVIDE_BY_128((int32_t)(NOTE_ON_LOUNDNESS) \
							* (DAMPER_ON_BUT_NOTE_OFF_LOUDNESS_LEVEL))\
						)

#define CHANNEL_CONTROLLER_SCALE_BY_LEVEL(VALUE, LEVEL)	\
	MULTIPLY_THEN_DIVIDE_BY_128((VALUE), (LEVEL))

#define SUSTAIN_AMPLITUDE(LOUNDNESS, SUSTAIN_LEVEL)	\
	CHANNEL_CONTROLLER_SCALE_BY_LEVEL((LOUNDNESS), (SUSTAIN_LEVEL))

#define MELODIC_ENVELOPE_DELTA_AMPLITUDE(AMPLITUDE, TABLE_VALUE)	\
	CHANNEL_CONTROLLER_SCALE_BY_LEVEL((AMPLITUDE), (TABLE_VALUE))

#define PERCUSSION_PHASE_SWEEP_DELTA(MAX_PHASE_SWEEP_DELTA, SWEEP_TABLE_VALUE) \
	CHANNEL_CONTROLLER_SCALE_BY_LEVEL((MAX_PHASE_SWEEP_DELTA), (SWEEP_TABLE_VALUE))

#define PERCUSSION_ENVELOPE(LOUDNESS, ENVELOPE_TABLE_VALUE)	\
	CHANNEL_CONTROLLER_SCALE_BY_LEVEL((LOUDNESS), (ENVELOPE_TABLE_VALUE))

enum
{
	WAVEFORM_SILENCE		= -1,
	WAVEFORM_SQUARE			= 0,
	WAVEFORM_TRIANGLE		= 1,
	WAVEFORM_SAW			= 2,
	WAVEFORM_SINE			= 3,
	WAVEFORM_NOISE			= 4,
};

enum
{
	DUTY_CYLCE_NONE					= UINT16_MAX,
	DUTY_CYLCE_50_CRITICAL_PHASE	= (UINT16_MAX + 1) >> 1,
	DUTY_CYLCE_25_CRITICAL_PHASE	= (UINT16_MAX + 1) >> 2,
	DUTY_CYLCE_125_CRITICAL_PHASE	= (UINT16_MAX + 1) >> 3,
	DUTY_CYLCE_75_CRITICAL_PHASE	= (UINT16_MAX + 1) - ((UINT16_MAX + 1) >> 2),
};

enum
{
	ENVELOPE_CURVE_LINEAR,
	ENVELOPE_CURVE_EXPONENTIAL,
	ENVELOPE_CURVE_GAUSSIAN,
	ENVELOPE_CURVE_FERMI,
};

typedef struct _channel_controller
{
	int8_t				instrument;
	int8_t				coarse_tuning_value;
	int16_t				fine_tuning_value;
	float				tuning_in_semitones;

	int8_t				volume;
	int8_t				pressure;
	int8_t				expression;
	int8_t				pan;

	int8_t				waveform;
	uint8_t				: 8;
	uint16_t			duty_cycle_critical_phase;

	int8_t				pitch_wheel_bend_range_in_semitones;
	float				pitch_wheel_bend_in_semitones;

	int8_t				modulation_wheel;
	int8_t				vibrato_modulation_in_semitones;
	int8_t const *		p_vibrato_phase_table;
	uint16_t			vibrato_same_index_number;

	int8_t				reverb;
	int8_t				chorus;
	float				max_pitch_chorus_bend_in_semitones;

	int8_t const *		p_envelope_attack_table;
	uint16_t			envelope_attack_same_index_number;

	int8_t const *		p_envelope_decay_table;
	uint16_t			envelope_decay_same_index_number;

	uint8_t				envelope_sustain_level;

	float				envelope_release_duration_in_seconds;
	int8_t const *		p_envelope_release_table;
	uint16_t			envelope_release_same_index_number;
	uint16_t			envelope_release_tick_number;

	bool				is_damper_pedal_on;

	uint8_t				envelop_damper_on_but_note_off_sustain_level;
	float				envelope_damper_on_but_note_off_sustain_duration_in_seconds;
	int8_t const *		p_envelope_damper_on_but_note_off_sustain_table;
	uint16_t			envelope_damper_on_but_note_off_sustain_same_index_number;

	uint16_t			registered_parameter_number;
	uint16_t			registered_parameter_value;
} channel_controller_t;


#define MAX_WAVEFORM_CHANGE_NUMBER					(4)

typedef struct _percussion
{
	int8_t				waveform[MAX_WAVEFORM_CHANGE_NUMBER];
	uint32_t			waveform_duration_sample_number[MAX_WAVEFORM_CHANGE_NUMBER];
	uint16_t			base_phase_increment;
	int16_t				max_phase_sweep_delta;
	int8_t const *		p_phase_sweep_table;
	int8_t const *		p_amplitude_envelope_table;
	uint16_t			envelope_same_index_number;

	bool				is_implemented;
} percussion_t;

void initialize_channel_controllers(void);

void reset_channel_controller_midi_control_change_parameters(int8_t const index);
void reset_all_channel_controllers();

channel_controller_t * const get_channel_controller_pointer_from_index(int8_t const index);
percussion_t * const get_percussion_pointer_from_index(int8_t const index);

void update_channel_controllers_parameters_related_to_playing_tempo(void);

int set_pitch_channel_parameters(int8_t const channel_index, int8_t const waveform, uint16_t const dutycycle_critical_phase,
									   int8_t const envelope_attack_curve, float const envelope_attack_duration_in_seconds,
									   int8_t const envelope_decay_curve, float const envelope_decay_duration_in_seconds,
									   uint8_t const envelope_sustain_level,
									   int8_t const envelope_release_curve, float const envelope_release_duration_in_seconds,
									   uint8_t const envelope_damper_on_but_note_off_sustain_level,
									   int8_t const envelope_damper_on_but_note_off_sustain_curve,
									   float const envelope_damper_on_but_note_off_sustain_duration_in_seconds);

#endif // _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_
