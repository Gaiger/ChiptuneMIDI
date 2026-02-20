#ifndef _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_
#define _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_

#include <stdint.h>
#include <stdbool.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "chiptune_common_internal.h"

#define CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH		(64)
#define REMAINDER_OF_DIVIDE_BY_CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH(INDEX) \
	((INDEX) & (CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1))

//NOTE :: 5 extra ticks make the envenlop release in linear curve able to down to 1% of loudness.
#define COMPLEMENTARY_RELEASE_TICK_NUMBER			(5)
#define ENSURE_RELEASE_TICK_NUMBER_SUFFICIENT(RELREASE_TICK_NUMBER)	\
	(ceilf(RELREASE_TICK_NUMBER) + (COMPLEMENTARY_RELEASE_TICK_NUMBER))

enum
{
	WaveformSquare				= 0,
	WaveformTriangle			= 1,
	WaveformSaw					= 2,
	WaveformSine				= 3,
	WaveformNoise				= 4,
};

enum
{
	WaveformDutyCycle50			= DIVIDE_BY_2(UINT16_MAX + 1),
	WaveformDutyCycle25			= DIVIDE_BY_4(UINT16_MAX + 1),
	WaveformDutyCycle12_5		= DIVIDE_BY_8(UINT16_MAX + 1),
	WaveformDutyCycle75			= (UINT16_MAX + 1) - DIVIDE_BY_4(UINT16_MAX + 1),
};

enum EnvelopeCurve
{
	EnvelopeCurveLinear			= 0,
	EnvelopeCurveExponential,
	EnvelopeCurveGaussian,
	EnvelopeCurveFermi,
};

typedef struct _channel_controller
{
	midi_value_t				instrument;
	midi_value_t				coarse_tuning_value;
	int16_t						fine_tuning_value;
	float						tuning_in_semitones;

	normalized_midi_level_t		volume;
	normalized_midi_level_t		pressure;
	normalized_midi_level_t		expression;
	normalized_midi_level_t		breath;

	midi_value_t				pan;

	int8_t						waveform;
	uint8_t						: 8;
	uint16_t					duty_cycle_critical_phase;

	midi_value_t				pitch_wheel_bend_range_in_semitones;
	float						pitch_wheel_bend_in_semitones;

	normalized_midi_level_t		modulation_wheel;
	int8_t						vibrato_depth_in_semitones;
	int8_t const *				p_vibrato_phase_table;
	uint16_t					vibrato_same_index_number;

	normalized_midi_level_t		reverb;
	normalized_midi_level_t		chorus;
	normalized_midi_level_t		detune;

	int8_t const *				p_envelope_attack_table;
	uint16_t					envelope_attack_same_index_number;

	int8_t const *				p_envelope_decay_table;
	uint16_t					envelope_decay_same_index_number;

	normalized_midi_level_t		envelope_note_on_sustain_level;

	float						envelope_release_duration_in_seconds;
	int8_t const *				p_envelope_release_table;
	uint16_t					envelope_release_same_index_number;
	uint16_t					envelope_release_tick_number;

	bool						is_damper_pedal_on;

	normalized_midi_level_t 	envelop_damper_sustain_level;
	int8_t const *				p_envelope_damper_sustain_table;
	uint16_t					envelope_damper_sustain_same_index_number;

	uint16_t					registered_parameter_number;
	uint16_t					registered_parameter_value;
} channel_controller_t;

#define CHANNEL_CONTROLLER_INSTRUMENT_NOT_SPECIFIED		(-1)
#define CHANNEL_CONTROLLER_INSTRUMENT_UNUSED_CHANNEL	(-2)

#define MAX_PERCUSSION_WAVEFORM_SEGMENT_NUMBER		(4)

typedef struct _percussion
{
	int8_t				waveform[MAX_PERCUSSION_WAVEFORM_SEGMENT_NUMBER];
	uint32_t			waveform_segment_duration_sample_number[MAX_PERCUSSION_WAVEFORM_SEGMENT_NUMBER];
	uint16_t			base_phase_increment;
	int16_t				max_phase_sweep_delta;
	int8_t const *		p_phase_sweep_table;
	int8_t const *		p_amplitude_envelope_table;
	uint16_t			envelope_same_index_number;

	bool				is_implemented;
} percussion_t;

void initialize_channel_controllers(void);

void reset_channel_controller_to_midi_defaults(int8_t const channel_index);
void reset_all_channels_to_defaults();

void synchronize_channel_controllers_to_playing_tempo(void);

channel_controller_t * get_channel_controller_pointer_from_index(int8_t const channel_index);
percussion_t const * get_percussion_pointer_from_key(int8_t const percussion_key);

int set_melodic_channel_timbre(int8_t const channel_index, int8_t const waveform, uint16_t const dutycycle_critical_phase,
									   int8_t const envelope_attack_curve, float const envelope_attack_duration_in_seconds,
									   int8_t const envelope_decay_curve, float const envelope_decay_duration_in_seconds,
									   uint8_t const envelope_note_on_sustain_level,
									   int8_t const envelope_release_curve, float const envelope_release_duration_in_seconds,
									   uint8_t const envelope_damper_sustain_level,
									   int8_t const envelope_damper_sustain_curve,
									   float const envelope_damper_sustain_duration_in_seconds);

#endif // _CHIPTUNE_CHANNEL_CONTROLLER_INTERNAL_H_
