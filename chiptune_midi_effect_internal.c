#include <string.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_oscillator_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_event_internal.h"

#include "chiptune_midi_effect_internal.h"


#define DIVIDE_BY_16(VALUE)							((VALUE) >> 4)
#define DIVIDE_BY_128(VALUE)						((VALUE) >> 7)

#define ASSOCIATE_OSCILLATOR_NUMBER					(4 - 1)

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

static inline void swap(int16_t *a, int16_t *b)
{
	int temp = *a;
	*a = *b;
	*b = temp;
}

/**********************************************************************************/

static void sort_loudnesses_to_descending(int16_t loudnesses[4])
{
	do {
		if(loudnesses[0] < loudnesses[1]) {
			swap(&loudnesses[0] , &loudnesses[1]);
		}
		if(loudnesses[2] < loudnesses[3]) {
			swap(&loudnesses[2] , &loudnesses[3]);
		}
		if(loudnesses[0] < loudnesses[2]) {
			swap(&loudnesses[0] , &loudnesses[2]);
		}
		if(loudnesses[1] < loudnesses[3]) {
			swap(&loudnesses[1] , &loudnesses[3]);
		}
		if(loudnesses[1] < loudnesses[2]) {
			swap(&loudnesses[1], &loudnesses[2]);
		}
	}while(0);
}

/**********************************************************************************/
#define ENHANDED_REVERB_LOUDNESS(LUDNESS, REVERB_VALUE) \
														DIVIDE_BY_128((LUDNESS) * ((INT8_MAX + 1) + (REVERB_VALUE + 1)))

int process_reverb_effect(uint32_t const tick, int8_t const event_type,
						  int8_t const voice, int8_t const note, int8_t const velocity,
						  int16_t const native_oscillator_index)
{
	(void)velocity;
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0 == voice ||
			MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1 == voice){
		return 1;
	}
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	if(0 == p_channel_controller->reverb){
		return 2;
	}
	//return 3;
	oscillator_t  * const p_native_oscillator = get_event_oscillator_pointer_from_index(native_oscillator_index);

	do {
		if(EVENT_ACTIVATE == event_type){
			int16_t const loudness = p_native_oscillator->loudness;
			int16_t averaged_loudness = DIVIDE_BY_16(ENHANDED_REVERB_LOUDNESS(loudness, p_channel_controller->reverb));
			// oscillator 1 : 3 * averaged_volume
			// oscillator 2 : 4 * averaged_volume
			// oscillator 3 : 6 * averaged_volume
			// oscillator 0 : volume - (3 + 4 + 6) * averaged_volume
			int16_t loudnesses[1 + ASSOCIATE_REVERB_OSCILLATOR_NUMBER]
					= { loudness - (3 + 4 + 6) * averaged_loudness,
						3 * averaged_loudness,
						4 * averaged_loudness,
						6 * averaged_loudness };
			sort_loudnesses_to_descending(loudnesses);
			p_native_oscillator->loudness = loudnesses[0];

			for(int16_t i = 0; i < ASSOCIATE_REVERB_OSCILLATOR_NUMBER; i++){
				int16_t oscillator_index;
				oscillator_t * const p_oscillator = acquire_event_freed_oscillator(&oscillator_index);
				if(NULL == p_oscillator){
					return -1;
				}
				memcpy(p_oscillator, p_native_oscillator, sizeof(oscillator_t));
				p_oscillator->loudness = loudnesses[i + 1];
				SET_REVERB_ASSOCIATE(p_oscillator->state_bits);
				p_native_oscillator->reverb_asscociate_oscillators[i] = oscillator_index;
			}
			break;
		}
	} while(0);

	uint32_t reverb_delta_tick = obtain_reverb_delta_tick(p_channel_controller->reverb);
	for(int16_t i = 0; i < ASSOCIATE_REVERB_OSCILLATOR_NUMBER; i++){
		put_event(event_type, p_native_oscillator->reverb_asscociate_oscillators[i],
				  tick + (i + 1) * reverb_delta_tick);
	}

	return 0;
}

/**********************************************************************************/
#define ENHANDED_CHORUS_LOUDNESS(LUDNESS, CHORUS_VALUE) \
														DIVIDE_BY_128((LUDNESS) * ((INT8_MAX + 1) + (CHORUS_VALUE + 1) * 3))

int process_chorus_effect(uint32_t const tick, int8_t const event_type,
						  int8_t const voice, int8_t const note, int8_t const velocity,
						  int16_t const native_oscillator_index)
{
	(void)velocity;
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0 == voice ||
			MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1 == voice){
		return 1;
	}
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	if(0 == p_channel_controller->chorus){
		return 2;
	}

	uint32_t chorus_delta_tick = obtain_chorus_delta_tick(p_channel_controller->chorus);
	oscillator_t  * p_native_oscillator = get_event_oscillator_pointer_from_index(native_oscillator_index);
	int native_oscillator_indexes[1 + ASSOCIATE_REVERB_OSCILLATOR_NUMBER]
			= {native_oscillator_index,
			   p_native_oscillator->reverb_asscociate_oscillators[0],
			   p_native_oscillator->reverb_asscociate_oscillators[1],
			   p_native_oscillator->reverb_asscociate_oscillators[2]};

	do {
		if(EVENT_ACTIVATE == event_type){
			for(int k = 0; k < 1 + ASSOCIATE_REVERB_OSCILLATOR_NUMBER; k++){
				if(UNUSED_OSCILLATOR == native_oscillator_indexes[k]){
					break;
				}
				p_native_oscillator = get_event_oscillator_pointer_from_index(native_oscillator_indexes[k]);

				int16_t const loudness = p_native_oscillator->loudness;
				int16_t averaged_loudness = DIVIDE_BY_16(ENHANDED_CHORUS_LOUDNESS(loudness, p_channel_controller->chorus));
				// oscillator 1 : 3 * averaged_volume
				// oscillator 2 : 4 * averaged_volume
				// oscillator 3 : 6 * averaged_volume
				// oscillator 0 : volume - (3 + 5  + 6) * averaged_volume
				int16_t loudnesses[4]
						= { loudness - (3 + 5 + 6) * averaged_loudness,
							3 * averaged_loudness,
							5 * averaged_loudness,
							6 * averaged_loudness};
				p_native_oscillator->loudness = loudnesses[0];

				float pitch_wheel_bend_in_semitone = 0.0f;
				for(int16_t i = 0; i < ASSOCIATE_CHORUS_OSCILLATOR_NUMBER; i++){
					int16_t oscillator_index;
					oscillator_t * const p_oscillator = acquire_event_freed_oscillator(&oscillator_index);
					if(NULL == p_oscillator){
						return -1;
					}
					int8_t const vibrato_modulation_in_semitone = p_channel_controller->vibrato_modulation_in_semitone;
					memcpy(p_oscillator, p_native_oscillator, sizeof(oscillator_t));
					p_oscillator->loudness = loudnesses[i + 1];
					p_oscillator->pitch_chorus_bend_in_semitone
							= obtain_oscillator_pitch_chorus_bend_in_semitone(p_channel_controller->chorus,
																			  p_channel_controller->max_pitch_chorus_bend_in_semitones);
					p_oscillator->delta_phase
							= calculate_oscillator_delta_phase(p_oscillator->note, p_channel_controller->tuning_in_semitones,
															   p_channel_controller->pitch_wheel_bend_range_in_semitones,
															   p_channel_controller->pitch_wheel,
															   p_oscillator->pitch_chorus_bend_in_semitone,
															   &pitch_wheel_bend_in_semitone);
					p_oscillator->max_delta_vibrato_phase
							= calculate_oscillator_delta_phase(p_oscillator->note + vibrato_modulation_in_semitone,
															   p_channel_controller->tuning_in_semitones,
															   p_channel_controller->pitch_wheel_bend_range_in_semitones,
															   p_channel_controller->pitch_wheel,
															   p_oscillator->pitch_chorus_bend_in_semitone,
															   &pitch_wheel_bend_in_semitone) - p_oscillator->delta_phase;
					SET_CHORUS_ASSOCIATE(p_oscillator->state_bits);
					p_native_oscillator->chorus_asscociate_oscillators[i] = oscillator_index;
					put_event(event_type, p_native_oscillator->chorus_asscociate_oscillators[i],
							  tick + (i + 1) * chorus_delta_tick);
				}
			}
			break;
		}

		for(int k = 0; k < 1 + ASSOCIATE_REVERB_OSCILLATOR_NUMBER; k++){
			if(UNUSED_OSCILLATOR == native_oscillator_indexes[k]){
				break;
			}

			p_native_oscillator = get_event_oscillator_pointer_from_index(native_oscillator_indexes[k]);
			for(int16_t i = 0; i < ASSOCIATE_CHORUS_OSCILLATOR_NUMBER; i++){
				put_event(event_type, p_native_oscillator->chorus_asscociate_oscillators[i],
						  tick + (i + 1) * chorus_delta_tick);
			}
		}
	} while(0);

	return 0;
}

/**********************************************************************************/

void update_effect_tick(void)
{
	float const tempo = get_tempo();
	uint32_t const resolution = get_resolution();
	s_min_reverb_delta_tick = (float)(EACH_REVERB_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND * tempo * resolution / 60.0);
	s_min_chorus_delta_tick = (float)(EACH_CHORUS_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND * tempo * resolution / 60.0);
}
