#include "chiptune_midi_define_internal.h"
#include "chiptune_oscillator_internal.h"
#include "chiptune_channel_controller_internal.h"

#include "chiptune_envelope_internal.h"

void update_melodic_envelope(oscillator_t * const p_oscillator)
{
	do {
		if(MIDI_PERCUSSION_CHANNEL == p_oscillator->voice){
			break;
		}

		channel_controller_t const *p_channel_controller
				= get_channel_controller_pointer_from_index(p_oscillator->voice);

		if(EnvelopeStateSustain == p_oscillator->envelope_state){
			if(true == IS_NOTE_ON(p_oscillator->state_bits)){
				break;
			}
			if(UINT16_MAX == p_channel_controller->envelope_damper_on_but_note_off_sustain_same_index_number){
				break;
			}
		}

		if(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH == p_oscillator->envelope_table_index){
			break;
		}

		uint16_t envelope_same_index_number = 0;
		switch(p_oscillator->envelope_state)
		{
		case EnvelopeStateAttack:
			envelope_same_index_number = p_channel_controller->envelope_attack_same_index_number;
			break;
		case EnvelopeStateDecay:
			envelope_same_index_number = p_channel_controller->envelope_decay_same_index_number;
			break;
		case EnvelopeStateSustain:
			envelope_same_index_number = p_channel_controller->envelope_damper_on_but_note_off_sustain_same_index_number;
			break;
		case EnvelopeStateRelease:
			do {
				if(true == IS_RESTING(p_oscillator->state_bits)){
					envelope_same_index_number = p_channel_controller->envelope_attack_same_index_number;
					break;
				}
				envelope_same_index_number = p_channel_controller->envelope_release_same_index_number;
			} while(0);
			break;
		};

		do
		{
			if(0 < p_oscillator->envelope_same_index_count){
				break;
			}
			if(0 == envelope_same_index_number){
				break;
			}

			int8_t const * p_envelope_table = NULL;
			int16_t delta_amplitude = 0;
			int16_t shift_amplitude = 0;
			switch(p_oscillator->envelope_state)
			{
			case EnvelopeStateAttack:
				p_envelope_table = p_channel_controller->p_envelope_attack_table;
				delta_amplitude = p_oscillator->loudness - p_oscillator->attack_decay_reference_amplitude;
				shift_amplitude = p_oscillator->attack_decay_reference_amplitude;
				break;
			case EnvelopeStateDecay: {
				p_envelope_table = p_channel_controller->p_envelope_decay_table;
				int16_t sustain_ampitude
						= SUSTAIN_AMPLITUDE(p_oscillator->loudness,
											p_channel_controller->envelope_sustain_level);
				do {
					if(0 != p_oscillator->attack_decay_reference_amplitude){
						delta_amplitude = p_oscillator->attack_decay_reference_amplitude - sustain_ampitude;
						break;
					}
					delta_amplitude = p_oscillator->loudness - sustain_ampitude;
				} while(0);
				shift_amplitude = sustain_ampitude;
			}	break;
			case EnvelopeStateSustain:
				p_envelope_table = p_channel_controller->p_envelope_damper_on_but_note_off_sustain_table;
				delta_amplitude = p_oscillator->loudness;
				break;
			default:
			case EnvelopeStateRelease:
				do{
					if(true == IS_RESTING(p_oscillator->state_bits)){
						p_envelope_table = p_channel_controller->p_envelope_attack_table;
						break;
					}
					p_envelope_table = p_channel_controller->p_envelope_release_table;
				}while(0);
				delta_amplitude = p_oscillator->release_reference_amplitude;
				break;
			}

			p_oscillator->amplitude = MELODIC_ENVELOPE_DELTA_AMPLITUDE(delta_amplitude,
														 p_envelope_table[p_oscillator->envelope_table_index]);
			p_oscillator->amplitude	+= shift_amplitude;
			if(EnvelopeStateRelease != p_oscillator->envelope_state){
				p_oscillator->release_reference_amplitude = p_oscillator->amplitude;
			}
		} while(0);

		p_oscillator->envelope_same_index_count += 1;
		if(envelope_same_index_number > p_oscillator->envelope_same_index_count){
			break;
		}

		p_oscillator->envelope_same_index_count = 0;
		p_oscillator->envelope_table_index += 1;
		if(0 == envelope_same_index_number){
			p_oscillator->envelope_table_index = CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH;
		}

		do {
			if(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH > p_oscillator->envelope_table_index){
				break;
			}

			p_oscillator->envelope_table_index = 0;
			switch(p_oscillator->envelope_state)
			{
			case EnvelopeStateAttack:
				p_oscillator->attack_decay_reference_amplitude = 0;
				if(0 < p_channel_controller->envelope_decay_same_index_number){
					p_oscillator->envelope_state = EnvelopeStateDecay;
					break;
				}
			case EnvelopeStateDecay:
				p_oscillator->envelope_state = EnvelopeStateSustain;
				/*
				 * SIN: the last decay point is updated to sustain amplitude in advance.
				 *
				 * If we do NOT update here, sustain must keep doing
				 *     p_oscillator->amplitude = p_oscillator->sustain_amplitude
				 * on every sample; otherwise we need to add a field
				 *     is_sustain_amplitude_copied_to_amplitude
				 * which makes oscillator_t dirty.
				 *
				 * In contrast, updating one point in advance is acceptable for now.
				 * (Known issue with a workaround; behavior is intentional.)
				 */
				p_oscillator->amplitude = SUSTAIN_AMPLITUDE(p_oscillator->loudness,
															p_channel_controller->envelope_sustain_level);
				if(true == IS_NOTE_ON(p_oscillator->state_bits)){
					break;
				}
				if(0 < p_channel_controller->envelope_damper_on_but_note_off_sustain_same_index_number){
					break;
				}
				p_oscillator->envelope_state = EnvelopeStateRelease;
				break;
			case EnvelopeStateSustain:
			case EnvelopeStateRelease:
				SET_DEACTIVATED(p_oscillator->state_bits);
				break;
			}
			p_oscillator->release_reference_amplitude = p_oscillator->amplitude;
		} while(0);
	} while(0);
}

/**********************************************************************************/

void update_percussion_envelope(oscillator_t * const p_oscillator)
{
	do {
		if(false == (MIDI_PERCUSSION_CHANNEL == p_oscillator->voice)){
			break;
		}

		percussion_t const * const p_percussion = get_percussion_pointer_from_index(p_oscillator->note);
		int8_t waveform_segment_index = p_oscillator->percussion_waveform_segment_index;
		if (p_percussion->waveform_segment_duration_sample_number[waveform_segment_index]
				== p_oscillator->percussion_waveform_segment_duration_sample_count){
			p_oscillator->percussion_waveform_segment_duration_sample_count = 0;
			p_oscillator->percussion_waveform_segment_index += 1;
			if(MAX_PERCUSSION_WAVEFORM_SEGMENT_NUMBER == p_oscillator->percussion_waveform_segment_index){
				p_oscillator->percussion_waveform_segment_index = MAX_PERCUSSION_WAVEFORM_SEGMENT_NUMBER - 1;
			}
		}
		p_oscillator->percussion_waveform_segment_duration_sample_count += 1;

		do
		{
			if(0 < p_oscillator->percussion_envelope_same_index_count){
				break;
			}

			if(0 == p_percussion->envelope_same_index_number){
				break;
			}
			p_oscillator->amplitude = PERCUSSION_ENVELOPE(p_oscillator->loudness,
														  p_percussion->p_amplitude_envelope_table[p_oscillator->percussion_envelope_table_index]);
			p_oscillator->percussion_phase_sweep_delta
					= PERCUSSION_PHASE_SWEEP_DELTA(p_percussion->max_phase_sweep_delta,
												   p_percussion->p_phase_sweep_table[p_oscillator->percussion_envelope_table_index]);
		} while(0);

		p_oscillator->percussion_envelope_same_index_count += 1;
		p_oscillator->current_phase += p_oscillator->percussion_phase_sweep_delta;

		if(p_percussion->envelope_same_index_number > p_oscillator->percussion_envelope_same_index_count){
			break;
		}

		p_oscillator->percussion_envelope_table_index += 1;
		p_oscillator->percussion_envelope_same_index_count = 0;
		if(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH == p_oscillator->percussion_envelope_table_index){
			SET_DEACTIVATED(p_oscillator->state_bits);
			//p_oscillator->percussion_envelope_table_index = CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1;
			break;
		}
	} while(0);
}
