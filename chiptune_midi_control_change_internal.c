#include <stdio.h>
#include <string.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"

#include "chiptune_channel_controller_internal.h"
#include "chiptune_oscillator_internal.h"
#include "chiptune_event_internal.h"
#include "chiptune_midi_effect_internal.h"

#include "chiptune_midi_control_change_internal.h"


static inline void process_modulation_wheel(uint32_t const tick, int8_t const voice, int8_t const value)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_MODULATION_WHEEL(%d) :: voice = %d, value = %d\r\n",
					tick, MIDI_CC_MODULATION_WHEEL, voice, value);
	get_channel_controller_pointer_from_index(voice)->modulation_wheel = value;
}

/**********************************************************************************/

#define SEVEN_BITS_VALID(VALUE)						((0x7F) & (VALUE))

static void process_cc_registered_parameter(uint32_t const tick, int8_t const voice)
{
	(void)tick;
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	switch(p_channel_controller->registered_parameter_number)
	{
	case MIDI_CC_RPN_PITCH_BEND_SENSITIVY:
		p_channel_controller->pitch_wheel_bend_range_in_semitones = SEVEN_BITS_VALID(p_channel_controller->registered_parameter_value >> 8);
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_PITCH_BEND_SENSITIVY(%d) :: voice = %d, semitones = %d\r\n",
						MIDI_CC_RPN_PITCH_BEND_SENSITIVY,
						voice, p_channel_controller->pitch_wheel_bend_range_in_semitones);
		if(0 != SEVEN_BITS_VALID(p_channel_controller->registered_parameter_value)){
			CHIPTUNE_PRINTF(cMidiSetup, "----  MIDI_CC_RPN_PITCH_BEND_SENSITIVY(%d) :: voice = %d, cents = %d (%s)\r\n",
						MIDI_CC_RPN_PITCH_BEND_SENSITIVY,
						voice, SEVEN_BITS_VALID(p_channel_controller->registered_parameter_number), "(NOT IMPLEMENTED YET)");
		}
		break;
	case MIDI_CC_RPN_CHANNEL_FINE_TUNING:
		CHIPTUNE_PRINTF(cMidiSetup, "----  MIDI_CC_RPN_CHANNEL_FINE_TUNING(%d) :: voice = %d, value = %d %s\r\n",
						voice, p_channel_controller->registered_parameter_number, p_channel_controller->registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_CHANNEL_COARSE_TUNING:
		p_channel_controller->tuning_in_semitones = SEVEN_BITS_VALID(p_channel_controller->registered_parameter_value >> 8) - MIDI_CC_CENTER_VALUE;
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_CHANNEL_COARSE_TUNING(%d) :: voice = %d, tuning in semitones = %+d\r\n",
						voice, p_channel_controller->registered_parameter_number, p_channel_controller->tuning_in_semitones);
		break;
	case MIDI_CC_RPN_TURNING_PROGRAM_CHANGE:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_TURNING_PROGRAM_CHANGE(%d) :: voice = %d, value = %d, value = %d %s\r\n",
						voice, p_channel_controller->registered_parameter_number, p_channel_controller->registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_TURNING_BANK_SELECT:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_TURNING_BANK_SELECT(%d) :: voice = %d, value = %d %s\r\n",
						voice, p_channel_controller->registered_parameter_number, p_channel_controller->registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_MODULATION_DEPTH_RANGE:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_MODULATION_DEPTH_RANGE(%d) :: voice = %d %s\r\n",
						voice, p_channel_controller->registered_parameter_number, p_channel_controller->registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_NULL:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_NULL :: voice = %d\r\n", voice);
		p_channel_controller->registered_parameter_value = 0;
		break;
	default:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN code = %d :: voice = %d, value = %d \r\n",
						p_channel_controller->registered_parameter_number, voice, p_channel_controller->registered_parameter_value);
		break;
	}
}

/**********************************************************************************/
enum
{
	LoudnessChangeVolume = 0,
	LoudnessChangeExpression = 1,
};

static void process_loudness_change(uint32_t const tick, int8_t const voice, int8_t const value,
									int loudness_change_type)
{
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	int8_t original_value;
	switch(loudness_change_type)
	{
	case LoudnessChangeVolume:
		original_value = p_channel_controller->volume;
		break;
	case LoudnessChangeExpression:
	default:
		original_value = p_channel_controller->expression;
		break;
	}

	do {
		if(value == original_value){
			break;
		}


		int16_t oscillator_index = get_event_occupied_oscillator_head_index();
		int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();
		for(int16_t i = 0; i < occupied_oscillator_number; i++){
			oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
			do {
				if(voice != p_oscillator->voice){
					break;
				}
				if(true == IS_FREEING(p_oscillator->state_bits)){
					break;
				}

				//CHIPTUNE_PRINTF(cMidiSetup, "oscillator = %d, envelope_state = %d, loudness %u -> %u\r\n",
				//				oscillator_index, p_oscillator->envelope_state,
				//				p_oscillator->loudness, (p_oscillator->loudness * value)/original_value);
				p_oscillator->loudness = (p_oscillator->loudness * value)/original_value;
				original_value += !original_value;
				p_oscillator->envelope_table_index = 0;
				p_oscillator->envelope_same_index_count = 0;
				switch(p_oscillator->envelope_state){
				case ENVELOPE_ATTACK:
					break;
				case ENVELOPE_DECAY:
					break;
				case ENVELOPE_SUSTAIN:
					p_oscillator->envelope_state = ENVELOPE_DECAY;
					break;
				}
			} while(0);
			oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
		}
	} while(0);
}

/**********************************************************************************/

static void process_cc_volume(uint32_t const tick, int8_t const voice, int8_t const value)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_VOLUME(%d) :: voice = %d, value = %d\r\n",
					tick, MIDI_CC_VOLUME, voice, value);
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	process_loudness_change(tick, voice, value, LoudnessChangeVolume);
	p_channel_controller->volume = value;
}

/**********************************************************************************/

static void process_cc_pan(uint32_t const tick, int8_t const voice, int8_t const value)
{
#define PAN_BAR_SCALE_NUMBER						(16)
#define PAN_BAR_DELTA_TICK							((INT8_MAX + 1)/PAN_BAR_SCALE_NUMBER)
	char panning_bar_string[PAN_BAR_SCALE_NUMBER + 1] = "";
	do {
		int location = (value + PAN_BAR_DELTA_TICK/2)/PAN_BAR_DELTA_TICK;
		for(int i = 0; i < location - 1; i++){
			snprintf(&panning_bar_string[strlen(&panning_bar_string[0])],
					sizeof(panning_bar_string) - strlen(&panning_bar_string[0]), "-");
		}
		snprintf(&panning_bar_string[strlen(&panning_bar_string[0])],
				sizeof(panning_bar_string) - strlen(&panning_bar_string[0]), "+");
		for(int i = location + 1; i < PAN_BAR_SCALE_NUMBER; i++){
			snprintf(&panning_bar_string[strlen(&panning_bar_string[0])],
					sizeof(panning_bar_string) - strlen(&panning_bar_string[0]), "-");
		}
	} while (0);
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_PAN_VOLUME(%d) :: voice = %2d, value = %3d  %s\r\n",
					tick, MIDI_CC_PAN, voice, value, &panning_bar_string);
	get_channel_controller_pointer_from_index(voice)->pan = value;
}

/**********************************************************************************/

static void process_cc_expression(uint32_t const tick, int8_t const voice, int8_t const value)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EXPRESSION(%d) :: voice = %d, value = %d\r\n",
					tick, MIDI_CC_EXPRESSION, voice, value);
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	process_loudness_change(tick, voice, value, LoudnessChangeExpression);
	p_channel_controller->expression = value;
}

/**********************************************************************************/

static void process_cc_damper_pedal(uint32_t const tick, int8_t const voice, uint8_t const value)
{
	bool is_damper_pedal_on = (value < MIDI_CC_CENTER_VALUE) ? false : true;
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_DAMPER_PEDAL(%d) :: voice = %d, %s\r\n",
					tick, MIDI_CC_DAMPER_PEDAL, voice, is_damper_pedal_on? "on" : "off");

	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	p_channel_controller->is_damper_pedal_on = is_damper_pedal_on;
	if(true == is_damper_pedal_on){
		return ;
	}

	int16_t oscillator_index = get_event_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();
	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t const * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
		do {
			if(voice != p_oscillator->voice){
				break;
			}
			if(true == IS_NOTE_ON(p_oscillator->state_bits)){
				break;
			}
			if(false == IS_NATIVE_OSCILLATOR(p_oscillator->state_bits)){
				break;
			}
			if(true == IS_FREEING(p_oscillator->state_bits)){
				break;
			}
			put_event(EVENT_FREE, oscillator_index, tick);

			int32_t expression_multiply_volume = p_channel_controller->expression * p_channel_controller->volume;
			expression_multiply_volume += !expression_multiply_volume;
			process_chorus_effect(tick, EVENT_FREE, voice, p_oscillator->note,
								  p_oscillator->loudness * INT8_MAX/ expression_multiply_volume, oscillator_index);
			process_reverb_effect(tick, EVENT_FREE, voice, p_oscillator->note,
								  p_oscillator->loudness * INT8_MAX/expression_multiply_volume, oscillator_index);
		} while(0);
		oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
	}
}

/**********************************************************************************/

static void process_cc_reverb_effect(uint32_t const tick, int8_t const voice, int8_t const value)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_REVERB_DEPTH(%d) :: voice = %d, value = %d\r\n",
					tick, MIDI_CC_REVERB_DEPTH, voice, value);
	get_channel_controller_pointer_from_index(voice)->reverb = value;
}

/**********************************************************************************/

static void process_cc_chorus_effect(uint32_t const tick, int8_t const voice, int8_t const value)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_CHORUS_EFFECT(%d) :: voice = %d, value = %d\r\n",
					tick, MIDI_CC_CHORUS_EFFECT, voice, value);
	get_channel_controller_pointer_from_index(voice)->chorus = value;
}

/**********************************************************************************/

static void process_cc_reset_all_controllers(uint32_t const tick, int8_t const voice, int8_t const value)
{
	(void)value;
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_RESET_ALL_CONTROLLERS(%d) :: voices = %d\r\n",
					tick, MIDI_CC_RESET_ALL_CONTROLLERS, voice);
	reset_channel_controller_midi_parameters_from_index(voice);
}

/**********************************************************************************/

int process_control_change_message(uint32_t const tick, int8_t const voice, int8_t const number, int8_t const value)
{
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	switch(number)
	{
	case MIDI_CC_MODULATION_WHEEL:
		process_modulation_wheel(tick, voice, value);
		break;
	case MIDI_CC_DATA_ENTRY_MSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_DATA_ENTRY_MSB(%d) :: voice = %d, value = %d\r\n",
						tick, number, voice, value);
		p_channel_controller->registered_parameter_value
				= ((value & 0xFF) << 8) | (p_channel_controller->registered_parameter_value & (0xFF << 0));
		process_cc_registered_parameter(tick, voice);
		break;
	case MIDI_CC_VOLUME:
		process_cc_volume(tick, voice, value);
		break;
	case MIDI_CC_PAN:
		process_cc_pan(tick, voice, value);
		break;
	case MIDI_CC_EXPRESSION:
		process_cc_expression(tick, voice, value);
		break;
	case MIDI_CC_DATA_ENTRY_LSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_DATA_ENTRY_LSB(%d) :: voice = %d, value = %d\r\n",
						tick, number, voice, value);
		p_channel_controller->registered_parameter_value
				= (p_channel_controller->registered_parameter_value & (0xFF << 8)) | ((value & 0xFF) << 0);
		process_cc_registered_parameter(tick, voice);
		break;
	case MIDI_CC_DAMPER_PEDAL:
		process_cc_damper_pedal(tick, voice, value);
		break;
	case MIDI_CC_REVERB_DEPTH:
		process_cc_reverb_effect(tick, voice, value);
		break;
	case MIDI_CC_EFFECT_2_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EFFECT_2_DEPTH(%d) :: voice = %d, value = %d %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_CHORUS_EFFECT:
		process_cc_chorus_effect(tick, voice, value);
		break;
	case MIDI_CC_EFFECT_4_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EFFECT_4_DEPTH(%d) :: voice = %d, value = %d %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_EFFECT_5_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EFFECT_5_DEPTH(%d) :: voice = %d, value = %d %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_NRPN_LSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_NRPN_LSB(%d) :: voice = %d, value = %d %s\r\n",
						tick, voice, number, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_NRPN_MSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_NRPN_MSB(%d) :: voice = %d, value = %d %s\r\n",
						tick, voice, number, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_LSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_RPN_LSB(%d) :: voice = %d, value = %d\r\n",
						tick, number, voice, value);
		p_channel_controller->registered_parameter_number
				= (p_channel_controller->registered_parameter_number & (0xFF << 8)) | ((value & 0xFF) << 0);
		break;
	case MIDI_CC_RPN_MSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_RPN_MSB(%d) :: voice = %d, value = %d\r\n",
						tick, number, voice, value);
		p_channel_controller->registered_parameter_number
				= ((value & 0xFF) << 8) | (p_channel_controller->registered_parameter_number & (0xFF << 0));
		break;
	case MIDI_CC_RESET_ALL_CONTROLLERS:
		process_cc_reset_all_controllers(tick, voice, value);
		break;
	default:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC code = %d :: voice = %d, value = %d %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	}

	return 0;
}
