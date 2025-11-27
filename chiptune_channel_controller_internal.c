#include <stddef.h>
#include <float.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"

#include "chiptune_channel_controller_internal.h"

static channel_controller_t s_channel_controllers[MIDI_MAX_CHANNEL_NUMBER];

static int8_t s_vibrato_lookup_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH] = {0};

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
	p_channel_controller->coarse_tuning_value = (midi_value_t)MIDI_SEVEN_BITS_CENTER_VALUE;
	p_channel_controller->fine_tuning_value = MIDI_FOURTEEN_BITS_CENTER_VALUE;
	p_channel_controller->tuning_in_semitones
			= (float)(p_channel_controller->coarse_tuning_value - MIDI_SEVEN_BITS_CENTER_VALUE)
			+ (p_channel_controller->fine_tuning_value - MIDI_FOURTEEN_BITS_CENTER_VALUE)/(float)MIDI_FOURTEEN_BITS_CENTER_VALUE;

	p_channel_controller->volume = (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(MIDI_SEVEN_BITS_CENTER_VALUE);
	p_channel_controller->pressure = (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(0);
	p_channel_controller->expression = (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(INT8_MAX);
	p_channel_controller->pan = (midi_value_t)MIDI_SEVEN_BITS_CENTER_VALUE;

	p_channel_controller->pitch_wheel_bend_range_in_semitones
			= (midi_value_t)MIDI_DEFAULT_PITCH_WHEEL_BEND_RANGE_IN_SEMITONES;
	p_channel_controller->pitch_wheel_bend_in_semitones = 0.0f;

	p_channel_controller->modulation_wheel = NORMALIZE_MIDI_LEVEL(0);
	p_channel_controller->reverb = (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(0);
	p_channel_controller->chorus = (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(0);
	p_channel_controller->is_damper_pedal_on = false;
}

/**********************************************************************************/

static void update_channel_controller_envelope_parameters_related_to_playing_tempo(int8_t const index)
{
	uint32_t const resolution = get_resolution();
	float const playing_tempo = get_playing_tempo();
	channel_controller_t * const p_channel_controller = &s_channel_controllers[index];

	p_channel_controller->envelope_release_tick_number
		= (uint16_t)(p_channel_controller->envelope_release_duration_in_seconds * resolution * playing_tempo/60.0f + 0.5f);
}

/**********************************************************************************/

static void set_growth_curve(int8_t const ** pp_phase_table, int8_t const curve)
{
	switch(curve)
	{
	case EnvelopeCurveLinear:
		*pp_phase_table = &s_linear_growth_table[0];
		break;
	case EnvelopeCurveExponential:
		*pp_phase_table = &s_exponential_growth_table[0];
		break;
	case EnvelopeCurveGaussian:
		*pp_phase_table = &s_gaussian_growth_table[0];
		break;
	case EnvelopeCurveFermi:
		*pp_phase_table = &s_fermi_growth_table[0];
		break;
	}
}

/**********************************************************************************/

static void set_decline_curve(int8_t const ** pp_phase_table, int8_t const curve)
{
	switch(curve)
	{
	case EnvelopeCurveLinear:
		*pp_phase_table = &s_linear_decline_table[0];
		break;
	case EnvelopeCurveExponential:
		*pp_phase_table = &s_exponential_decline_table[0];
		break;
	case EnvelopeCurveGaussian:
		*pp_phase_table = &s_gaussian_decline_table[0];
		break;
	case EnvelopeCurveFermi:
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
	if(MIDI_PERCUSSION_CHANNEL == index){
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

	p_channel_controller->envelope_sustain_level
			= (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(envelope_sustain_level);

	if(0 == p_channel_controller->envelope_decay_same_index_number){
		if(INT8_MAX != p_channel_controller->envelope_sustain_level){
			CHIPTUNE_PRINTF(cDeveloping, "WARNING :: envelope_decay_same_index_number is zero"
										 " but envelope_sustain_level is not INT8_MAX\r\n");
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
	p_channel_controller->envelop_damper_on_but_note_off_sustain_level
			= (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(envelope_damper_on_but_note_off_sustain_level);
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
	update_channel_controller_envelope_parameters_related_to_playing_tempo(index);
	return ret;
}

/**********************************************************************************/

static void reset_channel_controller_all_parameters(int8_t const index)
{
#define DEFAULT_ENVELOPE_ATTACK_DURATION_IN_SECOND	(0.02f)
#define DEFAULT_ENVELOPE_DECAY_DURATION_IN_SECOND	(0.01f)
#define DEFAULT_ENVELOPE_SUSTAIN_LEVEL				(96)
#define DEFAULT_ENVELOPE_RLEASE_DURATION_IN_SECOND	(0.03f)
#define DEFAULT_DAMPER_ON_BUT_NOTE_OFF_LOUDNESS_LEVEL	\
													(24)
#define DEFAULT_ENVELOPE_DAMPER_ON_BUT_NOTE_OFF_SUSTAIN_DURATION_IN_SECOND \
													(8.0f)

	set_pitch_channel_parameters(index, WaveformTriangle, WaveformDutyCycle50,
									  EnvelopeCurveLinear, DEFAULT_ENVELOPE_ATTACK_DURATION_IN_SECOND,
									  EnvelopeCurveFermi, DEFAULT_ENVELOPE_DECAY_DURATION_IN_SECOND,
									  DEFAULT_ENVELOPE_SUSTAIN_LEVEL,
									  EnvelopeCurveExponential, DEFAULT_ENVELOPE_RLEASE_DURATION_IN_SECOND,
									  DEFAULT_DAMPER_ON_BUT_NOTE_OFF_LOUDNESS_LEVEL, EnvelopeCurveLinear,
									  DEFAULT_ENVELOPE_DAMPER_ON_BUT_NOTE_OFF_SUSTAIN_DURATION_IN_SECOND);

	channel_controller_t * const p_channel_controller = &s_channel_controllers[index];
	p_channel_controller->instrument = CHANNEL_CONTROLLER_INSTRUMENT_UNUSED_CHANNEL;

	reset_channel_controller_midi_control_change_parameters(index);
}

/**********************************************************************************/

void reset_all_channel_controllers()
{
	for(int8_t voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		reset_channel_controller_all_parameters(voice);
	}
}

/**********************************************************************************/

void update_channel_controllers_parameters_related_to_playing_tempo(void)
{
	for(int8_t voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		update_channel_controller_envelope_parameters_related_to_playing_tempo(voice);
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
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_exponential_decline_table[i] = (int8_t)(INT8_MAX * expf( -alpha * i) + 0.5);
	}
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_exponential_growth_table[i]
				= s_exponential_decline_table[(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1) - i];
	}

	/*
	 * gaussian
	 *  INT8_MAX * exp(-beta * (TABLE_LENGTH -1)**2) = 1 -> beta = ln(INT8_MAX)/(TABLE_LENGTH -1)**2
	*/
	const float beta = logf(INT8_MAX)/powf((float)(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1), 2.0f);
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_gaussian_decline_table[i] = (int8_t)(INT8_MAX * expf(-beta * i * i) + 0.5);
	}
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_gaussian_growth_table[i] = s_gaussian_decline_table[(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1) - i];
	}

	/*
	 *  fermi
	 *  A * 1/(exp( -gamma*((TABLE_LENGTH - 1) - TABLE_LENGTH/2)) + 1) = 1  -> constraint A
	 *  A * 1/(exp( -gamma*((0) - TABLE_LENGTH/2)) + 1) = INT8_MAX  -> -> constraint A
	 *   -> gamma = -ln(A - 1)/((TABLE_LENGTH -1) - TABLE_LENGTH/2)
	 *   -> A = INT8_MAX*(1 + (A - 1)**(-TABLE_LENGTH/(TABLE_LENGTH - 2)) -> SCF solving (start A = 128) → A ≈ 127.85633
	 *
	 * For TABLE_LENGTH = 64 :
	 * SCF solving (start A = 128) → A ~ 127.85633
	 * Here uses A = INT8_MAX + 1
	 *
	 *《論語．子罕》：「有鄙夫問於我，空空如也。我叩其兩端而竭焉。」  -> taking the two extremes:
	 * As TABLE_LENGTH = 1 or 2 : degenerate, meaningless
	 * As TABLE_LENGTH -> infinity :
	 * A/INT8_MAX = 1 + 1/(A - 1)
	 * -> A*(A - 1) = ((A - 1) + 1)*INT8_MAX
	 * -> A*(A - (INT8_MAX + 1)) = 0
	 * thus A = INT8_MAX + 1
	 *
	 * => In practice always set A = INT8_MAX + 1
	*/
	const float gamma = -logf((INT8_MAX + 1) - 1)
			/ ((CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1) - CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH/2);
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		float exp_x = expf(-gamma * (i - CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH/2));
		s_fermi_decline_table[i] = (int8_t)((INT8_MAX + 1) * 1.0f/(exp_x + 1.0f) + 0.5);
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
		s_vibrato_lookup_table[i]
				= (int8_t)(INT8_MAX * sinf( 2.0f * (float)M_PI * i / (float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH));
	}
	initialize_envelope_tables();

	for(int8_t voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		channel_controller_t * const p_channel_controller = &s_channel_controllers[voice];
#define DEFAULT_VIBRATO_DEPTH_IN_SEMITONES			(1)
#define DEFAULT_VIBRATO_RATE_IN_HZ					(4)
		p_channel_controller->vibrato_depth_in_semitones = DEFAULT_VIBRATO_DEPTH_IN_SEMITONES;
		p_channel_controller->p_vibrato_phase_table = &s_vibrato_lookup_table[0];
		p_channel_controller->vibrato_same_index_number
			= (uint16_t)(get_sampling_rate()/CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH/(float)DEFAULT_VIBRATO_RATE_IN_HZ);
	}

#define DEFAULT_PERCUSSION_RELEASE_TIME_SECONDS		(0.03f)
	channel_controller_t * const p_channel_controller = &s_channel_controllers[MIDI_PERCUSSION_CHANNEL ];
	int8_t const ** pp_phase_table = NULL;
	pp_phase_table = &p_channel_controller->p_envelope_release_table;
	set_decline_curve(pp_phase_table, EnvelopeCurveExponential);
	p_channel_controller->envelope_release_duration_in_seconds = DEFAULT_PERCUSSION_RELEASE_TIME_SECONDS;
	p_channel_controller->envelope_release_same_index_number
		= (uint16_t)((get_sampling_rate() * DEFAULT_PERCUSSION_RELEASE_TIME_SECONDS)
					 / (float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH + 0.5);
	update_channel_controller_envelope_parameters_related_to_playing_tempo(MIDI_PERCUSSION_CHANNEL );

	for(int8_t i = PERCUSSION_CODE_MIN; i <= PERCUSSION_CODE_MAX; i++){
		reset_percussion_all_parameters_from_index(i);
	}

	reset_all_channel_controllers();
}

/**********************************************************************************/

percussion_t s_percussion[PERCUSSION_CODE_MAX - PERCUSSION_CODE_MIN + 1];

percussion_t * const get_percussion_pointer_from_index(int8_t const index)
{
	if(false == (PERCUSSION_CODE_MIN <= index && PERCUSSION_CODE_MAX >= index)){
		return NULL;
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

	float start_frequency = 0.0;
	float end_frequency = 0.0;
	float total_druation_time_in_second = 0.0;
	float waveform_duration_time_in_second[MAX_PERCUSSION_WAVEFORM_SEGMENT_NUMBER - 1] = {0.0};

	percussion_t * const p_percussion = get_percussion_pointer_from_index(index);
	p_percussion->p_phase_sweep_table = s_linear_decline_table;
	p_percussion->p_amplitude_envelope_table = s_exponential_decline_table;
	p_percussion->is_implemented = false;

	//http://kometbomb.net/2011/10/11/chiptune-drums/
	int last_waveform_segment_index = 0;
	switch(index){
	case AcousticBassDrum:
	case BassDrum1:
		start_frequency = 80;
		end_frequency = 50;
		total_druation_time_in_second = 1.2f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.04f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.3f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.05f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformSquare;
		p_percussion->is_implemented = true;
		break;
	case SideStick:
		start_frequency = 520;
		end_frequency = 460;
		total_druation_time_in_second = 0.12f;
		p_percussion->p_phase_sweep_table = s_exponential_decline_table;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.01f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.06f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case AcousticSnare:
	case ElectricSnare:
		start_frequency = 180;
		end_frequency = 165;
		total_druation_time_in_second = 0.35f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.015f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.15f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case HandClap: /*looser snare*/
		start_frequency = 170;
		end_frequency = 150;
		total_druation_time_in_second = 0.45f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.12f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case LowFloorTom:
		start_frequency = 150;
		end_frequency = 130;
		total_druation_time_in_second = 0.55f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.28f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case ClosedHiHat:
		start_frequency = 6800;
		end_frequency = 6700;
		total_druation_time_in_second = 0.25f;
		p_percussion->p_amplitude_envelope_table = s_gaussian_decline_table;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.015f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.025f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case HighFloorTom:
		start_frequency = 170;
		end_frequency = 160;
		total_druation_time_in_second = 0.5f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.25f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case PedalHiHat:
		start_frequency = 6400;
		end_frequency = 6300;
		total_druation_time_in_second = 0.16f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.008f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.03f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case LowTom:
		start_frequency = 185;
		end_frequency = 170;
		total_druation_time_in_second = 0.4f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.20f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case OpenHiHat:
		start_frequency = 7300;
		end_frequency = 7100;
		total_druation_time_in_second = 0.3f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.012f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case LowMidTom:
		start_frequency = 205;
		end_frequency = 195;
		total_druation_time_in_second = 0.40f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.18f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case HighMidTom:
		start_frequency = 225;
		end_frequency = 210;
		total_druation_time_in_second = 0.35f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.15f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case CrashCymbal1:
	case CrashCymbal2:
		start_frequency = 8400;
		end_frequency = 8200;
		total_druation_time_in_second = 1.7f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.30f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case HighTom:
		start_frequency = 245;
		end_frequency = 225;
		total_druation_time_in_second = 0.3f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.15f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case RideCymbal1:
	case RideCymbal2:
		start_frequency = 8000;
		end_frequency = 7800;
		total_druation_time_in_second = 3.4f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.33f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case ChineseCymbal:
		start_frequency = 7500;
		end_frequency = 7300;
		total_druation_time_in_second = 2.0f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.40f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case Tambourine:
		start_frequency = 210;
		end_frequency = 200;
		total_druation_time_in_second = 0.4f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformSquare;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.35f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case SplashCymbal:
		start_frequency = 8600;
		end_frequency = 8500;
		total_druation_time_in_second = 0.7f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.23f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	case LowBongo:
		start_frequency = 180;
		end_frequency = 250;
		p_percussion->p_phase_sweep_table = s_fermi_decline_table;
		total_druation_time_in_second = 0.10f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.005f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		p_percussion->is_implemented = true;
		break;
	case OpenHighConga:
		start_frequency = 230;
		end_frequency = 220;
		total_druation_time_in_second = 0.7f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.008f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.45f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	/*GM2*/
	case Castanets:
		start_frequency = 12000;
		end_frequency = 12000;
		total_druation_time_in_second = 0.08f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformSquare;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.05f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		p_percussion->is_implemented = true;
		break;
	default:
		//CHIPTUNE_PRINTF(cMidiNote, "percussion note = %d NOT IMPLEMENT YET\r\n", index);
		start_frequency = 150;
		end_frequency = 120;
		total_druation_time_in_second = 0.3f;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.02f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformTriangle;
		waveform_duration_time_in_second[last_waveform_segment_index] = 0.15f; last_waveform_segment_index += 1;
		p_percussion->waveform[last_waveform_segment_index] = WaveformNoise;
		break;
	}
	if(MAX_PERCUSSION_WAVEFORM_SEGMENT_NUMBER - 1 < last_waveform_segment_index){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: last_waveform_segment_index = %d >"
									 " MAX_PERCUSSION_WAVEFORM_SEGMENT_NUMBER - 1 in %s",
						last_waveform_segment_index, __func__);
		last_waveform_segment_index = MAX_PERCUSSION_WAVEFORM_SEGMENT_NUMBER - 1;
	}

	p_percussion->base_phase_increment = (uint16_t)((UINT16_MAX + 1) * start_frequency / get_sampling_rate());
	p_percussion->max_phase_sweep_delta =
			(int16_t)((UINT16_MAX + 1) * end_frequency / get_sampling_rate()) - p_percussion->base_phase_increment;

	float remain_duration_time_in_second = total_druation_time_in_second;
	for(int i = 0; i < last_waveform_segment_index; i++){
		p_percussion->waveform_segment_duration_sample_number[i]
				= (uint32_t)(waveform_duration_time_in_second[i] * get_sampling_rate() + 0.5f);
		remain_duration_time_in_second -= waveform_duration_time_in_second[i];
	}
	if(remain_duration_time_in_second < 0.0){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: remain_duration_time_in_second < 0.0 in %s",
						__func__);
		remain_duration_time_in_second = 0.0;
	}
	p_percussion->waveform_segment_duration_sample_number[last_waveform_segment_index]
			= (uint32_t)(remain_duration_time_in_second * get_sampling_rate() + 0.5);

	p_percussion->envelope_same_index_number =
			(uint32_t)((total_druation_time_in_second * get_sampling_rate())/(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH) + 0.5);
}
