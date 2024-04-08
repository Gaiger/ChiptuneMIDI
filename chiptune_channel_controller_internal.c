#include <stddef.h>
#include <float.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"

#include "chiptune_channel_controller_internal.h"

static channel_controller_t s_channel_controllers[MIDI_MAX_CHANNEL_NUMBER];

static int8_t s_vibrato_phase_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH] = {0};

static int8_t s_linear_decline_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH] = {0};
static int8_t s_linear_growth_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH] = {0};

static int8_t s_exponential_decline_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH] = {0};
static int8_t s_exponential_growth_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH] = {0};

static int8_t s_gaussian_decline_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH] = {0};
static int8_t s_gaussian_growth_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH] = {0};

static int8_t s_fermi_decline_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH] = {0};
static int8_t s_fermi_growth_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH]  = {0};

channel_controller_t * const get_channel_controller_pointer_from_index(int8_t const index)
{
	if(false == (index >= 0 && index < MIDI_MAX_CHANNEL_NUMBER) ){
		CHIPTUNE_PRINTF(cDeveloping, "channel_controller = %d, out of range\r\n", index);
		return NULL;
	}
	return &s_channel_controllers[index];
}

/**********************************************************************************/

void reset_channel_controller_midi_control_change_parameters(int8_t const index)
{
	channel_controller_t * const p_channel_controller = &s_channel_controllers[index];
	p_channel_controller->coarse_tuning_value = MIDI_SEVEN_BITS_CENTER_VALUE;
	p_channel_controller->fine_tuning_value = MIDI_FOURTEEN_BITS_CENTER_VALUE;
	p_channel_controller->tuning_in_semitones
			= (float)(p_channel_controller->coarse_tuning_value - MIDI_SEVEN_BITS_CENTER_VALUE)
			+ (p_channel_controller->fine_tuning_value - MIDI_FOURTEEN_BITS_CENTER_VALUE)/(float)MIDI_FOURTEEN_BITS_CENTER_VALUE;

	p_channel_controller->volume = MIDI_SEVEN_BITS_CENTER_VALUE;
	p_channel_controller->expression = INT8_MAX;
	p_channel_controller->pan = MIDI_SEVEN_BITS_CENTER_VALUE;

	p_channel_controller->pitch_wheel_bend_range_in_semitones = MIDI_DEFAULT_PITCH_WHEEL_BEND_RANGE_IN_SEMITONES;
	p_channel_controller->pitch_wheel_bend_in_semitones = 0;

	p_channel_controller->modulation_wheel = 0;
	p_channel_controller->reverb = 0;
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
		= (uint16_t)(p_channel_controller->envelope_release_duration_in_seconds * resolution * tempo/60.0f + 0.5f);
}

/**********************************************************************************/

static void set_growth_curve(int8_t const ** pp_phase_table, int8_t const curve)
{
	switch(curve)
	{
	case ENVELOPE_CURVE_LINEAR:
		*pp_phase_table = &s_linear_growth_table[0];
		break;
	case ENVELOPE_CURVE_EXPONENTIAL:
		*pp_phase_table = &s_exponential_growth_table[0];
		break;
	case ENVELOPE_CURVE_GAUSSIAN:
		*pp_phase_table = &s_gaussian_growth_table[0];
		break;
	case ENVELOPE_CURVE_FERMI:
		*pp_phase_table = &s_fermi_growth_table[0];
		break;
	}
}

/**********************************************************************************/

static void set_decline_curve(int8_t const ** pp_phase_table, int8_t const curve)
{
	switch(curve)
	{
	case ENVELOPE_CURVE_LINEAR:
		*pp_phase_table = &s_linear_decline_table[0];
		break;
	case ENVELOPE_CURVE_EXPONENTIAL:
		*pp_phase_table = &s_exponential_decline_table[0];
		break;
	case ENVELOPE_CURVE_GAUSSIAN:
		*pp_phase_table = &s_gaussian_decline_table[0];
		break;
	case ENVELOPE_CURVE_FERMI:
		*pp_phase_table = &s_fermi_decline_table[0];
		break;
	}
}

/**********************************************************************************/

int set_pitch_channel_parameters(int8_t const index, int8_t const waveform, uint16_t const dutycycle_critical_phase,
									   int8_t const envelope_attack_curve, float const envelope_attack_duration_in_seconds,
									   int8_t const envelope_decay_curve, float const envelope_decay_duration_in_seconds,
									   uint8_t const envelope_sustain_level,
									   int8_t const envelope_release_curve, float const envelope_release_duration_in_seconds,
									   uint8_t const envelope_damper_on_but_note_off_sustain_level,
									   int8_t const envelope_damper_on_but_note_off_sustain_curve,
									   float const envelope_damper_on_but_note_off_sustain_duration_in_seconds)
{
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == index){
		return 1;
	}

	int ret = 0;
	uint32_t const sampling_rate = get_sampling_rate();

	channel_controller_t * const p_channel_controller = &s_channel_controllers[index];
	p_channel_controller->waveform = waveform;
	p_channel_controller->duty_cycle_critical_phase = dutycycle_critical_phase;

	int8_t const ** pp_phase_table = NULL;

	pp_phase_table = &p_channel_controller->p_envelope_attack_table;
	set_growth_curve(pp_phase_table, envelope_attack_curve);
	p_channel_controller->envelope_attack_same_index_number
		= (uint16_t)((sampling_rate * envelope_attack_duration_in_seconds)
					 / (float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH + 0.5);
	if(0 == p_channel_controller->envelope_attack_same_index_number){
		CHIPTUNE_PRINTF(cDeveloping, "WARNING :: envelope_attack_same_index_number is zero\r\n");
		ret = 0x01 << 0;
	}

	pp_phase_table = &p_channel_controller->p_envelope_decay_table;
	set_decline_curve(pp_phase_table, envelope_decay_curve);
	p_channel_controller->envelope_decay_same_index_number
		= (uint16_t)((sampling_rate * envelope_decay_duration_in_seconds)
					 / (float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH + 0.5);

	p_channel_controller->envelope_sustain_level = envelope_sustain_level;

	if(0 == p_channel_controller->envelope_decay_same_index_number){
		if(INT8_MAX + 1 != p_channel_controller->envelope_sustain_level){
			CHIPTUNE_PRINTF(cDeveloping, "WARNING :: envelope_decay_same_index_number is zero"
										 " but envelope_sustain_level is not INT8_MAX + 1 \r\n");
		}
		ret |= 0x01 << 1;
	}

	pp_phase_table = &p_channel_controller->p_envelope_release_table;
	set_decline_curve(pp_phase_table, envelope_release_curve);
	p_channel_controller->envelope_release_duration_in_seconds = envelope_release_duration_in_seconds;
	p_channel_controller->envelope_release_same_index_number
		= (uint16_t)((sampling_rate * envelope_release_duration_in_seconds)
					 / (float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH + 0.5);
	if(0 == p_channel_controller->envelope_release_duration_in_seconds){
		CHIPTUNE_PRINTF(cDeveloping, "WARNING :: envelope_release_same_index_number is zero\r\n");
		ret |= 0x01 << 2;
	}

	pp_phase_table = &p_channel_controller->p_envelope_damper_on_but_note_off_sustain_table;
	set_decline_curve(pp_phase_table, envelope_damper_on_but_note_off_sustain_curve);
	p_channel_controller->envelop_damper_on_but_note_off_sustain_level = envelope_damper_on_but_note_off_sustain_level;
	p_channel_controller->envelope_damper_on_but_note_off_sustain_duration_in_seconds
			= envelope_damper_on_but_note_off_sustain_duration_in_seconds;
	do {
		if(FLT_MAX == p_channel_controller->envelope_damper_on_but_note_off_sustain_duration_in_seconds){
			p_channel_controller->envelope_damper_on_but_note_off_sustain_same_index_number = UINT16_MAX;
			break;
		}
		uint32_t envelope_damper_on_but_note_off_sustain_same_index_number
				= (uint32_t)((sampling_rate * envelope_damper_on_but_note_off_sustain_duration_in_seconds)
					 / (float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH + 0.5);

		if(envelope_damper_on_but_note_off_sustain_same_index_number >= UINT16_MAX){
			envelope_damper_on_but_note_off_sustain_same_index_number = UINT16_MAX - 1;
			float fixed_envelope_damper_on_but_note_off_sustain_duration_in_seconds
					= (envelope_damper_on_but_note_off_sustain_same_index_number * CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH)
					/ (float)sampling_rate;
			CHIPTUNE_PRINTF(cDeveloping, "WARNING :: envelope_damper_on_but_note_off_sustain_duration_in_second = %3.2f,"
										 "greater than UINT16_MAX, set as %3.2f seconds\r\n",
							p_channel_controller->envelope_damper_on_but_note_off_sustain_duration_in_seconds,
							fixed_envelope_damper_on_but_note_off_sustain_duration_in_seconds);
			p_channel_controller->envelope_damper_on_but_note_off_sustain_duration_in_seconds
					= fixed_envelope_damper_on_but_note_off_sustain_duration_in_seconds;
		}

		p_channel_controller->envelope_damper_on_but_note_off_sustain_same_index_number
				= (uint16_t)envelope_damper_on_but_note_off_sustain_same_index_number;
	} while(0);
#if(0)
	if(FLT_MAX == p_channel_controller->envelope_damper_on_but_note_off_sustain_same_index_number){
		CHIPTUNE_PRINTF(cDeveloping, "WARNING :: envelope_damper_on_but_note_off_sustain_duration_in_seconds is forever\r\n");
		ret |= 0x01 << 3;
	}
#endif
	update_channel_controller_envelope_parameters_related_to_tempo(index);
	return ret;
}

/**********************************************************************************/

void reset_channel_controller_all_parameters(int8_t const index)
{
#define DEFAULT_ENVELOPE_ATTACK_DURATION_IN_SECOND	(0.02f)
#define DEFAULT_ENVELOPE_DECAY_DURATION_IN_SECOND	(0.01f)
#define DEFAULT_ENVELOPE_SUSTAIN_LEVEL				(96)
#define DEFAULT_ENVELOPE_RLEASE_DURATION_IN_SECOND	(0.02f)
#define DEFAULT_DAMPER_ON_BUT_NOTE_OFF_LOUDNESS_LEVEL	\
													(24)
#define DEFAULT_ENVELOPE_DAMPER_ON_BUT_NOTE_OFF_SUSTAIN_DURATION_IN_SECOND \
													(8.0f)

	set_pitch_channel_parameters(index, WAVEFORM_TRIANGLE, DUTY_CYLCE_50_CRITICAL_PHASE,
									  ENVELOPE_CURVE_LINEAR, DEFAULT_ENVELOPE_ATTACK_DURATION_IN_SECOND,
									  ENVELOPE_CURVE_FERMI, DEFAULT_ENVELOPE_DECAY_DURATION_IN_SECOND,
									  DEFAULT_ENVELOPE_SUSTAIN_LEVEL,
									  ENVELOPE_CURVE_EXPONENTIAL, DEFAULT_ENVELOPE_RLEASE_DURATION_IN_SECOND,
									  DEFAULT_DAMPER_ON_BUT_NOTE_OFF_LOUDNESS_LEVEL, ENVELOPE_CURVE_LINEAR,
									  DEFAULT_ENVELOPE_DAMPER_ON_BUT_NOTE_OFF_SUSTAIN_DURATION_IN_SECOND);

	channel_controller_t * const p_channel_controller = &s_channel_controllers[index];
	p_channel_controller->instrument		= CHANNEL_CONTROLLER_INSTRUMENT_NOT_SPECIFIED;

	reset_channel_controller_midi_control_change_parameters(index);
}

/**********************************************************************************/

void update_channel_controllers_parameters_related_to_tempo(void)
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

	/*
	 *  fermi
	 *  INT8_MAX * 1/(exp( -gamma*((TABLE_LENGTH - 1) - TABLE_LENGTH/2)) + 1) = 1
	 *   -> gamma = -ln(INT8_MAX - 1)/((TABLE_LENGTH -1) - TABLE_LENGTH/2)
	*/
	const float gamma = -logf(INT8_MAX - 1)
			/ ((CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1) - CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH/2);
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		float exp_x = expf(-gamma * (i - CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH/2));
		s_fermi_decline_table[i] = (int8_t)(INT8_MAX * 1.0f/(exp_x + 1.0f) + 0.5);
	}
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_fermi_growth_table[i] = s_fermi_decline_table[(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1) - i];
	}

}

/**********************************************************************************/

void reset_percussion_all_parameters_from_index(int8_t const index);

void initialize_channel_controllers(void)
{
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_vibrato_phase_table[i] = (int8_t)(INT8_MAX * sinf( 2.0f * (float)M_PI * i / (float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH));
	}
	initialize_envelope_tables();

	for(int8_t i = 0; i < MIDI_MAX_CHANNEL_NUMBER; i++){
		channel_controller_t * const p_channel_controller = &s_channel_controllers[i];
#define	DEFAULT_VIBRATO_MODULATION_IN_SEMITINE		(1)
#define DEFAULT_VIBRATO_RATE						(4)
		p_channel_controller->vibrato_modulation_in_semitones = DEFAULT_VIBRATO_MODULATION_IN_SEMITINE;
		p_channel_controller->p_vibrato_phase_table = &s_vibrato_phase_table[0];
		p_channel_controller->vibrato_same_index_number
			= (uint16_t)(get_sampling_rate()/CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH/(float)DEFAULT_VIBRATO_RATE);
#define	DEFAULT_MAX_CHORUS_PITCH_BEND_IN_SEMITONE	(0.25f)
		p_channel_controller->max_pitch_chorus_bend_in_semitones = DEFAULT_MAX_CHORUS_PITCH_BEND_IN_SEMITONE;
	}

#define DEFAULT_PERCUSSION_RELEASE_TIME_SECONDS		(0.03f)
	channel_controller_t * const p_channel_controller = &s_channel_controllers[MIDI_PERCUSSION_INSTRUMENT_CHANNEL];
	int8_t const ** pp_phase_table = NULL;
	pp_phase_table = &p_channel_controller->p_envelope_release_table;
	set_decline_curve(pp_phase_table, ENVELOPE_CURVE_EXPONENTIAL);
	p_channel_controller->envelope_release_duration_in_seconds = DEFAULT_PERCUSSION_RELEASE_TIME_SECONDS;
	p_channel_controller->envelope_release_same_index_number
		= (uint16_t)((get_sampling_rate() * DEFAULT_PERCUSSION_RELEASE_TIME_SECONDS)
					 / (float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH + 0.5);
	update_channel_controller_envelope_parameters_related_to_tempo(MIDI_PERCUSSION_INSTRUMENT_CHANNEL);

	for(int i = PERCUSSION_CODE_MIN; i <= PERCUSSION_CODE_MAX; i++){
		reset_percussion_all_parameters_from_index(i);
	}

	for(int8_t i = 0; i < MIDI_MAX_CHANNEL_NUMBER; i++){
		reset_channel_controller_all_parameters(i);
	}
}

/**********************************************************************************/

percussion_t s_percussion[PERCUSSION_CODE_MAX - PERCUSSION_CODE_MIN + 1];

percussion_t * const get_percussion_pointer_from_index(int8_t const index)
{
	if(false == (PERCUSSION_CODE_MIN <= index && PERCUSSION_CODE_MAX >= index)){
		return &s_percussion[SNARE_DRUM_1 - PERCUSSION_CODE_MIN];
	}
	return &s_percussion[index - PERCUSSION_CODE_MIN];
}

/**********************************************************************************/

void reset_percussion_all_parameters_from_index(int8_t const index)
{
	if(false == (PERCUSSION_CODE_MIN <= index && PERCUSSION_CODE_MAX >= index)){
		CHIPTUNE_PRINTF(cDeveloping, "percussion = %d, out of range\r\n", index);
		return;
	}

	float start_frequency = 150;
	float end_frequency = 120;
	float total_druation_time_in_second = 0.3f;
	float waveform_duration_time_in_second[3];

	percussion_t * const p_percussion = get_percussion_pointer_from_index(index);
	p_percussion->waveform[0] = WAVEFORM_NOISE;
	waveform_duration_time_in_second[0] = 0.02f;
	p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
	waveform_duration_time_in_second[1] = 0.1f;
	p_percussion->waveform[2] = WAVEFORM_NOISE;
	waveform_duration_time_in_second[2] = total_druation_time_in_second
			- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];

	p_percussion->p_modulation_envelope_table = s_linear_decline_table;
	p_percussion->p_amplitude_envelope_table = s_exponential_decline_table;
	p_percussion->is_implemented = false;

//http://kometbomb.net/2011/10/11/chiptune-drums/
	switch(index){
	case BASS_DRUM:
	case KICK_DRUM:
		start_frequency = 80;
		end_frequency = 50;
		total_druation_time_in_second = 1.2f;
		//p_oscillator->p_percussion_modulation_table = s_linear_decline_table;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.04f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.3f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = 0.05f;
		p_percussion->waveform[3] = WAVEFORM_SQUARE;
		p_percussion->is_implemented = true;
		break;
	case SIDE_STICK:
		p_percussion->p_amplitude_envelope_table = s_exponential_decline_table;
		start_frequency = 350;
		end_frequency = 345;
		total_druation_time_in_second = 0.4f;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_SQUARE;
		waveform_duration_time_in_second[1] = 0.25f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second
				- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case SNARE_DRUM_1:
	case SNARE_DRUM_2:
		start_frequency = 170;
		end_frequency = 165;
		total_druation_time_in_second = 0.35f;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.15f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second
				- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case OPEN_HI_HAT:
		start_frequency = 7600;
		end_frequency = 7400;
		total_druation_time_in_second = 0.4f;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.12f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second
				-waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case CLOSED_HI_HAT:
		start_frequency = 6800;
		end_frequency = 6700;
		total_druation_time_in_second = 0.3f;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.05f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second
				- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case RIDE_CYMBAL_1:
	case RIDE_CYMBAL_2:
		start_frequency = 7200;
		end_frequency = 7000;
		total_druation_time_in_second = 0.4f;
		p_percussion->p_amplitude_envelope_table = s_exponential_decline_table;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.12f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second
				- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case PADEL_HI_HAT:
		start_frequency = 6400;
		end_frequency = 6300;
		total_druation_time_in_second = 0.3f;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.15f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second
				- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case CRASH_CYMBAL_1:
	case CRASH_CYMBAL_2:
		start_frequency = 100;
		end_frequency = 90;
		total_druation_time_in_second = 0.7f;
		p_percussion->p_amplitude_envelope_table = s_exponential_decline_table;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_SQUARE;
		waveform_duration_time_in_second[1] = 0.3f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second;
		p_percussion->is_implemented = true;
		break;
	case LOW_FLOOR_TOM:
		start_frequency = 220;
		end_frequency = 210;
		total_druation_time_in_second = 0.5f;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.025f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[1] = total_druation_time_in_second
				- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case HIGH_FLOOR_TOM:
		start_frequency = 240;
		end_frequency = 230;
		total_druation_time_in_second = 0.5f;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.3f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[1] = total_druation_time_in_second
				- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case LOW_TOM:
		start_frequency = 250;
		end_frequency = 240;
		total_druation_time_in_second = 0.4f;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.2f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[1] = total_druation_time_in_second
				- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case LOW_MID_TOM:
		start_frequency = 270;
		end_frequency = 260;
		total_druation_time_in_second = 0.35f;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.15f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second
				- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case HIGH_MID_TOM:
		start_frequency = 290;
		end_frequency = 280;
		total_druation_time_in_second = 0.35f;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.15f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second
				- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case HIGH_TOM:
		start_frequency = 310;
		end_frequency = 300;
		total_druation_time_in_second = 0.3f;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_TRIANGLE;
		waveform_duration_time_in_second[1] = 0.18f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[1] = total_druation_time_in_second
				- waveform_duration_time_in_second[1] - waveform_duration_time_in_second[0];
		p_percussion->is_implemented = true;
		break;
	case CHINESE_CYMBAL:
		start_frequency = 85;
		end_frequency = 70;
		total_druation_time_in_second = 1.5f;
		p_percussion->p_amplitude_envelope_table = s_exponential_decline_table;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_SQUARE;
		waveform_duration_time_in_second[1] = 0.3f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second;
		p_percussion->is_implemented = true;
		break;
	case TAMBOURINE:
		start_frequency = 210;
		end_frequency = 200;
		total_druation_time_in_second = 0.4f;
		p_percussion->p_amplitude_envelope_table = s_exponential_decline_table;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_SQUARE;
		waveform_duration_time_in_second[1] = 0.35f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second;
		p_percussion->is_implemented = true;
		break;
	case SPLASH_CYMBAL:
		start_frequency = 60;
		end_frequency = 50;
		total_druation_time_in_second = 2.0f;
		p_percussion->p_amplitude_envelope_table = s_exponential_decline_table;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_SQUARE;
		waveform_duration_time_in_second[1] = 0.5f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second;
		p_percussion->is_implemented = true;
		break;
	case CASTANETS:
		start_frequency = 12000;
		end_frequency = 12000;
		total_druation_time_in_second = 0.08f;
		p_percussion->p_amplitude_envelope_table = s_exponential_decline_table;
		p_percussion->waveform[0] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[0] = 0.02f;
		p_percussion->waveform[1] = WAVEFORM_SQUARE;
		waveform_duration_time_in_second[1] = 0.05f;
		p_percussion->waveform[2] = WAVEFORM_NOISE;
		waveform_duration_time_in_second[2] = total_druation_time_in_second;
		p_percussion->is_implemented = true;
		break;
	default:
		//CHIPTUNE_PRINTF(cMidiNote, "percussion note = %d NOT IMPLEMENT YET\r\n", index);
		break;
	}

	float remain_druation_time_in_second = total_druation_time_in_second;
	uint32_t const sampling_rate = get_sampling_rate();
	p_percussion->delta_phase = (uint16_t)((UINT16_MAX + 1) * start_frequency / get_sampling_rate());
	p_percussion->max_delta_modulation_phase =
			(uint16_t)((UINT16_MAX + 1) * end_frequency / get_sampling_rate()) - p_percussion->delta_phase;

	p_percussion->waveform_duration_sample_number[0]
			= (uint32_t)(waveform_duration_time_in_second[0] * sampling_rate + 0.5f);
	remain_druation_time_in_second -= waveform_duration_time_in_second[0];

	p_percussion->waveform_duration_sample_number[1]
			= (uint32_t)(waveform_duration_time_in_second[1] * sampling_rate + 0.5f);
	remain_druation_time_in_second -= waveform_duration_time_in_second[1];

	p_percussion->waveform_duration_sample_number[2]
			= (uint32_t)(waveform_duration_time_in_second[2] * sampling_rate + 0.5f);
	remain_druation_time_in_second -= waveform_duration_time_in_second[2];

	p_percussion->waveform_duration_sample_number[3]
			= (uint32_t)(remain_druation_time_in_second * sampling_rate + 0.5);

	p_percussion->envelope_same_index_number =
			(uint32_t)((total_druation_time_in_second * sampling_rate)/(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH) + 0.5);
}
