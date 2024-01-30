#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"

#include "chiptune_channel_controller_internal.h"
#include "chiptune_oscillator_internal.h"
#include "chiptune_event_internal.h"

#include "chiptune_midi_control_change_internal.h"

//https://anotherproducer.com/online-tools-for-musicians/midi-cc-list/

static inline void process_modulation_wheel(uint32_t const tick, uint8_t const voice, uint8_t const value)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_MODULATION_WHEEL :: voice = %u, value = %u\r\n",
					tick, voice, value);
	get_channel_controller_pointer_from_index(voice)->modulation_wheel = value;
}

/**********************************************************************************/

static void process_cc_registered_parameter(uint32_t const tick, uint8_t const voice)
{
	(void)tick;
//http://www.philrees.co.uk/nrpnq.htm
#define MIDI_CC_RPN_PITCH_BEND_SENSITIVY			(0)
#define MIDI_CC_RPN_CHANNEL_FINE_TUNING				(1)
#define MIDI_CC_RPN_CHANNEL_COARSE_TUNING			(2)
#define MIDI_CC_RPN_TURNING_PROGRAM_CHANGE			(3)
#define MIDI_CC_RPN_TURNING_BANK_SELECT				(4)
#define MIDI_CC_RPN_MODULATION_DEPTH_RANGE			(5)
#ifndef MIDI_CC_RPN_NULL
	#define MIDI_CC_RPN_NULL						((127 << 8) + 127)
#endif
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	switch(p_channel_controller->registered_parameter_number)
	{
	case MIDI_CC_RPN_PITCH_BEND_SENSITIVY:
		p_channel_controller->pitch_wheel_bend_range_in_semitones = 0x7F & (p_channel_controller->registered_parameter_value >> 8);
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_PITCH_BEND_SENSITIVY :: voice = %u, semitones = %u\r\n",
						voice, p_channel_controller->pitch_wheel_bend_range_in_semitones);
		if(0 != (p_channel_controller->registered_parameter_value & 0x7F)){
			CHIPTUNE_PRINTF(cMidiSetup, "----  MIDI_CC_RPN_PITCH_BEND_SENSITIVY :: voice = %u, cents = %u (%s)\r\n",
						voice, p_channel_controller->registered_parameter_number & 0x7F, "(NOT IMPLEMENTED YET)");
		}
		break;
	case MIDI_CC_RPN_CHANNEL_FINE_TUNING:
		CHIPTUNE_PRINTF(cMidiSetup, "----  MIDI_CC_RPN_CHANNEL_FINE_TUNING(%u) :: voice = %u, value = %u %s\r\n",
						voice, p_channel_controller->registered_parameter_number, p_channel_controller->registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_CHANNEL_COARSE_TUNING:
		p_channel_controller->tuning_in_semitones = 0x7F & (p_channel_controller->registered_parameter_value >> 8) - MIDI_CC_CENTER_VALUE;
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_CHANNEL_COARSE_TUNING(%u) :: voice = %u, tuning in semitones = %+d\r\n",
						voice, p_channel_controller->registered_parameter_number, p_channel_controller->tuning_in_semitones);
		break;
	case MIDI_CC_RPN_TURNING_PROGRAM_CHANGE:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_TURNING_PROGRAM_CHANGE(%u) :: voice = %u, value = %u, value = %u %s\r\n",
						voice, p_channel_controller->registered_parameter_number, p_channel_controller->registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_TURNING_BANK_SELECT:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_TURNING_BANK_SELECT(%u) :: voice = %u, value = %u %s\r\n",
						voice, p_channel_controller->registered_parameter_number, p_channel_controller->registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_MODULATION_DEPTH_RANGE:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_MODULATION_DEPTH_RANGE(%u) :: voice = %u %s\r\n",
						voice, p_channel_controller->registered_parameter_number, p_channel_controller->registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_NULL:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_NULL :: voice = %u\r\n", voice);
		p_channel_controller->registered_parameter_value = 0;
		break;
	default:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN code = %d :: voice = %u, value = %u \r\n",
						p_channel_controller->registered_parameter_number, voice, p_channel_controller->registered_parameter_value);
		break;
	}
}

/**********************************************************************************/

static inline void process_cc_volume(uint32_t const tick, uint8_t const voice, uint8_t const value)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_VOLUME :: voice = %u, value = %u\r\n", tick, voice, value);
	get_channel_controller_pointer_from_index(voice)->max_volume = value;
}

/**********************************************************************************/

static inline void process_cc_expression(uint32_t const tick, uint8_t const voice, uint8_t const value)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EXPRESSION :: voice = %u, value = %u\r\n", tick, voice, value);
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	p_channel_controller->playing_volume = (value * p_channel_controller->max_volume)/INT8_MAX;
}

/**********************************************************************************/

int process_chorus_effect(uint32_t const tick, bool const is_note_on,
						   uint8_t const voice, uint8_t const note, uint8_t const velocity,
						   int16_t const original_oscillator_index);

static void process_cc_damper_pedal(uint32_t const tick, uint8_t const voice, uint8_t const value)
{
	bool is_damper_pedal_on = (value < MIDI_CC_CENTER_VALUE) ? false : true;
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_DAMPER_PEDAL :: voice = %u, %s\r\n",
					tick, voice, is_damper_pedal_on? "on" : "off");

	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	p_channel_controller->is_damper_pedal_on = is_damper_pedal_on;
	if(true == is_damper_pedal_on){
		return ;
	}

	int16_t oscillator_index = get_head_occupied_oscillator_index();
	int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
		do {
			if(voice != p_oscillator->voice){
				break;
			}
			if(true == IS_NOTE_ON(p_oscillator->state_bits)){
				break;
			}
			if(UNUSED_OSCILLATOR != p_oscillator->native_oscillator){
				break;
			}
			put_event(RELEASE_EVENT, oscillator_index, tick);
			process_chorus_effect(tick, false, voice, p_oscillator->note,
						  p_oscillator->volume/p_channel_controller->playing_volume,
								  oscillator_index);
		} while(0);
		oscillator_index = get_next_occupied_oscillator_index(oscillator_index);
	}
}

/**********************************************************************************/

static void process_cc_chorus_effect(uint32_t const tick, uint8_t const voice, uint8_t const value)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_CHORUS_EFFECT :: voice = %u, value = %u\r\n", tick, voice, value);
	get_channel_controller_pointer_from_index(voice)->chorus = value;
}

/**********************************************************************************/

static void process_cc_reset_all_controllers(uint32_t const tick, uint8_t const voice, uint8_t const value)
{
	(void)voice;
	(void)value;
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_RESET_ALL_CONTROLLERS :: voices = %u \r\n", tick, voice);
	reset_channel_controller_from_index(voice);

	int16_t oscillator_index = get_head_occupied_oscillator_index();
	int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
		if( voice == p_oscillator->voice){
			put_event(RELEASE_EVENT, oscillator_index, tick);
		}
		oscillator_index = get_next_occupied_oscillator_index(oscillator_index);
	}
}

/**********************************************************************************/

int process_control_change_message(uint32_t const tick, uint8_t const voice, uint8_t const number, uint8_t const value)
{
#define MIDI_CC_MODULATION_WHEEL					(1)

#define MIDI_CC_DATA_ENTRY_MSB						(6)
#define MIDI_CC_VOLUME								(7)
#define MIDI_CC_PAN									(10)
#define MIDI_CC_EXPRESSION							(11)

#define MIDI_CC_DATA_ENTRY_LSB						(32 + MIDI_CC_DATA_ENTRY_MSB)

#define MIDI_CC_DAMPER_PEDAL						(64)

#define MIDI_CC_EFFECT_1_DEPTH						(91)
#define MIDI_CC_EFFECT_2_DEPTH						(92)
#define MIDI_CC_EFFECT_3_DEPTH						(93)
#define MIDI_CC_CHORUS_EFFECT						(MIDI_CC_EFFECT_3_DEPTH)
#define MIDI_CC_EFFECT_4_DEPTH						(94)
#define MIDI_CC_EFFECT_5_DEPTH						(95)

#define MIDI_CC_NRPN_LSB							(98)
#define MIDI_CC_NRPN_MSB							(99)

//https://zh.wikipedia.org/zh-tw/General_MIDI
#define MIDI_CC_RPN_LSB								(100)
#define MIDI_CC_RPN_MSB								(101)

#define MIDI_CC_RESET_ALL_CONTROLLERS				(121)
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	switch(number)
	{
	case MIDI_CC_MODULATION_WHEEL:
		process_modulation_wheel(tick, voice, value);
		break;
	case MIDI_CC_DATA_ENTRY_MSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_DATA_ENTRY_MSB :: voice = %u, value = %u\r\n",
						tick, voice, value);
		p_channel_controller->registered_parameter_value
				= ((value & 0xFF) << 8) | (p_channel_controller->registered_parameter_value & (0xFF << 0));
		process_cc_registered_parameter(tick, voice);
		break;
	case MIDI_CC_VOLUME:
		process_cc_volume(tick, voice, value);
		break;
	case MIDI_CC_PAN:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_PAN(%u) :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		p_channel_controller->pan = value;
		break;
	case MIDI_CC_EXPRESSION:
		process_cc_expression(tick, voice, value);
		break;
	case MIDI_CC_DATA_ENTRY_LSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_DATA_ENTRY_LSB :: voice = %u, value = %u\r\n",
						tick, voice, value);
		p_channel_controller->registered_parameter_value
				= (p_channel_controller->registered_parameter_value & (0xFF << 8)) | ((value & 0xFF) << 0);
		process_cc_registered_parameter(tick, voice);
		break;
	case MIDI_CC_DAMPER_PEDAL:
		process_cc_damper_pedal(tick, voice, value);
		break;
	case MIDI_CC_EFFECT_1_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EFFECT_1_DEPTH(%u) :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_EFFECT_2_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EFFECT_2_DEPTH(%u) :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_CHORUS_EFFECT:
		process_cc_chorus_effect(tick, voice, value);
		break;
	case MIDI_CC_EFFECT_4_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EFFECT_4_DEPTH(%u) :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_EFFECT_5_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EFFECT_5_DEPTH(%u) :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_NRPN_LSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_NRPN_LSB(%u) :: voice = %u, value = %u %s\r\n",
						tick, voice, number, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_NRPN_MSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_NRPN_MSB(%u) :: voice = %u, value = %u %s\r\n",
						tick, voice, number, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_LSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_RPN_LSB :: voice = %u, value = %u\r\n",
						tick, voice, value);
		p_channel_controller->registered_parameter_number
				= (p_channel_controller->registered_parameter_number & (0xFF << 8)) | ((value & 0xFF) << 0);
		break;
	case MIDI_CC_RPN_MSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_RPN_MSB :: voice = %u, value = %u\r\n",
						tick, voice, value);
		p_channel_controller->registered_parameter_number
				= ((value & 0xFF) << 8) | (p_channel_controller->registered_parameter_number & (0xFF << 0));
		break;
	case MIDI_CC_RESET_ALL_CONTROLLERS:
		process_cc_reset_all_controllers(tick, voice, value);
		break;
	default:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC code = %u :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	}

	return 0;
}
