//NOLINTBEGIN(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif
#include <stdio.h>	// IWYU pragma: keep

#include "chiptune_midi_define.h"
#include "chiptune_printf_internal.h"

#include "chiptune_oscillator_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_event_internal.h"
#include "chiptune_envelope_internal.h"
#include "chiptune_midi_effect_internal.h"
#include "chiptune_midi_control_change_internal.h"

#include "chiptune_midi_note_internal.h"


int finalize_melodic_oscillator_setup(uint32_t const tick, int8_t const voice,
									  midi_value_t const note, normalized_midi_level_t const velocity,
									  oscillator_t * const p_oscillator)
{
	(void)tick; (void)voice; (void)note; (void)velocity;
	if(true == IS_PERCUSSION_OSCILLATOR(p_oscillator)){
		return 1;
	}

	update_oscillator_phase_increment(p_oscillator);
	p_oscillator->amplitude = 0;
	p_oscillator->pitch_detune_in_semitones = 0.0;
	p_oscillator->vibrato_table_index = 0;
	p_oscillator->vibrato_same_index_count = 0;

	p_oscillator->envelope_state = EnvelopeStateAttack;
	p_oscillator->envelope_same_index_count = 0;
	p_oscillator->envelope_table_index = 0;
	p_oscillator->envelope_reference_amplitude = 0;
	p_oscillator->midi_effect_association = MidiEffectNone;
	return 0;
}

/**********************************************************************************/

int finalize_percussion_oscillator_setup(uint32_t const tick, int8_t const voice,
										 midi_value_t const note, normalized_midi_level_t const velocity,
										 oscillator_t * const p_oscillator)
{
	(void)tick; (void)voice; (void)note; (void)velocity;
	if(false == (true == IS_PERCUSSION_OSCILLATOR(p_oscillator))){
		return 1;
	}

	p_oscillator->percussion_waveform_segment_index = 0;
	p_oscillator->percussion_waveform_segment_duration_sample_count = 0;
	p_oscillator->percussion_envelope_same_index_count = 0;
	p_oscillator->percussion_envelope_table_index = 0;

	percussion_t const * const p_percussion = get_percussion_pointer_from_index(note);
	p_oscillator->base_phase_increment = p_percussion->base_phase_increment;
	//p_oscillator->amplitude = PERCUSSION_ENVELOPE(p_oscillator->loudness,
	//				p_percussion->p_amplitude_envelope_table[p_oscillator->percussion_envelope_table_index]);
	return 0;
}

/**********************************************************************************/

static void rest_occupied_oscillator_with_same_voice_note(uint32_t const tick, int8_t const voice,
														  midi_value_t const note,
														  normalized_midi_level_t const velocity)
{
	int16_t oscillator_index = get_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
		do {
			if(note != p_oscillator->note){
				break;
			}
			if(voice != p_oscillator->voice){
				break;
			}
			if(false == IS_PRIMARY_OSCILLATOR(p_oscillator)){
				break;
			}
			if(true == IS_FREEING_OR_PREPARE_TO_FREE(p_oscillator->state_bits)){
				break;
			}
			if(true == IS_RESTING_OR_PREPARE_TO_REST(p_oscillator->state_bits)){
				break;
			}
			put_event(EventTypeRest, oscillator_index, tick);
			process_effects(tick, EventTypeRest, voice, note, velocity, oscillator_index);
		} while(0);
		oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
	}
}

/**********************************************************************************/

static int process_note_on_message(uint32_t const tick, int8_t const voice,
								   midi_value_t const note, normalized_midi_level_t const velocity)
{
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);

	rest_occupied_oscillator_with_same_voice_note(tick, voice, note, velocity);

	int16_t oscillator_index;
	oscillator_t * const p_oscillator = acquire_oscillator(&oscillator_index);
	if(NULL == p_oscillator){
		return -1;
	}
	RESET_STATE_BITES(p_oscillator->state_bits);
	SET_NOTE_ON(p_oscillator->state_bits);
	p_oscillator->voice = voice;
	p_oscillator->note = note;
	p_oscillator->velocity = velocity;
	p_oscillator->loudness = compute_loudness(velocity, p_channel_controller->volume,
											  p_channel_controller->pressure, p_channel_controller->expression,
											  p_channel_controller->breath);
	p_oscillator->current_phase = 0;
	do {
		if(MIDI_PERCUSSION_CHANNEL == voice){
			finalize_percussion_oscillator_setup(tick, voice, note, velocity, p_oscillator);
			break;
		}
		finalize_melodic_oscillator_setup(tick, voice, note, velocity, p_oscillator);
	} while(0);

	put_event(EventTypeActivate, oscillator_index, tick);
	process_effects(tick, EventTypeActivate, voice, note, velocity, oscillator_index);
	return 0;
}

/**********************************************************************************/

static int process_note_off_message(uint32_t const tick, int8_t const voice,
								   midi_value_t const note, normalized_midi_level_t const normalized_velocity)
{
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);

	bool is_found = false;
	int16_t oscillator_index = get_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
		do {
			if(note != p_oscillator->note){
				break;
			}
			if(voice != p_oscillator->voice){
				break;
			}
			if(false == IS_PRIMARY_OSCILLATOR(p_oscillator)){
				break;
			}
			if(false == IS_NOTE_ON(p_oscillator->state_bits)){
				break;
			}
			if(true == IS_FREEING_OR_PREPARE_TO_FREE(p_oscillator->state_bits)){
				break;
			}
			put_event(EventTypeFree, oscillator_index, tick);
			process_effects(tick, EventTypeFree, voice, note, normalized_velocity, oscillator_index);
			is_found = true;
		} while(0);
		if(true == is_found){
			break;
		}
		oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
	}

	if(false == is_found){
		CHIPTUNE_PRINTF(cDeveloping, "WARNING::dangling off note:: tick = %u, voice = %d, note = %u\r\n",
						tick, voice, note);
		return -2;
	}
#if 1
	do {
		if(MIDI_PERCUSSION_CHANNEL == voice){
			break;
		}
		if(false == p_channel_controller->is_damper_pedal_on){
			break;
		}
		int16_t const original_oscillator_index = oscillator_index;
		int16_t reduced_loundness_oscillator_index;
		oscillator_t * const p_oscillator = replicate_oscillator(original_oscillator_index,
																 &reduced_loundness_oscillator_index);
		RESET_STATE_BITES(p_oscillator->state_bits);

		SET_NOTE_OFF(p_oscillator->state_bits);
		p_oscillator->loudness
				= LOUNDNESS_AS_DAMPING_PEDAL_ON_BUT_NOTE_OFF(
					p_oscillator->loudness,
					p_channel_controller->envelop_damper_sustain_level);

		finalize_melodic_oscillator_setup(tick, voice, note, normalized_velocity, p_oscillator);
		put_event(EventTypeActivate, reduced_loundness_oscillator_index, tick);
		process_effects(tick, EventTypeActivate, voice, note, normalized_velocity,
						reduced_loundness_oscillator_index);
	} while(0);
#endif
	return 0;
}

/**********************************************************************************/

int process_note_message(uint32_t const tick, bool const is_note_on,
						 int8_t const voice, midi_value_t const note, midi_value_t const velocity)
{
	if(MIDI_PERCUSSION_CHANNEL == voice){
		if(NULL == get_percussion_pointer_from_index(note)){
			CHIPTUNE_PRINTF(cDeveloping, "WARNING:: tick = %u, PERCUSSION_INSTRUMENT = %d (%s)"
										 " does not be defined in the MIDI standard, ignored\r\n",
							tick, note, is_note_on ? "on" : "off");
			return 1;
		}
	}
	normalized_midi_level_t const normalized_velocity
			= (normalized_midi_level_t)NORMALIZE_MIDI_LEVEL(velocity);

	if(true == is_note_on){
		do {
			if(MIDI_PERCUSSION_CHANNEL == voice){
				percussion_t const * const p_percussion = get_percussion_pointer_from_index(note);
				char not_implemented_string[24] = {0};
				if(false == p_percussion->is_implemented){
					snprintf(&not_implemented_string[0], sizeof(not_implemented_string), "%s", "(NOT IMPLEMENTED)");
				}
				CHIPTUNE_PRINTF(cMidiNote, "tick = %u, %s :: voice = %d, %s%s, velocity = %d\r\n",
							tick,  "MIDI_MESSAGE_NOTE_ON",
							voice, get_percussion_name_string(note), not_implemented_string, velocity);
				break;
			}
			char pitch_wheel_bend_string[32] = "";
#ifdef _PRINT_MIDI_PITCH_WHEEL
			if(0.0f != p_channel_controller->pitch_wheel_bend_in_semitones){
				snprintf(&pitch_wheel_bend_string[0], sizeof(pitch_wheel_bend_string),
					", pitch wheel bend in semitone = %+3.2f", p_channel_controller->pitch_wheel_bend_in_semitones);
			}
#endif
			CHIPTUNE_PRINTF(cMidiNote, "tick = %u, %s :: voice = %d, note = %d, velocity = %d%s\r\n",
							tick, "MIDI_MESSAGE_NOTE_ON",
							voice, note, velocity, &pitch_wheel_bend_string[0]);
		} while(0);
		return process_note_on_message(tick, voice, note, normalized_velocity);
	}

	do {
		if(MIDI_PERCUSSION_CHANNEL == voice){
			CHIPTUNE_PRINTF(cMidiNote, "tick = %u, %s :: voice = %d, %s, velocity = %d\r\n",
							tick,  "MIDI_MESSAGE_NOTE_OFF",
							voice, get_percussion_name_string(note), velocity);
			break;
		}
		CHIPTUNE_PRINTF(cMidiNote, "tick = %u, %s :: voice = %d, note = %d, velocity = %d\r\n",
						tick,  "MIDI_MESSAGE_NOTE_OFF",
						voice, note, velocity);
	} while(0);
	return process_note_off_message(tick, voice, note, normalized_velocity);
}
