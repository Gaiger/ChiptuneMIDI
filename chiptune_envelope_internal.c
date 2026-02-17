#include "chiptune_printf_internal.h"
#include "chiptune_oscillator_internal.h"
#include "chiptune_channel_controller_internal.h"

#include "chiptune_envelope_internal.h"

#define SUSTAIN_AMPLITUDE(LOUNDNESS, SUSTAIN_LEVEL)	\
	CHANNEL_CONTROLLER_SCALE_BY_LEVEL((LOUNDNESS), (SUSTAIN_LEVEL))

#define MELODIC_ENVELOPE_DELTA_AMPLITUDE(AMPLITUDE, TABLE_VALUE)	\
	CHANNEL_CONTROLLER_SCALE_BY_LEVEL((AMPLITUDE), (TABLE_VALUE))

#define PERCUSSION_PHASE_SWEEP_DELTA(MAX_PHASE_SWEEP_DELTA, SWEEP_TABLE_VALUE) \
	CHANNEL_CONTROLLER_SCALE_BY_LEVEL((MAX_PHASE_SWEEP_DELTA), (SWEEP_TABLE_VALUE))

#define PERCUSSION_ENVELOPE(LOUDNESS, ENVELOPE_TABLE_VALUE)	\
	CHANNEL_CONTROLLER_SCALE_BY_LEVEL((LOUDNESS), (ENVELOPE_TABLE_VALUE))

uint16_t calculate_sustain_amplitude(uint16_t const loudness,
								   normalized_midi_level_t const envelope_sustain_level)
{
	return SUSTAIN_AMPLITUDE(loudness, envelope_sustain_level);
}

/**********************************************************************************/

int switch_melodic_envelope_state(oscillator_t * const p_oscillator, uint8_t const evelope_state)
{
	int ret = 0;
	do
	{
		if(EnvelopeStateMax <= evelope_state){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: undefined state number = %u in %s\r\n",
							evelope_state, __func__);
			ret = -1;
			break;
		}

		if(true == IS_PERCUSSION_OSCILLATOR(p_oscillator)){
			ret = 1;
			break;
		}

		if(false == IS_ACTIVATED(p_oscillator->state_bits)){
			ret = 3;
			break;
		}

		if(EnvelopeStateFreeRelease == p_oscillator->envelope_state){
			if(EnvelopeStateFreeRelease != evelope_state){
				CHIPTUNE_PRINTF(cDeveloping, "WARNING :: voice = %d, note = %d,"
											 "ignore from EnvelopeStateFreeRelease to %u in %s\r\n",
								p_oscillator->voice, p_oscillator->note, evelope_state, __func__);
			}
			break;
		}
		p_oscillator->envelope_state = evelope_state;
		p_oscillator->envelope_table_index = 0;
		p_oscillator->envelope_same_index_count = 0;
		p_oscillator->envelope_reference_amplitude = p_oscillator->amplitude;
	} while(0);

	return ret;
}

/**********************************************************************************/

void update_melodic_envelope(oscillator_t * const p_oscillator)
{
	do {
		if(true == IS_PERCUSSION_OSCILLATOR(p_oscillator)){
			break;
		}

		channel_controller_t const * const p_channel_controller
				= get_channel_controller_pointer_from_index(p_oscillator->voice);
		if(EnvelopeStateNoteOnSustain == p_oscillator->envelope_state){
			break;
		}
		if(EnvelopeStateDamperSustain == p_oscillator->envelope_state){
			if(UINT16_MAX == p_channel_controller->envelope_damper_sustain_same_index_number){
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
		case EnvelopeStateDamperSustain:
			envelope_same_index_number = p_channel_controller->envelope_damper_sustain_same_index_number;
			break;
		case EnvelopeStateDamperEntryRelease:
		case EnvelopeStateFreeRelease:
			/*even true == IS_RESTING() treat as the normal release.*/
			envelope_same_index_number = p_channel_controller->envelope_release_same_index_number;
			break;
		default:
			CHIPTUNE_PRINTF(cDeveloping, "ERROR::envelope_state = %d in %s\r\n",
							p_oscillator->envelope_state, __func__);
		};

		do
		{
			if(0 < p_oscillator->envelope_same_index_count){
				break;
			}
			if(0 == envelope_same_index_number){
				break;
			}

			do
			{
				if(0 < p_oscillator->envelope_table_index){
					break;
				}
				uint16_t delta_amplitude = 0;
				uint16_t shift_amplitude = 0;
				int8_t const * p_envelope_table = NULL;

				switch(p_oscillator->envelope_state)
				{
				case EnvelopeStateAttack:
					p_envelope_table = p_channel_controller->p_envelope_attack_table;
					delta_amplitude = p_oscillator->loudness - p_oscillator->envelope_reference_amplitude;
					shift_amplitude = p_oscillator->envelope_reference_amplitude;
					break;
				case EnvelopeStateDecay:
				case EnvelopeStateDamperEntryRelease: {
					p_envelope_table = p_channel_controller->p_envelope_decay_table;
					normalized_midi_level_t sustain_level = p_channel_controller->envelope_note_on_sustain_level;
					if(EnvelopeStateDamperEntryRelease == p_oscillator->envelope_state){
						p_envelope_table = p_channel_controller->p_envelope_release_table;
						sustain_level = p_channel_controller->envelop_damper_sustain_level;
					}

					uint16_t sustain_ampitude = SUSTAIN_AMPLITUDE(p_oscillator->loudness, sustain_level);
					do {
						if(p_oscillator->envelope_reference_amplitude >= sustain_ampitude)
						{
							delta_amplitude = p_oscillator->envelope_reference_amplitude - sustain_ampitude;
							break;
						}
						//Theoretically, envelope_reference_amplitude should be always greater than sustain_ampitude
						// here the fallback uses loudness to get delta_amplitude
						CHIPTUNE_PRINTF(cDeveloping, "WARNING :: envelope_reference_amplitude = %u"
													 ", less than sustain_ampitude = %u as envelope_state = %u in %s\r\n",
										p_oscillator->envelope_reference_amplitude, sustain_ampitude,
										p_oscillator->envelope_state, __func__);
						delta_amplitude = p_oscillator->loudness - sustain_ampitude;
					} while(0);
					shift_amplitude = sustain_ampitude;
				  } break;
				case EnvelopeStateDamperSustain:
					p_envelope_table = p_channel_controller->p_envelope_damper_sustain_table;
					delta_amplitude = p_oscillator->envelope_reference_amplitude;
					break;
				default:
				case EnvelopeStateFreeRelease:
					/*even true == IS_RESTING() treat as the normal release.*/
					p_envelope_table = p_channel_controller->p_envelope_release_table;
					delta_amplitude = p_oscillator->envelope_reference_amplitude;
					break;
				}
				p_oscillator->p_envelope_table = p_envelope_table;
				p_oscillator->delta_amplitude = delta_amplitude;
				p_oscillator->shift_amplitude = shift_amplitude;
			} while(0);
			p_oscillator->amplitude = MELODIC_ENVELOPE_DELTA_AMPLITUDE(p_oscillator->delta_amplitude,
														 p_oscillator->p_envelope_table[p_oscillator->envelope_table_index]);
			p_oscillator->amplitude	+= p_oscillator->shift_amplitude;
			if(INT16_MAX_PLUS_1 < p_oscillator->amplitude){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: amplitude = %u, "
								"greater than INT16_MAX_PLUS_1 in %s\r\n", p_oscillator->amplitude, __func__);
			}
			p_oscillator->envelope_reference_amplitude = p_oscillator->amplitude;
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
				if(0 < p_channel_controller->envelope_decay_same_index_number){
					p_oscillator->envelope_state = EnvelopeStateDecay;
					break;
				}
			case EnvelopeStateDecay:
				p_oscillator->envelope_state = EnvelopeStateNoteOnSustain;
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
															p_channel_controller->envelope_note_on_sustain_level);
				if(true == IS_NOTE_ON(p_oscillator->state_bits)){
					break;
				}
				p_oscillator->envelope_state = EnvelopeStateDamperSustain;
				if(0 < p_channel_controller->envelope_damper_sustain_same_index_number){
					break;
				}
				p_oscillator->envelope_state = EnvelopeStateFreeRelease;
				break;
			case EnvelopeStateDamperEntryRelease:
				p_oscillator->envelope_state = EnvelopeStateDamperSustain;
				p_oscillator->amplitude = SUSTAIN_AMPLITUDE(p_oscillator->loudness,
															p_channel_controller->envelop_damper_sustain_level);
				break;
			case EnvelopeStateDamperSustain:
			case EnvelopeStateFreeRelease:
				SET_DEACTIVATED(p_oscillator->state_bits);
				break;
			}
			p_oscillator->envelope_reference_amplitude = p_oscillator->amplitude;
		} while(0);
	} while(0);
}

/**********************************************************************************/

void update_percussion_envelope(oscillator_t * const p_oscillator)
{
	do {
		if(false == (true == IS_PERCUSSION_OSCILLATOR(p_oscillator))){
			break;
		}

		percussion_t const * const p_percussion = get_percussion_pointer_from_key(p_oscillator->note);
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
