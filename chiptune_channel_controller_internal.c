#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"

#include "chiptune_channel_controller_internal.h"


static channel_controller_t s_channel_controllers[MIDI_MAX_CHANNEL_NUMBER];

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

void reset_channel_controller_from_index(int8_t const index)
{
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
			= (uint16_t)(get_sampling_rate()/VIBRATO_PHASE_TABLE_LENGTH/(float)DEFAULT_VIBRATO_FREQUENCY);

#define	DEFAULT_MAX_CHORUS_PITCH_BEND_IN_SEMITONE	(0.25f)
	p_channel_controller->chorus = 0;
	p_channel_controller->max_pitch_chorus_bend_in_semitones = DEFAULT_MAX_CHORUS_PITCH_BEND_IN_SEMITONE;

	p_channel_controller->registered_parameter_number = MIDI_CC_RPN_NULL;
	p_channel_controller->registered_parameter_value = 0;
}

/**********************************************************************************/

void reset_all_reset_channel_controller(void)
{
	for(int8_t i = 0; i < MIDI_MAX_CHANNEL_NUMBER; i++){
		reset_channel_controller_from_index(i);
	}
}

