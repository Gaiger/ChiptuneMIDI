// NOLINTBEGIN(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)

#include <string.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_oscillator_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_event_internal.h"

#include "chiptune_midi_effect_internal.h"


#define DIVIDE_BY_16(VALUE)							((VALUE) >> 4)
#define DIVIDE_BY_32(VALUE)							((VALUE) >> 5)
#define DIVIDE_BY_128(VALUE)						((VALUE) >> 7)

#define ASSOCIATE_OSCILLATOR_NUMBER					(4 - 1)

#define TOP_LEVEL									(0)
#define REVERB_LEVEL								(0)
#define CHORUS_LEVEL								(1)

#define REVERB_ASSOCIATE_START_INDEX				(0)
#define CHORUS_ASSOCIATE_START_INDEX				((CHORUS_LEVEL) * ASSOCIATE_OSCILLATOR_NUMBER)

/**********************************************************************************/
#define ASSOCIATE_REVERB_OSCILLATOR_NUMBER			(ASSOCIATE_OSCILLATOR_NUMBER)
#define MAX_REVERB_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND	\
															(0.150)
#define EACH_REVERB_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND	\
															(MAX_REVERB_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND / \
															(float)( ASSOCIATE_REVERB_OSCILLATOR_NUMBER *(INT8_MAX + 1)))

#define ASSOCIATE_CHORUS_OSCILLATOR_NUMBER			(ASSOCIATE_OSCILLATOR_NUMBER)
#define MAX_CHORUS_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND	\
															(0.033)
#define EACH_CHORUS_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND	\
															(MAX_CHORUS_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND / \
															(float)( ASSOCIATE_CHORUS_OSCILLATOR_NUMBER * (INT8_MAX + 1)) \
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

int find_associate_oscillator_indexes(int16_t const native_index,
									  uint8_t const find_level, uint8_t current_level,
									  int oscillator_indexes[], int const max_oscillator_number,
									  int * const p_oscillator_number)
{
	if(find_level < current_level){
		return 0;
	}

	oscillator_t  * const p_native_oscillator = get_event_oscillator_pointer_from_index(native_index);
	for(int i = 0; i < ASSOCIATE_OSCILLATOR_NUMBER; i++){
		int16_t associate_oscillator_index
				= p_native_oscillator->associate_oscillators[find_level * ASSOCIATE_OSCILLATOR_NUMBER + i];
		if(UNOCCUPIED_OSCILLATOR == associate_oscillator_index){
			continue;
		}
		if(max_oscillator_number == *p_oscillator_number){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: oscillator_indexes is full in %s \r\n", __func__);
			return -1;
		}

		oscillator_indexes[*p_oscillator_number] = associate_oscillator_index;
		*p_oscillator_number += 1;

		if(find_level == current_level ){
			continue;
		}
		associate_oscillator_index = p_native_oscillator->associate_oscillators[current_level * ASSOCIATE_OSCILLATOR_NUMBER + i];
		if(UNOCCUPIED_OSCILLATOR == associate_oscillator_index){
			continue;
		}
		find_associate_oscillator_indexes(associate_oscillator_index, find_level, current_level + 1,
										  &oscillator_indexes[0], max_oscillator_number, p_oscillator_number);
	}

	return 0;
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

	oscillator_t  * const p_native_oscillator = get_event_oscillator_pointer_from_index(native_oscillator_index);

	do {
		if(EVENT_ACTIVATE == event_type){
			int32_t const enhanced_loudness = ENHANCE_REVERB_LOUDNESS(p_native_oscillator->loudness, p_channel_controller->reverb);
			int16_t const enhanced_loudness_over_32 = DIVIDE_BY_32(enhanced_loudness);
			int16_t loudnesses[1 + ASSOCIATE_REVERB_OSCILLATOR_NUMBER]
					= { 0,
						enhanced_loudness_over_32 * 9,
						enhanced_loudness_over_32 * 6,
						enhanced_loudness_over_32 * 7};
			loudnesses[0] = enhanced_loudness - (loudnesses[1] + loudnesses[2] + loudnesses[3]);
			p_native_oscillator->loudness = loudnesses[0];

			int16_t assocatiate_oscillator_indexes[ASSOCIATE_REVERB_OSCILLATOR_NUMBER];
			for(int16_t i = 0; i < ASSOCIATE_REVERB_OSCILLATOR_NUMBER; i++){
				int16_t oscillator_index;
				oscillator_t * const p_oscillator = acquire_event_freed_oscillator(&oscillator_index);
				if(NULL == p_oscillator){
					return -1;
				}
				memcpy(p_oscillator, p_native_oscillator, sizeof(oscillator_t));
				p_oscillator->loudness = loudnesses[i + 1];
				SET_REVERB_ASSOCIATE(p_oscillator->state_bits);
				assocatiate_oscillator_indexes[i] = oscillator_index;
			}

			for(int16_t i = 0; i < ASSOCIATE_REVERB_OSCILLATOR_NUMBER; i++){
				p_native_oscillator->associate_oscillators[REVERB_ASSOCIATE_START_INDEX + i]
						= assocatiate_oscillator_indexes[i];
			}
			break;
		}
	} while(0);

	uint32_t reverb_delta_tick = obtain_reverb_delta_tick(p_channel_controller->reverb);
	for(int16_t i = 0; i < ASSOCIATE_REVERB_OSCILLATOR_NUMBER; i++){
		put_event(event_type, p_native_oscillator->associate_oscillators[REVERB_ASSOCIATE_START_INDEX + i],
				  tick + (i + 1) * reverb_delta_tick);
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

	uint32_t chorus_delta_tick = obtain_chorus_delta_tick(p_channel_controller->chorus);

	do {
		if(EVENT_ACTIVATE == event_type){
			int native_oscillator_indexes[1 + ASSOCIATE_REVERB_OSCILLATOR_NUMBER];
			int native_oscillator_number = 0;
			native_oscillator_indexes[0] = native_oscillator_index;
			native_oscillator_number += 1;
			find_associate_oscillator_indexes(native_oscillator_index, REVERB_LEVEL, TOP_LEVEL,
											  &native_oscillator_indexes[0], 1 + ASSOCIATE_REVERB_OSCILLATOR_NUMBER,
					&native_oscillator_number);

			for(int k = 0; k < native_oscillator_number; k++){
				oscillator_t *p_native_oscillator = get_event_oscillator_pointer_from_index(native_oscillator_indexes[k]);

				int16_t const loudness = p_native_oscillator->loudness;
				int16_t loudness_over_16 = DIVIDE_BY_16(loudness);
				int16_t loudnesses[4]
						= { 0,
							4 * loudness_over_16,
							3 * loudness_over_16,
							5 * loudness_over_16};
				loudnesses[0] = loudness - (4 + 3 + 5) * loudness_over_16;
				p_native_oscillator->loudness = loudnesses[0];
				int16_t assocatiate_oscillator_indexes[ASSOCIATE_REVERB_OSCILLATOR_NUMBER];
				for(int16_t i = 0; i < ASSOCIATE_CHORUS_OSCILLATOR_NUMBER; i++){
					int16_t oscillator_index;
					oscillator_t * const p_oscillator = acquire_event_freed_oscillator(&oscillator_index);
					if(NULL == p_oscillator){
						return -1;
					}
					memcpy(p_oscillator, p_native_oscillator, sizeof(oscillator_t));
					p_oscillator->loudness = loudnesses[i + 1];
					p_oscillator->pitch_chorus_bend_in_semitones
							= obtain_oscillator_pitch_chorus_bend_in_semitones(p_channel_controller->chorus,
																			  p_channel_controller->max_pitch_chorus_bend_in_semitones);
					p_oscillator->delta_phase
							= calculate_oscillator_delta_phase(voice, p_oscillator->note,
															   p_oscillator->pitch_chorus_bend_in_semitones);

					int8_t const vibrato_modulation_in_semitones = p_channel_controller->vibrato_modulation_in_semitones;
					p_oscillator->max_delta_vibrato_phase
							= calculate_oscillator_delta_phase(voice, p_oscillator->note + vibrato_modulation_in_semitones,
															   p_oscillator->pitch_chorus_bend_in_semitones) - p_oscillator->delta_phase;
					SET_CHORUS_ASSOCIATE(p_oscillator->state_bits);
					assocatiate_oscillator_indexes[i] = oscillator_index;
				}

				for(int16_t i = 0; i < ASSOCIATE_CHORUS_OSCILLATOR_NUMBER; i++){
					p_native_oscillator->associate_oscillators[CHORUS_ASSOCIATE_START_INDEX + i]
						= assocatiate_oscillator_indexes[i];
					put_event(event_type, p_native_oscillator->associate_oscillators[CHORUS_ASSOCIATE_START_INDEX + i],
							  tick + (i + 1) * chorus_delta_tick);
				}
			}

			break;
		}

		int oscillator_indexes[(1 + ASSOCIATE_REVERB_OSCILLATOR_NUMBER) * ASSOCIATE_REVERB_OSCILLATOR_NUMBER];
		int oscillator_number = 0;
		find_associate_oscillator_indexes(native_oscillator_index, CHORUS_LEVEL, TOP_LEVEL,
										  &oscillator_indexes[0],
				(1 + ASSOCIATE_REVERB_OSCILLATOR_NUMBER) * ASSOCIATE_REVERB_OSCILLATOR_NUMBER,
				&oscillator_number);
		for(int16_t i = 0; i < oscillator_number; i++){
			put_event(event_type, oscillator_indexes[i], tick + (i + 1) * chorus_delta_tick);
		}
	} while(0);

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

// NOLINTEND(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)