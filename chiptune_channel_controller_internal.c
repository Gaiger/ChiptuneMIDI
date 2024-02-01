#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"

#include "chiptune_channel_controller_internal.h"


static channel_controller_t s_channel_controllers[MIDI_MAX_CHANNEL_NUMBER];

static int8_t s_linear_decline_table[ENVELOPE_TABLE_LENGTH];
static int8_t s_linear_growth_table[ENVELOPE_TABLE_LENGTH];

/**********************************************************************************/


channel_controller_t * const get_channel_controller_pointer_from_index(int8_t const index)
{
	if(false == (index >= 0 && index < MIDI_MAX_CHANNEL_NUMBER)){
		CHIPTUNE_PRINTF(cDeveloping, "channel_controller = %d, out of range\r\n");
		return NULL;
	}
	return &s_channel_controllers[index];
}

/**********************************************************************************/

void update_channel_controller_envelope(int8_t const index)
{
	uint32_t const sampling_rate = get_sampling_rate();
	uint32_t const resolution = get_resolution();
	float tempo = get_tempo();

	channel_controller_t * const p_channel_controller = &s_channel_controllers[index];

#define DEFAULT_ENVELOPE_ATTACK_DURATION_IN_SECOND	(0.02f)
	p_channel_controller->envelope_attack_tick_number
		= (uint16_t)(DEFAULT_ENVELOPE_ATTACK_DURATION_IN_SECOND * resolution * tempo/60.0f + 0.5);
	p_channel_controller->envelope_attack_same_index_number
				= (uint16_t)((sampling_rate * DEFAULT_ENVELOPE_ATTACK_DURATION_IN_SECOND)/(float)ENVELOPE_TABLE_LENGTH + 0.5);

#define DEFAULT_ENVELOPE_DECAY_DURATION_IN_SECOND	(0.01f)
	p_channel_controller->envelope_decay_tick_number
		= (uint16_t)(DEFAULT_ENVELOPE_DECAY_DURATION_IN_SECOND * resolution * tempo/60.0f + 0.5);
	p_channel_controller->envelope_decay_same_index_number
				= (uint16_t)((sampling_rate * DEFAULT_ENVELOPE_DECAY_DURATION_IN_SECOND)/(float)ENVELOPE_TABLE_LENGTH + 0.5);

#define DEFAULT_ENVELOPE_SUSTAIN_LEVEL				(7)
	p_channel_controller->envelope_sustain_level = DEFAULT_ENVELOPE_SUSTAIN_LEVEL;

#define DEFAULT_ENVELOPE_RLEASE_DURATION_IN_SECOND	(0.03f)
	p_channel_controller->envelope_release_tick_number
		= (uint16_t)(DEFAULT_ENVELOPE_RLEASE_DURATION_IN_SECOND * resolution * tempo/60.0f + 0.5f);

	p_channel_controller->envelope_release_same_index_number
				= (uint16_t)((sampling_rate * DEFAULT_ENVELOPE_RLEASE_DURATION_IN_SECOND)/(float)ENVELOPE_TABLE_LENGTH + 0.5);

}

/**********************************************************************************/

void reset_channel_controller_from_index(int8_t const index)
{
	uint32_t const sampling_rate = get_sampling_rate();

	channel_controller_t * const p_channel_controller = &s_channel_controllers[index];
	p_channel_controller->tuning_in_semitones = 0;

	p_channel_controller->max_volume = MIDI_CC_CENTER_VALUE;
	p_channel_controller->playing_volume = (p_channel_controller->max_volume * INT8_MAX)/INT8_MAX;
	p_channel_controller->pan = MIDI_CC_CENTER_VALUE;

	p_channel_controller->waveform = WAVEFORM_TRIANGLE;

	p_channel_controller->pitch_wheel_bend_range_in_semitones = MIDI_DEFAULT_PITCH_WHEEL_BEND_RANGE_IN_SEMITONES;
	p_channel_controller->pitch_wheel = MIDI_PITCH_WHEEL_CENTER;

	p_channel_controller->is_damper_pedal_on = false;

#define	DEFAULT_VIBRATO_MODULATION_IN_SEMITINE		(1)
#define DEFAULT_VIBRATO_FREQUENCY					(4)
	p_channel_controller->modulation_wheel = 0;
	p_channel_controller->vibrato_modulation_in_semitone = DEFAULT_VIBRATO_MODULATION_IN_SEMITINE;
	p_channel_controller->vibrato_same_index_number
			= (uint16_t)(sampling_rate/VIBRATO_PHASE_TABLE_LENGTH/(float)DEFAULT_VIBRATO_FREQUENCY);

#define	DEFAULT_MAX_CHORUS_PITCH_BEND_IN_SEMITONE	(0.25f)
	p_channel_controller->chorus = 0;
	p_channel_controller->max_pitch_chorus_bend_in_semitones = DEFAULT_MAX_CHORUS_PITCH_BEND_IN_SEMITONE;

	p_channel_controller->p_envelope_attack_table = &s_linear_growth_table[0];
	p_channel_controller->p_envelope_decay_table  = &s_linear_decline_table[0];
	p_channel_controller->p_envelope_release_table = &s_linear_decline_table[0];
	update_channel_controller_envelope(index);
}

/**********************************************************************************/

void update_all_channel_controllers_envelope(void)
{
	for(int8_t i = 0; i < MIDI_MAX_CHANNEL_NUMBER; i++){
		update_channel_controller_envelope(i);
	}
}

/**********************************************************************************/

static void initialize_envelope_tables(void)
{
	for(int16_t i = 0; i < ENVELOPE_TABLE_LENGTH; i++){
		s_linear_growth_table[i] = (int8_t)(INT8_MAX * (i/(float)ENVELOPE_TABLE_LENGTH));
	}

	for(int16_t i = 0; i < ENVELOPE_TABLE_LENGTH; i++){
		s_linear_decline_table[i] = (int8_t)(INT8_MAX * ((ENVELOPE_TABLE_LENGTH - i)/(float)ENVELOPE_TABLE_LENGTH));
	}
}

/**********************************************************************************/

void initialize_channel_controller(void)
{
	initialize_envelope_tables();
	for(int8_t i = 0; i < MIDI_MAX_CHANNEL_NUMBER; i++){
		reset_channel_controller_from_index(i);
	}
}
