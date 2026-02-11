#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h" // IWYU pragma: export
#include "chiptune_oscillator_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_event_internal.h"

#include "chiptune_midi_effect_internal.h"

//#define ASSOCIATE_OSCILLATOR_NUMBER					(SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER)

/**********************************************************************************/

#define	DEFAULT_MAX_DETUNE_DETUNE_IN_SEMITONE	(0.6f)
static float s_max_detune_detune_in_semitones = DEFAULT_MAX_DETUNE_DETUNE_IN_SEMITONE;

static normalized_midi_level_t get_effective_detune_depth(midi_value_t const note,
														  normalized_midi_level_t const detune_depth)
{
	if(0 == detune_depth){
		return 0;
	}

	int32_t scale_detune_note_multiply_128 = 0;
	do
	{
#define MIDEI_NOTE_C3									(48)//130.81
		if(note <= MIDEI_NOTE_C3){
			break;
		}
#define MIDEI_NOTE_C4									(60)//261.63
		if(note < MIDEI_NOTE_C3){
			scale_detune_note_multiply_128
					= ((note - MIDEI_NOTE_C3)*INT8_MAX_PLUS_1)/(MIDEI_NOTE_C4 - MIDEI_NOTE_C3);
			break;
		}
		scale_detune_note_multiply_128 = INT8_MAX_PLUS_1;
	}while(0);

	return DIVIDE_BY_128(scale_detune_note_multiply_128 * detune_depth);
}

/**********************************************************************************/

static float const calculate_detune_detune_in_semitones(normalized_midi_level_t const effective_detune_depth,
														float const max_detune_detune_in_semitones)
{
	if(0 == effective_detune_depth){
		return 0.0;
	}

	float detune_in_semitones = (float)effective_detune_depth/(float)(INT8_MAX_PLUS_1);
	detune_in_semitones *= max_detune_detune_in_semitones;
	return detune_in_semitones;
}

/**********************************************************************************/

#define DETUNE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER		(SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER)
#define ENHANCE_DETUNE_LOUDNESS(LOUDNESS, REVERB_VALUE)	\
	(LOUDNESS)

static int process_detune_effect(uint32_t const tick, int8_t const event_type,
								 int8_t const voice, midi_value_t const note,
								 normalized_midi_level_t const velocity,
								 int16_t const primary_oscillator_index)
{
	(void)note;
	(void)velocity;
	if(MIDI_PERCUSSION_CHANNEL == voice){
		return 1;
	}
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	normalized_midi_level_t effective_detune
			= get_effective_detune_depth(note, p_channel_controller->detune);
	if(0 == effective_detune){
		return 2;
	}

	do {
		if(EventTypeActivate == event_type){
			int cooperative_oscillator_number = 0;
			cooperative_oscillator_number += 1;
			cooperative_oscillator_number += count_all_subordinate_oscillators(WITHOUT_EFFECT(MidiEffectDetune),
																					 primary_oscillator_index);
			STACK_ARRAY(int16_t, cooperative_oscillator_indexes, cooperative_oscillator_number);
			cooperative_oscillator_indexes[0] = primary_oscillator_index;
			get_all_subordinate_oscillator_indexes(WITHOUT_EFFECT(MidiEffectDetune),
												   primary_oscillator_index,
												   &cooperative_oscillator_indexes[1]);
			float oscillator_detune_in_semitones = calculate_detune_detune_in_semitones(
						effective_detune, s_max_detune_detune_in_semitones);

			float detunes_in_semitones[DETUNE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER]
					= { 0.0f, oscillator_detune_in_semitones, -oscillator_detune_in_semitones};

			for(int k = 0; k < cooperative_oscillator_number; k++){
				oscillator_t  * const p_cooperative_oscillator
						= get_oscillator_pointer_from_index(cooperative_oscillator_indexes[k]);

				int32_t const enhanced_loudness = ENHANCE_DETUNE_LOUDNESS(p_cooperative_oscillator->loudness,
																		  effective_detune);
				int16_t const enhanced_loudness_over_64 = DIVIDE_BY_64(enhanced_loudness);
				int16_t bucket = enhanced_loudness_over_64 * (1 + DIVIDE_BY_8(effective_detune - 1));
				int16_t loudnesses[1 + DETUNE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER] = { 0, 0 , bucket, bucket};
				loudnesses[0] = enhanced_loudness - (loudnesses[1] + loudnesses[2] + loudnesses[3]);
				p_cooperative_oscillator->loudness = loudnesses[0];

				int16_t associate_oscillator_indexes[DETUNE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER];
				for(int16_t i = 0; i < DETUNE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER; i++){
					int16_t oscillator_index;
					oscillator_t * const p_oscillator
							= replicate_oscillator(cooperative_oscillator_indexes[k], &oscillator_index);
					if(NULL == p_oscillator){
						return -1;
					}
					associate_oscillator_indexes[i] = oscillator_index;
					p_oscillator->loudness = loudnesses[i + 1];
					p_oscillator->pitch_detune_in_semitones
							+= detunes_in_semitones[i];
					update_oscillator_phase_increment(p_oscillator);
					p_oscillator->midi_effect_association = MidiEffectDetune;
				}
				store_associate_oscillator_indexes(MidiEffectDetune, cooperative_oscillator_indexes[k],
											   &associate_oscillator_indexes[0]);
			}
			break;
		}
	} while(0);

	{
		int effect_subordinate_oscillator_number = 0;
		effect_subordinate_oscillator_number = count_all_subordinate_oscillators(MidiEffectDetune,
																				 primary_oscillator_index);
		STACK_ARRAY(int16_t, effect_subordinate_oscillator_indexes, effect_subordinate_oscillator_number);
		get_all_subordinate_oscillator_indexes(MidiEffectDetune, primary_oscillator_index,
											   &effect_subordinate_oscillator_indexes[0]);
		for(int16_t i = 0; i < effect_subordinate_oscillator_number; i++){
			put_event(event_type, effect_subordinate_oscillator_indexes[i], tick);
		}
	}
	return 0;
}

/**********************************************************************************/

#define REVERB_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER		(SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER)
#define MAX_REVERB_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND	\
													(0.150)
#define EACH_REVERB_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND	\
	(MAX_REVERB_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND / \
	(float)( REVERB_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER *(INT8_MAX_PLUS_1)) )


static float s_min_reverb_delta_tick =
		(float)(EACH_REVERB_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND * MIDI_DEFAULT_TEMPO * MIDI_DEFAULT_RESOLUTION / (60.0));

/**********************************************************************************/

static inline uint32_t calculate_reverb_delta_tick(normalized_midi_level_t reverb_depth)
{
	uint32_t reverb_delta_tick = (uint32_t)(reverb_depth * s_min_reverb_delta_tick + 0.5);
	reverb_delta_tick = ZERO_AS_ONE(reverb_delta_tick);
	return reverb_delta_tick;
}

/**********************************************************************************/

//#define ENHANCE_REVERB_LOUDNESS(LOUDNESS, REVERB_VALUE)	\
//	(LOUDNESS)

#define ENHANCE_REVERB_LOUDNESS(LOUDNESS, REVERB_VALUE) \
	( (int32_t)(LOUDNESS) + \
		DIVIDE_BY_8( \
				DIVIDE_BY_128( (int32_t)(LOUDNESS) * (int32_t)(REVERB_VALUE) ) \
		) \
	)

enum ReverbEffectProfile
{
	ReverbEffectProfileAudioRoom = 0,
	ReverbEffectProfileBathroom,
	ReverbEffectProfileCave,
	ReverbEffectProfileMax
};

static int16_t const s_reverb_loudness_proportional_coefficients \
	[ReverbEffectProfileMax][1 + REVERB_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER] =
{
	{ 10, 8, 6, 4}, // ReverbEffectProfileAudioRoom
	{ 10, 7, 9, 4}, // ReverbEffectProfileBathroom
	{ 10, 9, 6, 7}, // ReverbEffectProfileCave
};

static int process_reverb_effect(uint32_t const tick, int8_t const event_type,
								 int8_t const voice, midi_value_t const note,
								 normalized_midi_level_t const velocity,
								 int16_t const primary_oscillator_index)
{
	(void)note;
	(void)velocity;
	if(MIDI_PERCUSSION_CHANNEL == voice){
		return 1;
	}
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	if(0 == p_channel_controller->reverb){
		return 2;
	}

	do {
		if(EventTypeActivate == event_type){
			int cooperative_oscillator_number = 0;
			cooperative_oscillator_number += 1;
			cooperative_oscillator_number += count_all_subordinate_oscillators(WITHOUT_EFFECT(MidiEffectReverb),
																					 primary_oscillator_index);
			STACK_ARRAY(int16_t, cooperative_oscillator_indexes, cooperative_oscillator_number);
			cooperative_oscillator_indexes[0] = primary_oscillator_index;
			get_all_subordinate_oscillator_indexes(WITHOUT_EFFECT(MidiEffectReverb),
												   primary_oscillator_index,
												   &cooperative_oscillator_indexes[1]);

			uint8_t const reverb_effect_profile = ReverbEffectProfileAudioRoom;
			for(int k = 0; k < cooperative_oscillator_number; k++){
				oscillator_t  * const p_cooperative_oscillator
						= get_oscillator_pointer_from_index(cooperative_oscillator_indexes[k]);

				int32_t const enhanced_loudness = ENHANCE_REVERB_LOUDNESS(p_cooperative_oscillator->loudness,
																		  p_channel_controller->reverb);
				int16_t const enhanced_loudness_over_32 = DIVIDE_BY_32(enhanced_loudness);
				int16_t loudnesses[1 + REVERB_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER]
						= { 0,
							enhanced_loudness_over_32
								* s_reverb_loudness_proportional_coefficients[reverb_effect_profile][1],
							enhanced_loudness_over_32
								* s_reverb_loudness_proportional_coefficients[reverb_effect_profile][2],
							enhanced_loudness_over_32
								* s_reverb_loudness_proportional_coefficients[reverb_effect_profile][3]
						  };
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
					p_oscillator->midi_effect_association = MidiEffectReverb;
				}
				store_associate_oscillator_indexes(MidiEffectReverb, cooperative_oscillator_indexes[k],
											   &associate_oscillator_indexes[0]);
			}
			break;
		}
	} while(0);

	uint32_t const reverb_delta_tick = calculate_reverb_delta_tick(p_channel_controller->reverb);
	{
		int effect_subordinate_oscillator_number = 0;
		effect_subordinate_oscillator_number = count_all_subordinate_oscillators(MidiEffectReverb,
																				 primary_oscillator_index);
		STACK_ARRAY(int16_t, effect_subordinate_oscillator_indexes, effect_subordinate_oscillator_number);
		get_all_subordinate_oscillator_indexes(MidiEffectReverb, primary_oscillator_index,
											   &effect_subordinate_oscillator_indexes[0]);
		int jj = 0;
		for(int16_t i = 0; i < effect_subordinate_oscillator_number; i++){
			put_event(event_type, effect_subordinate_oscillator_indexes[i],
					  tick + (jj + 1) * reverb_delta_tick);
			jj += 1;
			jj /= REVERB_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER;
		}
	}
	return 0;
}

/**********************************************************************************/

#define CHORUS_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER		(SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER)
#define MAX_CHORUS_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND	\
													(0.033)
#define EACH_CHORUS_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND	\
	(MAX_CHORUS_OSCILLAOTERS_OVERALL_INTERVAL_IN_SECOND / \
	(float)( CHORUS_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER * (INT8_MAX_PLUS_1)) )

static float s_min_chorus_delta_tick =
		(float)(EACH_CHORUS_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND * MIDI_DEFAULT_TEMPO * MIDI_DEFAULT_RESOLUTION / (60.0));

#define	DEFAULT_MAX_CHORUS_DETUNE_IN_SEMITONE	(0.25f)
static float s_max_chorus_detune_in_semitones = DEFAULT_MAX_CHORUS_DETUNE_IN_SEMITONE;

/**********************************************************************************/

static inline uint32_t calculate_chorus_delta_tick(normalized_midi_level_t chorus_depth)
{
	uint32_t chorus_delta_tick = (uint32_t)(chorus_depth * s_min_chorus_delta_tick + 0.5);
	chorus_delta_tick = ZERO_AS_ONE(chorus_delta_tick);
	return chorus_delta_tick;
}

/**********************************************************************************/

//xor-shift pesudo random https://en.wikipedia.org/wiki/Xorshift
static uint32_t s_chorus_random_seed = 20240129;

static uint16_t generate_xorshift_random(void)
{
	s_chorus_random_seed ^= s_chorus_random_seed << 13;
	s_chorus_random_seed ^= s_chorus_random_seed >> 17;
	s_chorus_random_seed ^= s_chorus_random_seed << 5;
	return (uint16_t)(s_chorus_random_seed);
}

/**********************************************************************************/

static inline uint16_t generate_chorus_detune_value()
{
	return generate_xorshift_random();
}

/**********************************************************************************/

#define RANDOM_RANGE_TO_PLUS_MINUS_ONE(VALUE) \
	( ((int32_t)DIVIDE_BY_2(UINT16_MAX + 1) - (int32_t)(VALUE)) \
	 / (float)DIVIDE_BY_2(UINT16_MAX + 1) )

static float const calculate_chorus_detune_in_semitones(normalized_midi_level_t const chorus_depth,
											  float const max_chorus_detune_in_semitones)
{
	if(0 == chorus_depth){
		return 0.0;
	}

	float chorus_detune_semitones
			= RANDOM_RANGE_TO_PLUS_MINUS_ONE(generate_chorus_detune_value())
			* (float)chorus_depth/(float)(INT8_MAX_PLUS_1);
	chorus_detune_semitones *= max_chorus_detune_in_semitones;
	//CHIPTUNE_PRINTF(cDeveloping, "chorus_detune_semitones = %3.2f\r\n", chorus_detune_semitones);
	return chorus_detune_semitones;
}

/**********************************************************************************/

enum ChorusEffectProfile
{
	ChorusEffectProfileLeadDominant = 0,
	ChorusEffectProfileEvenEnsemble,
	ChorusEffectProfileMax
};

static int16_t const s_chorus_loudness_proportional_coefficients \
	[ChorusEffectProfileMax][1 + CHORUS_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER] =
{
	{ 11, 6, 7, 8}, // ChorusEffectProfileLeadDominant
	{ 8, 9, 7, 8}, // ChorusEffectProfileEvenEnsemble
};

static int process_chorus_effect(uint32_t const tick, int8_t const event_type,
								 int8_t const voice, midi_value_t const note,
								 normalized_midi_level_t const velocity,
								 int16_t const primary_oscillator_index)
{
	(void)note;
	(void)velocity;
	if(MIDI_PERCUSSION_CHANNEL == voice){
		return 1;
	}
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	if(0 == p_channel_controller->chorus){
		return 2;
	}

	do {
		if(EventTypeActivate == event_type){
			int cooperative_oscillator_number = 0;
			cooperative_oscillator_number += 1;
			cooperative_oscillator_number += count_all_subordinate_oscillators(WITHOUT_EFFECT(MidiEffectChorus),
																					 primary_oscillator_index);
			STACK_ARRAY(int16_t, cooperative_oscillator_indexes, cooperative_oscillator_number);
			cooperative_oscillator_indexes[0] = primary_oscillator_index;
			get_all_subordinate_oscillator_indexes(WITHOUT_EFFECT(MidiEffectChorus),
												   primary_oscillator_index,
												   &cooperative_oscillator_indexes[1]);

			uint8_t const chorus_effect_profile = ChorusEffectProfileLeadDominant;
			for(int k = 0; k < cooperative_oscillator_number; k++){
				oscillator_t *p_cooperative_oscillator
						= get_oscillator_pointer_from_index(cooperative_oscillator_indexes[k]);
				int16_t const loudness = p_cooperative_oscillator->loudness;
				int16_t loudness_over_32 = DIVIDE_BY_32(loudness);
				int16_t loudnesses[1 + CHORUS_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER]
						= { 0,
							loudness_over_32
								* s_chorus_loudness_proportional_coefficients[chorus_effect_profile][1],
							loudness_over_32
								* s_chorus_loudness_proportional_coefficients[chorus_effect_profile][2],
							loudness_over_32
								* s_chorus_loudness_proportional_coefficients[chorus_effect_profile][3]
						  };
				loudnesses[0] = loudness -  (loudnesses[1] + loudnesses[2] + loudnesses[3]);;
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
					p_oscillator->pitch_detune_in_semitones
							+= calculate_chorus_detune_in_semitones(
								p_channel_controller->chorus, s_max_chorus_detune_in_semitones);
					update_oscillator_phase_increment(p_oscillator);

					associate_oscillator_indexes[i] = oscillator_index;
					p_oscillator->midi_effect_association = MidiEffectChorus;
				}
				store_associate_oscillator_indexes(MidiEffectChorus, cooperative_oscillator_indexes[k],
											   &associate_oscillator_indexes[0]);
			}
			break;
		}
	} while(0);

	uint32_t const chorus_delta_tick = calculate_chorus_delta_tick(p_channel_controller->chorus);
	{
		int effect_subordinate_oscillator_number = 0;
		effect_subordinate_oscillator_number = count_all_subordinate_oscillators(MidiEffectChorus,
																				 primary_oscillator_index);
		STACK_ARRAY(int16_t, effect_subordinate_oscillator_indexes, effect_subordinate_oscillator_number);
		get_all_subordinate_oscillator_indexes(MidiEffectChorus, primary_oscillator_index,
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

int process_effects(uint32_t const tick, int8_t const event_type, int8_t const voice, midi_value_t const note,
					normalized_midi_level_t const velocity, int16_t const primary_oscillator_index)
{
	process_detune_effect(tick, event_type, voice, note, velocity, primary_oscillator_index);
	process_reverb_effect(tick, event_type, voice, note, velocity, primary_oscillator_index);
	process_chorus_effect(tick, event_type, voice, note, velocity, primary_oscillator_index);
	return 0;
}

/**********************************************************************************/

void update_effect_tick(void)
{
	float const playing_tempo = get_playing_tempo();
	uint32_t const resolution = get_resolution();
	s_min_reverb_delta_tick = (float)(EACH_REVERB_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND
									  * playing_tempo * resolution / 60.0);
	s_min_chorus_delta_tick = (float)(EACH_CHORUS_OSCILLATER_MIN_TIME_INTERVAL_IN_SECOND
									  * playing_tempo * resolution / 60.0);
}

