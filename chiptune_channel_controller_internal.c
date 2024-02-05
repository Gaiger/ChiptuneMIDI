#define _USE_MATH_DEFINES
#include <math.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"

#include "chiptune_channel_controller_internal.h"

static channel_controller_t s_channel_controllers[MIDI_MAX_CHANNEL_NUMBER];

static int8_t s_vibrato_phase_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH] = {0};

static int8_t s_linear_decline_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH];
static int8_t s_linear_growth_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH];

static int8_t s_exponential_decline_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH];
static int8_t s_exponential_growth_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH];

static int8_t s_gaussian_decline_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH];
static int8_t s_gaussian_growth_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH];


channel_controller_t * const get_channel_controller_pointer_from_index(int8_t const index)
{
	if(false == (index >= 0 && index < MIDI_MAX_CHANNEL_NUMBER)){
		CHIPTUNE_PRINTF(cDeveloping, "channel_controller = %d, out of range\r\n");
		return NULL;
	}
	return &s_channel_controllers[index];
}

/**********************************************************************************/

void reset_channel_controller_midi_parameters_from_index(int8_t const index)
{
	channel_controller_t * const p_channel_controller = &s_channel_controllers[index];
	p_channel_controller->tuning_in_semitones = 0;

	p_channel_controller->max_volume = MIDI_CC_CENTER_VALUE;
	p_channel_controller->playing_volume = (p_channel_controller->max_volume * INT8_MAX)/INT8_MAX;
	p_channel_controller->pan = MIDI_CC_CENTER_VALUE;

	p_channel_controller->pitch_wheel_bend_range_in_semitones = MIDI_DEFAULT_PITCH_WHEEL_BEND_RANGE_IN_SEMITONES;
	p_channel_controller->pitch_wheel = MIDI_PITCH_WHEEL_CENTER;

	p_channel_controller->modulation_wheel = 0;
	p_channel_controller->chorus = 0;
	p_channel_controller->is_damper_pedal_on = false;
}

/**********************************************************************************/

static void update_channel_controller_envelope_parameters_related_to_tempo(int8_t const index)
{
	uint32_t const resolution = get_resolution();
	float const tempo = get_tempo();
	channel_controller_t * const p_channel_controller = &s_channel_controllers[index];

	p_channel_controller->envelope_release_tick_number
		= (uint16_t)(p_channel_controller->envelope_release_duration_in_second * resolution * tempo/60.0f + 0.5f);
}

/**********************************************************************************/

void reset_channel_controller_all_parameters_from_index(int8_t const index)
{
	uint32_t const sampling_rate = get_sampling_rate();

	channel_controller_t * const p_channel_controller = &s_channel_controllers[index];
	p_channel_controller->waveform = WAVEFORM_TRIANGLE;

#define	DEFAULT_VIBRATO_MODULATION_IN_SEMITINE		(1)
#define DEFAULT_VIBRATO_RATE						(4)
	p_channel_controller->vibrato_modulation_in_semitone = DEFAULT_VIBRATO_MODULATION_IN_SEMITINE;
	p_channel_controller->p_vibrato_phase_table = &s_vibrato_phase_table[0];
	p_channel_controller->vibrato_same_index_number
			= (uint16_t)(sampling_rate/CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH/(float)DEFAULT_VIBRATO_RATE);

#define	DEFAULT_MAX_CHORUS_PITCH_BEND_IN_SEMITONE	(0.25f)
	p_channel_controller->max_pitch_chorus_bend_in_semitones = DEFAULT_MAX_CHORUS_PITCH_BEND_IN_SEMITONE;

	/*
	 * s_linear_decline_table s_linear_growth_table
	 * s_exponential_decline_table s_exponential_growth_table
	 * s_gaussian_decline_table s_gaussian_growth_table
	*/

	p_channel_controller->p_envelope_attack_table = &s_linear_growth_table[0];
#define DEFAULT_ENVELOPE_ATTACK_DURATION_IN_SECOND	(0.02f)
	p_channel_controller->envelope_attack_same_index_number
		= (uint16_t)((sampling_rate * DEFAULT_ENVELOPE_ATTACK_DURATION_IN_SECOND)
					 /(float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH + 0.5);

	p_channel_controller->p_envelope_decay_table  = &s_gaussian_decline_table[0];
	#define DEFAULT_ENVELOPE_DECAY_DURATION_IN_SECOND	(0.01f)
	p_channel_controller->envelope_decay_same_index_number
		= (uint16_t)((sampling_rate * DEFAULT_ENVELOPE_DECAY_DURATION_IN_SECOND)
					 /(float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH + 0.5);

	//100% = level 8
#define DEFAULT_ENVELOPE_SUSTAIN_LEVEL				(7)
	p_channel_controller->envelope_sustain_level = DEFAULT_ENVELOPE_SUSTAIN_LEVEL;

#define DEFAULT_ENVELOPE_RLEASE_DURATION_IN_SECOND	(0.03f)
	p_channel_controller->envelope_release_duration_in_second = DEFAULT_ENVELOPE_RLEASE_DURATION_IN_SECOND;
	p_channel_controller->p_envelope_release_table = &s_exponential_decline_table[0];
	p_channel_controller->envelope_release_same_index_number
		= (uint16_t)((sampling_rate * DEFAULT_ENVELOPE_RLEASE_DURATION_IN_SECOND)
					 /(float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH + 0.5);


	//100% = level 32
#define DEFAULT_DAMPER_ON_BUT_NOTE_OFF_LOUDNESS_LEVEL	(6)
	p_channel_controller->damper_on_but_note_off_loudness_level = DEFAULT_DAMPER_ON_BUT_NOTE_OFF_LOUDNESS_LEVEL;
	p_channel_controller->p_envelope_damper_on_but_note_off_sustain_table = &s_linear_decline_table[0];
#define DEFAULT_ENVELOPE_DAMPER_ON_BUT_NOTE_OFF_SUSTAIN_DURATION_IN_SECOND \
													(8.0)
	p_channel_controller->envelope_damper_on_but_note_off_sustain_same_index_number
		= (uint16_t)((sampling_rate * DEFAULT_ENVELOPE_DAMPER_ON_BUT_NOTE_OFF_SUSTAIN_DURATION_IN_SECOND)
					 /(float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH + 0.5);
	//p_channel_controller->envelope_damper_on_but_note_off_sustain_same_index_number = UINT16_MAX;

	update_channel_controller_envelope_parameters_related_to_tempo(index);
	reset_channel_controller_midi_parameters_from_index(index);
}

/**********************************************************************************/

void update_channel_controller_parameters_related_to_tempo(void)
{
	for(int8_t i = 0; i < MIDI_MAX_CHANNEL_NUMBER; i++){
		update_channel_controller_envelope_parameters_related_to_tempo(i);
	}
}

/**********************************************************************************/

static void initialize_envelope_tables(void)
{
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		int ii = (CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH  - i);
		s_linear_decline_table[i] = (int8_t)(INT8_MAX * (ii/(float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH));
	}
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_linear_growth_table[i]
				= s_linear_decline_table[(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1) - i];
	}

	/*
	 * exponential :
	 *  INT8_MAX * exp(-alpha * (TABLE_LENGTH -1)) = 1 -> alpha = -ln(1/INT8_MAX)/(TABLE_LENGTH -1)
	*/
	const float alpha = -logf(1/(float)INT8_MAX)/(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1);
	s_exponential_decline_table[0] = INT8_MAX;
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_exponential_decline_table[i] = (int8_t)(INT8_MAX * expf( -alpha * i) + 0.5);
	}
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_exponential_growth_table[i]
				= s_exponential_decline_table[(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1) - i];
	}

	/*
	 * gaussian
	 *  INT8_MAX * exp(-beta * (TABLE_LENGTH -1)**2) = 1 -> beta = -ln(INT8_MAX - 1)/(1 - (TABLE_LENGTH -1)**2)
	*/
	const float beta = -logf(INT8_MAX - 1)/(1 - powf((float)(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1), 2.0f));
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_gaussian_decline_table[i] = (int8_t)(INT8_MAX * expf(-beta * i * i) + 0.5);
	}
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_gaussian_growth_table[i] = s_gaussian_decline_table[(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1) - i];
	}
}

/**********************************************************************************/

void initialize_channel_controller(void)
{
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_vibrato_phase_table[i] = (int8_t)(INT8_MAX * sinf( 2.0f * (float)M_PI * i / (float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH));
	}
	initialize_envelope_tables();
	for(int8_t i = 0; i < MIDI_MAX_CHANNEL_NUMBER; i++){
		reset_channel_controller_all_parameters_from_index(i);
	}
}
