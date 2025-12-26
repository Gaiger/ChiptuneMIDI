// NOLINTBEGIN(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif
#include <stdio.h>	// IWYU pragma: keep
#include <string.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"

#include "chiptune_channel_controller_internal.h"
#include "chiptune_oscillator_internal.h"
#include "chiptune_event_internal.h"
#include "chiptune_envelope_internal.h"
#include "chiptune_midi_effect_internal.h"

#include "chiptune_midi_control_change_internal.h"


static inline void process_modulation_wheel(uint32_t const tick, int8_t const voice, midi_value_t const value)
{
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_MODULATION_WHEEL(%d) :: voice = %d, value = %d\r\n",
					tick, MIDI_CC_MODULATION_WHEEL, voice, value);
	get_channel_controller_pointer_from_index(voice)->modulation_wheel
			= (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(value);
}

/**********************************************************************************/

#define SEVEN_BITS_VALID(VALUE)						((0x7F) & (VALUE))

static void process_cc_registered_parameter(uint32_t const tick, int8_t const voice)
{
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	switch(p_channel_controller->registered_parameter_number)
	{
	case MIDI_CC_RPN_PITCH_BEND_SENSITIVY:
		p_channel_controller->pitch_wheel_bend_range_in_semitones
				= (midi_value_t)SEVEN_BITS_VALID(p_channel_controller->registered_parameter_value >> 8);
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %d, MIDI_CC_RPN_PITCH_BEND_SENSITIVY(%d) :: voice = %d, pitch_wheel_bend_range_in_semitones = %d\r\n",
						tick, MIDI_CC_RPN_PITCH_BEND_SENSITIVY,
						voice, p_channel_controller->pitch_wheel_bend_range_in_semitones);
		if(0 != SEVEN_BITS_VALID(p_channel_controller->registered_parameter_value)){
			CHIPTUNE_PRINTF(cMidiControlChange, "tick = %d, MIDI_CC_RPN_PITCH_BEND_SENSITIVY(%d) :: voice = %d, cents = %d %s\r\n",
						tick, MIDI_CC_RPN_PITCH_BEND_SENSITIVY,
						voice, SEVEN_BITS_VALID(p_channel_controller->registered_parameter_number), "(NOT IMPLEMENTED YET)");
		}
		break;
	case MIDI_CC_RPN_CHANNEL_FINE_TUNING: {
			short fourteen_bits_value
					= (SEVEN_BITS_VALID(p_channel_controller->registered_parameter_value >> 8) << 7)
					+ SEVEN_BITS_VALID(p_channel_controller->registered_parameter_value & 0xFF);
			p_channel_controller->fine_tuning_value = fourteen_bits_value;
			CHIPTUNE_PRINTF(cMidiControlChange, "tick = %d, MIDI_CC_RPN_CHANNEL_FINE_TUNING(%d) :: voice = %d, fine tuning in semitones = %+3.2f\r\n",
							tick, MIDI_CC_RPN_CHANNEL_FINE_TUNING,
							voice, (p_channel_controller->fine_tuning_value - MIDI_FOURTEEN_BITS_CENTER_VALUE)/(float)MIDI_FOURTEEN_BITS_CENTER_VALUE);
			p_channel_controller->tuning_in_semitones
					= (float)(p_channel_controller->coarse_tuning_value - MIDI_SEVEN_BITS_CENTER_VALUE)
					+ (p_channel_controller->fine_tuning_value - MIDI_FOURTEEN_BITS_CENTER_VALUE)/(float)MIDI_FOURTEEN_BITS_CENTER_VALUE;
		} break;
	case MIDI_CC_RPN_CHANNEL_COARSE_TUNING:
		p_channel_controller->coarse_tuning_value
				= (midi_value_t)SEVEN_BITS_VALID(p_channel_controller->registered_parameter_value >> 8);
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %d, MIDI_CC_RPN_CHANNEL_COARSE_TUNING(%d) :: voice = %d, tuning in semitones = %+d\r\n",
						tick, MIDI_CC_RPN_CHANNEL_COARSE_TUNING, voice, p_channel_controller->coarse_tuning_value - MIDI_SEVEN_BITS_CENTER_VALUE);
		p_channel_controller->tuning_in_semitones
							= (float)(p_channel_controller->coarse_tuning_value - MIDI_SEVEN_BITS_CENTER_VALUE)
							+ (p_channel_controller->fine_tuning_value - MIDI_FOURTEEN_BITS_CENTER_VALUE)/(float)MIDI_FOURTEEN_BITS_CENTER_VALUE;
		break;
	case MIDI_CC_RPN_TURNING_PROGRAM_CHANGE:
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %d, MIDI_CC_RPN_TURNING_PROGRAM_CHANGE(%d) :: voice = %d, value = %d, value = %d (%s)\r\n",
						tick, MIDI_CC_RPN_TURNING_PROGRAM_CHANGE,
						voice, p_channel_controller->registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_TURNING_BANK_SELECT:
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %d, MIDI_CC_RPN_TURNING_BANK_SELECT(%d) :: voice = %d, value = %d %s\r\n",
						tick, MIDI_CC_RPN_TURNING_BANK_SELECT, voice, p_channel_controller->registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_MODULATION_DEPTH_RANGE:
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %d, MIDI_CC_RPN_MODULATION_DEPTH_RANGE(%d) :: voice = %d, value = %d %s\r\n",
						tick, MIDI_CC_RPN_MODULATION_DEPTH_RANGE, voice, p_channel_controller->registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_NULL:
		CHIPTUNE_PRINTF(cMidiControlChange, "---- MIDI_CC_RPN_NULL :: voice = %d\r\n", voice);
		p_channel_controller->registered_parameter_value = 0;
		break;
	default:
		CHIPTUNE_PRINTF(cMidiControlChange, "---- MIDI_CC_RPN code = %d :: voice = %d, value = %d \r\n",
						p_channel_controller->registered_parameter_number, voice, p_channel_controller->registered_parameter_value);
		break;
	}
}

/**********************************************************************************/

void process_loudness_change(uint32_t const tick, int8_t const voice, midi_value_t const value,
									int8_t loudness_change_type)
{
	normalized_midi_level_t const normalized_value
			= (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(value);
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	int16_t original_value;
	int16_t change_to_value;
	do{
		if(LoudnessChangeVolume == loudness_change_type){
			original_value = p_channel_controller->volume;
			change_to_value = normalized_value;
			break;
		}

		original_value = p_channel_controller->expression + EFFECTIVE_PRESSURE_LEVEL(p_channel_controller->pressure);
		if(LoundnessChangePressure == loudness_change_type){
			change_to_value = p_channel_controller->expression + EFFECTIVE_PRESSURE_LEVEL(normalized_value);
			break;
		}
		//LoudnessChangeExpression || LoundessBreathController
		change_to_value = EFFECTIVE_PRESSURE_LEVEL(p_channel_controller->pressure) + normalized_value;
	} while(0);

	do {
		if(normalized_value == original_value){
			break;
		}

		int16_t oscillator_index = get_occupied_oscillator_head_index();
		int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
		for(int16_t i = 0; i < occupied_oscillator_number; i++){
			oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
			do {
				if(voice != p_oscillator->voice){
					break;
				}
				if(true == IS_FREEING_OR_PREPARE_TO_FREE(p_oscillator->state_bits)){
					break;
				}
				if(EnvelopeStateFreeRelease == p_oscillator->envelope_state){
					break;
				}

				int32_t temp = (p_oscillator->loudness * change_to_value) / ZERO_AS_ONE(original_value);
				if(temp > INT16_MAX_PLUS_1){
					CHIPTUNE_PRINTF(cDeveloping, "WARNING :: loudness over IN16_MAX in %s\r\n", __func__);
					temp = INT16_MAX_PLUS_1;
				}
				p_oscillator->loudness = (int16_t)temp;

				uint8_t to_envelope_state = EnvelopeStateDecay;
				/*low to high, always attack, even it is LoundessBreathController.*/
				if(p_oscillator->amplitude < p_oscillator->loudness){
					to_envelope_state = EnvelopeStateAttack;
				}
				switch_melodic_envelope_state(p_oscillator, to_envelope_state);
			} while(0);
			oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
		}
	} while(0);
}

/**********************************************************************************/

static void process_breath_controller(uint32_t const tick, int8_t const voice, midi_value_t const value)
{
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_BREATH_CONTROLLER(%d) :: voice = %d, value = %d\r\n",
					tick, MIDI_CC_BREATH_CONTROLLER, voice, value);
	process_loudness_change(tick, voice, value, LoundessBreathController);
}

/**********************************************************************************/

static void process_cc_volume(uint32_t const tick, int8_t const voice, midi_value_t const value)
{
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_VOLUME(%d) :: voice = %d, value = %d\r\n",
					tick, MIDI_CC_VOLUME, voice, value);
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	process_loudness_change(tick, voice, value, LoudnessChangeVolume);
	p_channel_controller->volume = (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(value);
}

/**********************************************************************************/

static void process_cc_pan(uint32_t const tick, int8_t const voice, midi_value_t const value)
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
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_PAN_VOLUME(%d) :: voice = %2d, value = %3d  %s\r\n",
					tick, MIDI_CC_PAN, voice, value, &panning_bar_string);
	get_channel_controller_pointer_from_index(voice)->pan = value;
}

/**********************************************************************************/

static void process_cc_expression(uint32_t const tick, int8_t const voice, midi_value_t const value)
{
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_EXPRESSION(%d) :: voice = %d, value = %d\r\n",
					tick, MIDI_CC_EXPRESSION, voice, value);
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	process_loudness_change(tick, voice, value, LoudnessChangeExpression);
	p_channel_controller->expression = (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(value);
}

/**********************************************************************************/

static void process_cc_damper_pedal(uint32_t const tick, int8_t const voice, midi_value_t const value)
{
	bool is_damper_pedal_on = (value < MIDI_SEVEN_BITS_CENTER_VALUE) ? false : true;
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_DAMPER_PEDAL(%d) :: voice = %d, %s\r\n",
					tick, MIDI_CC_DAMPER_PEDAL, voice, is_damper_pedal_on? "on" : "off");

	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	p_channel_controller->is_damper_pedal_on = is_damper_pedal_on;
	if(true == is_damper_pedal_on){
		return ;
	}

	int16_t oscillator_index = get_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t const * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
		do {
			if(voice != p_oscillator->voice){
				break;
			}
			if(false == IS_PRIMARY_OSCILLATOR(p_oscillator)){
				break;
			}
			if(true == IS_NOTE_ON(p_oscillator->state_bits)){
				break;
			}
			if(true == IS_FREEING_OR_PREPARE_TO_FREE(p_oscillator->state_bits)){
				break;
			}

			put_event(EventTypeFree, oscillator_index, tick);
			int16_t expression_added_pressure = p_channel_controller->expression
					+ EFFECTIVE_PRESSURE_LEVEL(p_channel_controller->pressure);

			int32_t expression_added_pressure_multiply_volume = expression_added_pressure * p_channel_controller->volume;
			process_effects(tick, EventTypeFree, voice, p_oscillator->note,
							MULTIPLY_BY_128(p_oscillator->loudness) / ZERO_AS_ONE(expression_added_pressure_multiply_volume),
							oscillator_index);
		} while(0);
		oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
	}
}

/**********************************************************************************/

static void process_cc_reverb_effect(uint32_t const tick, int8_t const voice, midi_value_t const value)
{
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_REVERB_EFFECT(%d) :: voice = %d, depth = %d\r\n",
					tick, MIDI_CC_REVERB_EFFECT, voice, value);
	get_channel_controller_pointer_from_index(voice)->reverb
			= (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(value);
}

/**********************************************************************************/

static void process_cc_chorus_effect(uint32_t const tick, int8_t const voice, midi_value_t const value)
{
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_CHORUS_EFFECT(%d) :: voice = %d, depth = %d\r\n",
					tick, MIDI_CC_CHORUS_EFFECT, voice, value);
	get_channel_controller_pointer_from_index(voice)->chorus
			= (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(value);
}

/**********************************************************************************/

static void process_cc_detune_effect(uint32_t const tick, int8_t const voice, midi_value_t const value)
{
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_DETUNE_EFFECT(%d) :: voice = %d, depth = %d\r\n",
					tick, MIDI_CC_DETUNE_EFFECT, voice, value);
	get_channel_controller_pointer_from_index(voice)->detune
			= (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(value);
}

/**********************************************************************************/

static void process_cc_reset_all_controllers(uint32_t const tick, int8_t const voice, midi_value_t const value)
{
	(void)value;
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_RESET_ALL_CONTROLLERS(%d) :: voices = %d\r\n",
					tick, MIDI_CC_RESET_ALL_CONTROLLERS, voice);
	reset_channel_controller_midi_control_change_parameters(voice);
}

/**********************************************************************************/

int process_control_change_message(uint32_t const tick, int8_t const voice,
								   midi_value_t const number, midi_value_t const value)
{
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	switch(number)
	{
	case MIDI_CC_MODULATION_WHEEL:
		process_modulation_wheel(tick, voice, value);
		break;
	case MIDI_CC_BREATH_CONTROLLER:
		process_breath_controller(tick, voice, value);
		break;
	case MIDI_CC_DATA_ENTRY_MSB:
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_DATA_ENTRY_MSB(%d) :: voice = %d, value = %d\r\n",
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
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_DATA_ENTRY_LSB(%d) :: voice = %d, value = %d\r\n",
						tick, number, voice, value);
		p_channel_controller->registered_parameter_value
				= (p_channel_controller->registered_parameter_value & (0xFF << 8)) | ((value & 0xFF) << 0);
		process_cc_registered_parameter(tick, voice);
		break;
	case MIDI_CC_DAMPER_PEDAL:
		process_cc_damper_pedal(tick, voice, value);
		break;
	case MIDI_CC_REVERB_EFFECT:
		process_cc_reverb_effect(tick, voice, value);
		break;
	case MIDI_CC_EFFECT_2:
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_EFFECT_2(%d) :: voice = %d, depth = %d %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_CHORUS_EFFECT:
		process_cc_chorus_effect(tick, voice, value);
		break;
	case MIDI_CC_DETUNE_EFFECT:
		process_cc_detune_effect(tick, voice, value);
		break;
	case MIDI_CC_EFFECT_5:
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_EFFECT_5(%d) :: voice = %d, depth = %d %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_NRPN_LSB:
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_NRPN_LSB(%d) :: voice = %d, value = %d %s\r\n",
						tick, voice, number, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_NRPN_MSB:
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_NRPN_MSB(%d) :: voice = %d, value = %d %s\r\n",
						tick, voice, number, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_LSB:
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_RPN_LSB(%d) :: voice = %d, value = %d\r\n",
						tick, number, voice, value);
		p_channel_controller->registered_parameter_number
				= (p_channel_controller->registered_parameter_number & (0xFF << 8)) | ((value & 0xFF) << 0);
		break;
	case MIDI_CC_RPN_MSB:
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC_RPN_MSB(%d) :: voice = %d, value = %d\r\n",
						tick, number, voice, value);
		p_channel_controller->registered_parameter_number
				= ((value & 0xFF) << 8) | (p_channel_controller->registered_parameter_number & (0xFF << 0));
		break;
	case MIDI_CC_RESET_ALL_CONTROLLERS:
		process_cc_reset_all_controllers(tick, voice, value);
		break;
	default:
		CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_CC code = %d :: voice = %d, value = %d %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	}

	return 0;
}

// NOLINTEND(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
