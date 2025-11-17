#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h" // IWYU pragma: export
#include "chiptune_oscillator_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_event_internal.h"

#include "chiptune_midi_effect_internal.h"

//#define ASSOCIATE_OSCILLATOR_NUMBER					(SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER)

/**********************************************************************************/
#define REVERB_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER		(SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER)
#define MAX_REVERB_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND	\
													(0.150)
#define EACH_REVERB_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND	\
													(MAX_REVERB_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND / \
													(float)( REVERB_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER *(INT8_MAX + 1)))

#define CHORUS_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER		(SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER)
#define MAX_CHORUS_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND	\
													(0.033)
#define EACH_CHORUS_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND	\
													(MAX_CHORUS_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND / \
													(float)( CHORUS_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER * (INT8_MAX + 1)) \
													)

static float s_min_reverb_delta_tick =
		(float)(EACH_REVERB_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND * MIDI_DEFAULT_TEMPO * MIDI_DEFAULT_RESOLUTION / (60.0));
static float s_min_chorus_delta_tick =
		(float)(EACH_CHORUS_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND * MIDI_DEFAULT_TEMPO * MIDI_DEFAULT_RESOLUTION / (60.0));

/**********************************************************************************/

static inline uint32_t obtain_chorus_delta_tick(int8_t chorus)
{
	uint32_t chorus_delta_tick = (uint32_t)((chorus + 1) * s_min_chorus_delta_tick + 0.5);
	chorus_delta_tick |= !chorus_delta_tick;
	return chorus_delta_tick;
}

/**********************************************************************************/

static inline uint32_t obtain_reverb_delta_tick(int8_t reverb)
{
	uint32_t reverb_delta_tick = (uint32_t)((reverb + 1) * s_min_reverb_delta_tick + 0.5);
	reverb_delta_tick |= !reverb_delta_tick;
	return reverb_delta_tick;
}

/**********************************************************************************/

//#define ENHANCE_REVERB_LOUDNESS(LOUDNESS, REVERB_VALUE) \
//								DIVIDE_BY_128((LOUDNESS) * ((INT8_MAX + 1) + ((REVERB_VALUE) + 1)/2))

#define ENHANCE_REVERB_LOUDNESS(LOUDNESS, REVERB_VALUE) \
													(LOUDNESS)

static int process_reverb_effect(uint32_t const tick, int8_t const event_type,
						  int8_t const voice, int8_t const note, int8_t const velocity,
						  int16_t const native_oscillator_index)
{
	(void)velocity;
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
		return 1;
	}
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	if(0 == p_channel_controller->reverb){
		return 2;
	}

	do {
		if(EVENT_ACTIVATE == event_type){
			int cooperative_oscillator_number = 0;
			cooperative_oscillator_number += 1;
			cooperative_oscillator_number += count_all_subordinate_oscillators(WITHOUT_EFFECT(MidiEffectReverb),
																					 native_oscillator_index);
			STACK_ARRAY(int16_t, cooperative_oscillator_indexes, cooperative_oscillator_number);
			cooperative_oscillator_indexes[0] = native_oscillator_index;
			get_all_subordinate_oscillator_indexes(WITHOUT_EFFECT(MidiEffectReverb),
												   native_oscillator_index,
												   &cooperative_oscillator_indexes[1]);

			for(int k = 0; k < cooperative_oscillator_number; k++){
				oscillator_t  * const p_cooperative_oscillator
						= get_oscillator_pointer_from_index(cooperative_oscillator_indexes[k]);

				int32_t const enhanced_loudness = ENHANCE_REVERB_LOUDNESS(p_cooperative_oscillator->loudness,
																		  p_channel_controller->reverb);
				int16_t const enhanced_loudness_over_32 = DIVIDE_BY_32(enhanced_loudness);
				int16_t loudnesses[1 + REVERB_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER]
						= { 0,
							enhanced_loudness_over_32 * 9,
							enhanced_loudness_over_32 * 6,
							enhanced_loudness_over_32 * 7};
				loudnesses[0] = enhanced_loudness - (loudnesses[1] + loudnesses[2] + loudnesses[3]);
				p_cooperative_oscillator->loudness = loudnesses[0];

				int16_t associate_oscillator_indexes[REVERB_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER];
				for(int16_t i = 0; i < REVERB_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER; i++){
					int16_t oscillator_index;
					oscillator_t * const p_oscillator
							= replicate_oscillator(cooperative_oscillator_indexes[k], &oscillator_index);
					if(NULL == p_oscillator){
						return -1;
					}
					associate_oscillator_indexes[i] = oscillator_index;
					p_oscillator->loudness = loudnesses[1 + i];
					SET_REVERB_ASSOCIATE(p_oscillator->state_bits);
				}
				store_associate_oscillator_indexes(MidiEffectReverb, cooperative_oscillator_indexes[k],
											   &associate_oscillator_indexes[0]);
			}
			break;
		}
	} while(0);

	uint32_t const reverb_delta_tick = obtain_reverb_delta_tick(p_channel_controller->reverb);
	{
		int effect_subordinate_oscillator_number = 0;
		effect_subordinate_oscillator_number = count_all_subordinate_oscillators(MidiEffectReverb,
																				 native_oscillator_index);
		STACK_ARRAY(int16_t, effect_subordinate_oscillator_indexes, effect_subordinate_oscillator_number);
		get_all_subordinate_oscillator_indexes(MidiEffectReverb, native_oscillator_index,
											   &effect_subordinate_oscillator_indexes[0]);
		for(int16_t i = 0; i < effect_subordinate_oscillator_number; i++){
			put_event(event_type, effect_subordinate_oscillator_indexes[i],
					  tick + (i + 1) * reverb_delta_tick);
		}
	}
	return 0;
}

/**********************************************************************************/

static int process_chorus_effect(uint32_t const tick, int8_t const event_type,
						  int8_t const voice, int8_t const note, int8_t const velocity,
						  int16_t const native_oscillator_index)
{
	(void)velocity;
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
		return 1;
	}
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	if(0 == p_channel_controller->chorus){
		return 2;
	}

	do {
		if(EVENT_ACTIVATE == event_type){
			int cooperative_oscillator_number = 0;
			cooperative_oscillator_number += 1;
			cooperative_oscillator_number += count_all_subordinate_oscillators(WITHOUT_EFFECT(MidiEffectChorus),
																					 native_oscillator_index);
			STACK_ARRAY(int16_t, cooperative_oscillator_indexes, cooperative_oscillator_number);
			cooperative_oscillator_indexes[0] = native_oscillator_index;
			get_all_subordinate_oscillator_indexes(WITHOUT_EFFECT(MidiEffectChorus),
												   native_oscillator_index,
												   &cooperative_oscillator_indexes[1]);

			for(int k = 0; k < cooperative_oscillator_number; k++){
				oscillator_t *p_cooperative_oscillator
						= get_oscillator_pointer_from_index(cooperative_oscillator_indexes[k]);
				int16_t const loudness = p_cooperative_oscillator->loudness;
				int16_t loudness_over_16 = DIVIDE_BY_16(loudness);
				int16_t loudnesses[1 + CHORUS_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER]
						= { 0,
							4 * loudness_over_16,
							3 * loudness_over_16,
							5 * loudness_over_16};
				loudnesses[0] = loudness - (4 + 3 + 5) * loudness_over_16;
				p_cooperative_oscillator->loudness = loudnesses[0];

				int16_t associate_oscillator_indexes[CHORUS_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER];
				for(int16_t i = 0; i < CHORUS_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER; i++){
					int16_t oscillator_index;
					oscillator_t * const p_oscillator
							= replicate_oscillator(cooperative_oscillator_indexes[k], &oscillator_index);
					if(NULL == p_oscillator){
						return -1;
					}
					p_oscillator->loudness = loudnesses[i + 1];
					p_oscillator->pitch_chorus_bend_in_semitones
							= obtain_oscillator_pitch_chorus_bend_in_semitones(
								p_channel_controller->chorus, p_channel_controller->max_pitch_chorus_bend_in_semitones);

					p_oscillator->base_phase_increment = calculate_oscillator_base_phase_increment(voice, p_oscillator->note,
																				 p_oscillator->pitch_chorus_bend_in_semitones);
					associate_oscillator_indexes[i] = oscillator_index;
					SET_CHORUS_ASSOCIATE(p_oscillator->state_bits);
				}
				store_associate_oscillator_indexes(MidiEffectChorus, cooperative_oscillator_indexes[k],
											   &associate_oscillator_indexes[0]);
			}
			break;
		}
	} while(0);

	uint32_t const chorus_delta_tick = obtain_chorus_delta_tick(p_channel_controller->chorus);
	{
		int effect_subordinate_oscillator_number = 0;
		effect_subordinate_oscillator_number = count_all_subordinate_oscillators(MidiEffectChorus,
																				 native_oscillator_index);
		STACK_ARRAY(int16_t, effect_subordinate_oscillator_indexes, effect_subordinate_oscillator_number);
		get_all_subordinate_oscillator_indexes(MidiEffectChorus, native_oscillator_index,
											   &effect_subordinate_oscillator_indexes[0]);
		int jj = 0;
		for(int16_t i = 0; i < effect_subordinate_oscillator_number; i++){
			put_event(event_type, effect_subordinate_oscillator_indexes[i], tick + (jj + 1) * chorus_delta_tick);
			jj += 1;
			jj /= CHORUS_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER;
		}
	}

	return 0;
}

/**********************************************************************************/

int process_effects(uint32_t const tick, int8_t const event_type,
					int8_t const voice, int8_t const note, int8_t const velocity,
					int16_t const native_oscillator_index)
{
	process_reverb_effect(tick, event_type, voice, note, velocity, native_oscillator_index);
	process_chorus_effect(tick, event_type, voice, note, velocity, native_oscillator_index);
	return 0;
}

/**********************************************************************************/

void update_effect_tick(void)
{
	float const playing_tempo = get_playing_tempo();
	uint32_t const resolution = get_resolution();
	s_min_reverb_delta_tick = (float)(EACH_REVERB_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND * playing_tempo * resolution / 60.0);
	s_min_chorus_delta_tick = (float)(EACH_CHORUS_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND * playing_tempo * resolution / 60.0);
}

