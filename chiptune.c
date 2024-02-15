#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include<stdarg.h>
#include <stdio.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_oscillator_internal.h"
#include "chiptune_event_internal.h"

#include "chiptune_midi_control_change_internal.h"

#include "chiptune.h"

#ifdef _INCREMENTAL_SAMPLE_INDEX
typedef float chiptune_float;
#else
typedef double chiptune_float;
#endif

#define DEFAULT_TEMPO								(120.0)
#define DEFAULT_SAMPLING_RATE						(16000)
#define DEFAULT_RESOLUTION							(960)

static float s_tempo = DEFAULT_TEMPO;
static uint32_t s_sampling_rate = DEFAULT_SAMPLING_RATE;
static uint32_t s_resolution = DEFAULT_RESOLUTION;
static bool s_is_stereo = false;
bool s_is_processing_left_channel = true;

#ifdef _INCREMENTAL_SAMPLE_INDEX
static uint32_t s_current_sample_index = 0;
static chiptune_float s_tick_to_sample_index_ratio = (chiptune_float)(DEFAULT_SAMPLING_RATE * 1.0/(DEFAULT_TEMPO/60.0)/DEFAULT_RESOLUTION);

#define RESET_CURRENT_TIME()						\
													do { \
														s_current_sample_index = 0; \
													} while(0)

#define UPDATE_SAMPLES_TO_TICK_RATIO()				\
													do { \
														s_tick_to_sample_index_ratio \
														= (chiptune_float)(s_sampling_rate * 60.0/s_tempo/s_resolution); \
													} while(0)

#define CORRECT_BASE_TIME()							\
													do { \
														s_current_sample_index = (uint32_t)(s_current_sample_index * s_tempo/tempo); \
													} while(0)

#define UPDATE_BASE_TIME_UNIT()						UPDATE_SAMPLES_TO_TICK_RATIO()

#define TICK_TO_SAMPLE_INDEX(TICK)					((uint32_t)(s_tick_to_sample_index_ratio * (chiptune_float)(TICK) + 0.5 ))

#define	IS_AFTER_CURRENT_TIME(TICK)					( (TICK_TO_SAMPLE_INDEX(TICK) > s_current_sample_index) ? true : false)
#define CURRENT_TICK()								( (uint32_t)(s_current_sample_index/(s_tick_to_sample_index_ratio) + 0.5))
#else
static chiptune_float s_current_tick = 0.0;
static chiptune_float s_delta_tick_per_sample = (DEFAULT_RESOLUTION / ( (chiptune_float)DEFAULT_SAMPLING_RATE/(DEFAULT_TEMPO /60.0) ) );

#define RESET_CURRENT_TIME()						\
													do { \
														s_current_tick = 0.0; \
													} while(0)

#define	UPDATE_DELTA_TICK_PER_SAMPLE()				\
													do { \
														s_delta_tick_per_sample = ( s_resolution * s_tempo / (chiptune_float)s_sampling_rate/ 60.0 ); \
													} while(0)

#define CORRECT_BASE_TIME()							\
													do { \
														(void)0; \
													} while(0)

#define UPDATE_BASE_TIME_UNIT()						UPDATE_DELTA_TICK_PER_SAMPLE()

#define	IS_AFTER_CURRENT_TIME(TICK)					(((chiptune_float)(TICK) > s_current_tick) ? true : false)
#define CURRENT_TICK()								((uint32_t)(s_current_tick + 0.5))
#endif

#define EACH_CHORUS_OSCILLAOTER_TIME_INTERVAL_IN_SECOND				(0.008)

uint32_t s_chorus_delta_tick = (uint32_t)(EACH_CHORUS_OSCILLAOTER_TIME_INTERVAL_IN_SECOND * DEFAULT_TEMPO * DEFAULT_RESOLUTION / (60.0) + 0.5);

#define	UPDATE_CHORUS_DELTA_TICK()					\
													do { \
														 s_chorus_delta_tick \
															= (uint32_t)(EACH_CHORUS_OSCILLAOTER_TIME_INTERVAL_IN_SECOND * s_tempo * s_resolution/ 60.0 + 0.5); \
													} while(0)

#define SINE_TABLE_LENGTH							(2048)
static int16_t s_sine_table[SINE_TABLE_LENGTH]		= {0};

/**********************************************************************************/

static int(*s_handler_get_midi_message)(uint32_t index, uint32_t * const p_tick, uint32_t * const p_message) = NULL;

static bool s_is_tune_ending = false;

/**********************************************************************************/

void chiptune_set_midi_message_callback( int(*handler_get_midi_message)(uint32_t index, uint32_t * const p_tick, uint32_t * const p_message) )
{
	s_handler_get_midi_message = handler_get_midi_message;
}

/**********************************************************************************/

uint32_t const get_sampling_rate(void) { return s_sampling_rate; }

/**********************************************************************************/

uint32_t const get_resolution(void) { return s_resolution; }

/**********************************************************************************/

float const get_tempo(void) { return s_tempo; }

/**********************************************************************************/

static inline bool const is_stereo() { return s_is_stereo;}

/**********************************************************************************/

static inline bool const is_processing_left_channel() { return s_is_processing_left_channel; }

/**********************************************************************************/

static inline void swap_processing_channel() { s_is_processing_left_channel = !s_is_processing_left_channel; }

/**********************************************************************************/

static int process_program_change_message(uint32_t const tick, int8_t const voice, uint8_t const number)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_PROGRAM_CHANGE :: ", tick);
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0 == voice
			|| MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1 == voice){
		p_channel_controller->waveform = WAVEFORM_NOISE;
		p_channel_controller->envelope_sustain_level = 8;
	}

	switch(p_channel_controller->waveform)
	{
	case WAVEFORM_SQUARE:
		CHIPTUNE_PRINTF(cMidiSetup, "%voice = %d instrument = %d, is WAVEFORM_SQUARE\r\n", voice, number);
		break;
	case WAVEFORM_TRIANGLE:
		CHIPTUNE_PRINTF(cMidiSetup, "%voice = %d instrument = %d, is WAVEFORM_TRIANGLE\r\n", voice, number);
		break;
	case WAVEFORM_SAW:
		CHIPTUNE_PRINTF(cMidiSetup, "%voice = %d instrument = %d, is WAVEFORM_SAW\r\n", voice, number);
		break;
	case WAVEFORM_NOISE:
		CHIPTUNE_PRINTF(cMidiSetup, "%voice = %d instrument = %d, is WAVEFORM_NOISE\r\n", voice, number);
		break;
	default:
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: tick = %u, MIDI_MESSAGE_PROGRAM_CHANGE :: "
									 " %voice = %d instrument = %d, is WAVEFORM_UNKOWN\r\n", voice, number);
	}
	return 0;
}

/**********************************************************************************/

#define DIVIDE_BY_16(VALUE)							((VALUE) >> 4)
#define OSCILLATOR_NUMBER_FOR_CHORUS(VALUE)			(DIVIDE_BY_16(((VALUE) + 15)))

int process_chorus_effect(uint32_t const tick, int8_t const event_type,
						  int8_t const voice, int8_t const note, int8_t const velocity,
						  int16_t const native_oscillator_index)
{
	(void)velocity;
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	if(0 >= p_channel_controller->chorus){
		return 1;
	}

	oscillator_t  * const p_native_oscillator = get_event_oscillator_pointer_from_index(native_oscillator_index);
#define ASSOCIATE_CHORUS_OSCILLATOR_NUMBER			(4 - 1)
	int oscillator_indexes[ASSOCIATE_CHORUS_OSCILLATOR_NUMBER] = {UNUSED_OSCILLATOR, UNUSED_OSCILLATOR, UNUSED_OSCILLATOR};

	do {
		if(EVENT_ACTIVATE == event_type){
			int16_t const loudness = p_native_oscillator->loudness;
			int16_t averaged_loudness = DIVIDE_BY_16(loudness);
			// oscillator 1 : 4 * averaged_volume
			// oscillator 2 : 5 * averaged_volume
			// oscillator 3 : 6 * averaged_volume
			// oscillator 0 : volume - (4 + 5  + 6) * averaged_volume
			int16_t oscillator_loudness = loudness - (4 + 5 + 6) * averaged_loudness;
			p_native_oscillator->loudness = oscillator_loudness;

			float pitch_wheel_bend_in_semitone = 0.0f;
			oscillator_loudness = 4 * averaged_loudness;
			for(int16_t i = 0; i < ASSOCIATE_CHORUS_OSCILLATOR_NUMBER; i++){
				int16_t oscillator_index;
				oscillator_t * const p_oscillator = acquire_event_freed_oscillator(&oscillator_index);
				if(NULL == p_oscillator){
					return -1;
				}
				int8_t const vibrato_modulation_in_semitone = p_channel_controller->vibrato_modulation_in_semitone;
				memcpy(p_oscillator, p_native_oscillator, sizeof(oscillator_t));
				p_oscillator->loudness = oscillator_loudness;
				p_oscillator->pitch_chorus_bend_in_semitone = obtain_oscillator_pitch_chorus_bend_in_semitone(voice);
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
				p_oscillator->native_oscillator = native_oscillator_index;
				SET_CHORUS_ASSOCIATE(p_oscillator->state_bits);
				oscillator_indexes[i] = oscillator_index;
			}
			break;
		}

		int16_t kk = 0;
		bool is_all_found = false;
		int16_t oscillator_index = get_event_occupied_oscillator_head_index();
		int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();
		for(int16_t i = 0; i < occupied_oscillator_number; i++){
			oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
			do {
				if(true != IS_CHORUS_ASSOCIATE(p_oscillator->state_bits)){
					break;
				}
				if(native_oscillator_index != p_oscillator->native_oscillator){
					break;
				}
				if(note != p_oscillator->note){
					break;
				}
				if(voice != p_oscillator->voice){
					break;
				}
				// it is accociate oscillator, not directly related to scores
				//if(true == IS_FREEING(p_oscillator->state_bits)){
				//	break;
				//	}
				oscillator_indexes[kk] = oscillator_index;
				kk += 1;
			} while(0);

			if(ASSOCIATE_CHORUS_OSCILLATOR_NUMBER == kk){
				is_all_found = true;
				break;
			}
			oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
		}

		if(false == is_all_found){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR::targeted oscillator voice = %d, note = %d could not be found\r\n", voice, note);
			return -2;
		}
	} while(0);

	for(int16_t i = 0; i < ASSOCIATE_CHORUS_OSCILLATOR_NUMBER; i++){
		put_event(event_type, oscillator_indexes[i], tick + (i + 1) * s_chorus_delta_tick);
	}

	return 0;
}



#define DIVIDE_BY_16(VALUE)							((VALUE) >> 4)
#define OSCILLATOR_NU

static int8_t s_flat_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH];

static int8_t s_linear_decline_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH];
static int8_t s_linear_growth_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH];

static int8_t s_exponential_decline_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH];
static int8_t s_exponential_growth_table[CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH];


int process_percussion_effect(uint32_t const tick,
									int8_t const voice, int8_t const note, int8_t const velocity,
									oscillator_t * const p_oscillator)
{
	if(false == (MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0 == voice ||
			MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1 == voice)){
		return 1;
	}
	(void)velocity;
	p_oscillator->percussion_waveform_index = 0;
	p_oscillator->percussion_duration_sample_count = 0;
	p_oscillator->percussion_same_index_count = 0;
	p_oscillator->percussion_table_index = 0;

	percussion_t const * const p_percussion = get_percussion_pointer_from_index(note);
	p_oscillator->delta_phase = p_percussion->delta_phase;
	p_oscillator->amplitude
			= (int16_t)(((uint32_t)p_oscillator->loudness
						 * p_percussion->p_modulation_envelope_table[p_oscillator->percussion_table_index]) >> 7);

	return 0;
}

/**********************************************************************************/

static void rest_occupied_oscillator_with_same_voice_note(uint32_t const tick,
														  int8_t const voice, int8_t const note, int8_t const velocity)
{
	int16_t oscillator_index = get_event_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();
	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
		do {
			if(note != p_oscillator->note){
				break;
			}
			if(voice != p_oscillator->voice){
				break;
			}
			if(UNUSED_OSCILLATOR != p_oscillator->native_oscillator){
				break;
			}
			if(true == IS_FREEING(p_oscillator->state_bits)){
				break;
			}
			if(true == IS_RESTING(p_oscillator->state_bits)){
				break;
			}

			put_event(EVENT_REST, oscillator_index, tick);
			process_chorus_effect(tick, EVENT_REST, voice, note, velocity, oscillator_index);
		} while(0);
		oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
	}
}

/**********************************************************************************/


static int process_note_message(uint32_t const tick, bool const is_note_on,
						 int8_t const voice, int8_t const note, int8_t const velocity)
{
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	float pitch_wheel_bend_in_semitone = 0.0f;
	int8_t actual_velocity = velocity;
	do {
		if(true == is_note_on){
			rest_occupied_oscillator_with_same_voice_note(tick, voice, note, velocity);

			int16_t oscillator_index;
			oscillator_t * const p_oscillator = acquire_event_freed_oscillator(&oscillator_index);
			if(NULL == p_oscillator){
				return -1;
			}
			RESET_STATE_BITES(p_oscillator->state_bits);
			SET_NOTE_ON(p_oscillator->state_bits);
			p_oscillator->voice = voice;
			p_oscillator->note = note;
			do
			{
				if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0 == voice ||
						MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1 == voice){
					process_percussion_effect(tick, voice, note, velocity, p_oscillator);
					break;
				}

				p_oscillator->pitch_chorus_bend_in_semitone = 0;
				p_oscillator->delta_phase
						= calculate_oscillator_delta_phase(p_oscillator->note,
														   p_channel_controller->tuning_in_semitones,
														   p_channel_controller->pitch_wheel_bend_range_in_semitones,
														   p_channel_controller->pitch_wheel,
														   p_oscillator->pitch_chorus_bend_in_semitone,
														   &pitch_wheel_bend_in_semitone);

				p_oscillator->max_delta_vibrato_phase
						= calculate_oscillator_delta_phase(
							p_oscillator->note + p_channel_controller->vibrato_modulation_in_semitone,
							p_channel_controller->tuning_in_semitones,
							p_channel_controller->pitch_wheel_bend_range_in_semitones,
							p_channel_controller->pitch_wheel,
							p_oscillator->pitch_chorus_bend_in_semitone, &pitch_wheel_bend_in_semitone)
						- p_oscillator->delta_phase;

				p_oscillator->vibrato_table_index = 0;
				p_oscillator->vibrato_same_index_count = 0;

				p_oscillator->envelope_same_index_count = 0;
				p_oscillator->envelope_table_index = 0;
				p_oscillator->release_reference_amplitude = 0;

			}while(0);

			p_oscillator->loudness = (uint16_t)actual_velocity * (uint16_t)p_channel_controller->playing_volume;
			p_oscillator->amplitude = 0;
			p_oscillator->current_phase = 0;

			p_oscillator->native_oscillator = UNUSED_OSCILLATOR;
			put_event(EVENT_ACTIVATE, oscillator_index, tick);
			process_chorus_effect(tick, EVENT_ACTIVATE, voice, note, velocity, oscillator_index);
			break;
		}

		bool is_found = false;
		int16_t oscillator_index = get_event_occupied_oscillator_head_index();
		int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();
		for(int16_t i = 0; i < occupied_oscillator_number; i++){
			oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
			do
			{
				if(note != p_oscillator->note){
					break;
				}
				if(voice != p_oscillator->voice){
					break;
				}

				if(UNUSED_OSCILLATOR != p_oscillator->native_oscillator){
					break;
				}
				if(false == IS_NOTE_ON(p_oscillator->state_bits)){
					break;
				}
				if(true == IS_FREEING(p_oscillator->state_bits)){
					break;
				}
				put_event(EVENT_FREE, oscillator_index, tick);
				process_chorus_effect(tick, EVENT_FREE, voice, note, velocity, oscillator_index);
				is_found = true;
			} while(0);
			if(true == is_found){
				break;
			}
			oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
		}

		if(false == is_found){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR::no corresponding note for off :: tick = %u, voice = %d,  note = %u\r\n",
							tick, voice, note);
			return -2;
		}

		if(true == p_channel_controller->is_damper_pedal_on){
			int16_t const original_oscillator_index = oscillator_index;
			int16_t reduced_loundness_oscillator_index;
			oscillator_t * const p_oscillator = acquire_event_freed_oscillator(&reduced_loundness_oscillator_index);
			memcpy(p_oscillator, get_event_oscillator_pointer_from_index(original_oscillator_index),
				   sizeof(oscillator_t));
			RESET_STATE_BITES(p_oscillator->state_bits);
			SET_NOTE_OFF(p_oscillator->state_bits);
			p_oscillator->loudness =
					LOUNDNESS_AS_DAMPING_PEDAL_ON_BUT_NOTE_OFF(p_oscillator->loudness,
															   p_channel_controller->damper_on_but_note_off_loudness_level);
			p_oscillator->amplitude = 0;
			p_oscillator->envelope_same_index_count = 0;
			p_oscillator->envelope_table_index = 0;
			p_oscillator->release_reference_amplitude = 0;
			put_event(EVENT_ACTIVATE, reduced_loundness_oscillator_index, tick);
			process_chorus_effect(tick, EVENT_ACTIVATE, voice, note, velocity, reduced_loundness_oscillator_index);
		}
	} while(0);

	char pitch_wheel_bend_string[32] = "";
	if(true == is_note_on){
		if(0.0f != pitch_wheel_bend_in_semitone){
			snprintf(&pitch_wheel_bend_string[0], sizeof(pitch_wheel_bend_string),
					", pitch wheel bend in semitone = %+3.2f", pitch_wheel_bend_in_semitone);
		}
	}

	if(voice == 9){
	CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %d, note = %d, velocity = %d%s\r\n",
					tick, is_note_on ? "MIDI_MESSAGE_NOTE_ON" : "MIDI_MESSAGE_NOTE_OFF" ,
					voice, note, velocity, &pitch_wheel_bend_string[0]);
	}
	return 0;
}

/**********************************************************************************/

static int process_pitch_wheel_message(uint32_t const tick, int8_t const voice, int16_t const value)
{
	char delta_hex_string[12] = "";
	do {
		if(value == MIDI_PITCH_WHEEL_CENTER){
			break;
		}
		if(value > MIDI_PITCH_WHEEL_CENTER){
			snprintf(&delta_hex_string[0], sizeof(delta_hex_string),"(+0x%04x)", value - MIDI_PITCH_WHEEL_CENTER);
			break;
		}
		snprintf(&delta_hex_string[0], sizeof(delta_hex_string),"(-0x%04x)", MIDI_PITCH_WHEEL_CENTER - value);
	} while(0);
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_PITCH_WHEEL :: voice = %d, value = 0x%04x %s\r\n",
					tick, voice, value, &delta_hex_string[0]);

	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	p_channel_controller->pitch_wheel = value;

	int16_t oscillator_index = get_event_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();

	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
		do {
			if(voice != p_oscillator->voice){
				break;
			}
			float pitch_bend_in_semitone;
			p_oscillator->delta_phase = calculate_oscillator_delta_phase(p_oscillator->note,
																		 p_channel_controller->tuning_in_semitones,
																		 p_channel_controller->pitch_wheel_bend_range_in_semitones,
																		 p_channel_controller->pitch_wheel, p_oscillator->pitch_chorus_bend_in_semitone,
																		 &pitch_bend_in_semitone);
			CHIPTUNE_PRINTF(cMidiSetup, "---- voice = %d, note = %d, pitch bend in semitone = %+3.2f\r\n",
							voice, p_oscillator->note, pitch_bend_in_semitone);
		} while(0);
		oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
	}
	return 0;
}

/**********************************************************************************/

struct _tick_message
{
	uint32_t tick;
	uint32_t message;
};
#ifndef NULL_TICK
	#define NULL_TICK								(UINT32_MAX)
#endif
#define NULL_MESSAGE							(0)

#define IS_NULL_TICK_MESSAGE(MESSAGE_TICK)			\
								(((NULL_TICK == (MESSAGE_TICK).tick) && (NULL_MESSAGE == (MESSAGE_TICK).message) ) ? true : false)

#define SET_TICK_MESSAGE_NULL(MESSAGE_TICK)			\
								do { \
									(MESSAGE_TICK).tick = NULL_TICK; \
									(MESSAGE_TICK).message = NULL_MESSAGE; \
								} while(0)

#define SEVEN_BITS_VALID(VALUE)						((0x7F) & (VALUE))

static int process_midi_message(struct _tick_message const tick_message)
{
	if(true == IS_NULL_TICK_MESSAGE(tick_message)){
		return 1;
	}

	union {
		uint32_t data_as_uint32;
		unsigned char data_as_bytes[4];
	} u;

	const uint32_t tick = tick_message.tick;
	u.data_as_uint32 = tick_message.message;

	uint8_t type = u.data_as_bytes[0] & 0xF0;
	int8_t voice = u.data_as_bytes[0] & 0x0F;
#define MIDI_MESSAGE_NOTE_OFF						(0x80)
#define MIDI_MESSAGE_NOTE_ON						(0x90)
#define MIDI_MESSAGE_KEY_PRESSURE					(0xA0)
#define MIDI_MESSAGE_CONTROL_CHANGE					(0xB0)
#define MIDI_MESSAGE_PROGRAM_CHANGE					(0xC0)
#define MIDI_MESSAGE_CHANNEL_PRESSURE				(0xD0)
#define MIDI_MESSAGE_PITCH_WHEEL					(0xE0)
	switch(type)
	{
	case MIDI_MESSAGE_NOTE_OFF:
	case MIDI_MESSAGE_NOTE_ON:
		process_note_message(tick, (MIDI_MESSAGE_NOTE_OFF == type) ? false : true,
			voice, SEVEN_BITS_VALID(u.data_as_bytes[1]), SEVEN_BITS_VALID(u.data_as_bytes[2]));
	 break;
	case MIDI_MESSAGE_KEY_PRESSURE:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_CHANNEL_PRESSURE :: note = %d, amount = %d %s\r\n",
						tick, voice, SEVEN_BITS_VALID(u.data_as_bytes[1]), "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_MESSAGE_CONTROL_CHANGE:
		process_control_change_message(tick, voice, SEVEN_BITS_VALID(u.data_as_bytes[1]), SEVEN_BITS_VALID(u.data_as_bytes[2]));
		break;
	case MIDI_MESSAGE_PROGRAM_CHANGE:
		process_program_change_message(tick, voice, SEVEN_BITS_VALID(u.data_as_bytes[1]));
		break;
	case MIDI_MESSAGE_CHANNEL_PRESSURE:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_CHANNEL_PRESSURE :: voice = %d, amount = %d %s\r\n",
						tick, voice, SEVEN_BITS_VALID(u.data_as_bytes[1]), "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_MESSAGE_PITCH_WHEEL:
#define COMBINE_AS_PITCH_WHEEL_14BITS(BYTE1, BYTE2)	\
													(SEVEN_BITS_VALID(BYTE2) << 7) | SEVEN_BITS_VALID(BYTE1))
		process_pitch_wheel_message(tick, voice, COMBINE_AS_PITCH_WHEEL_14BITS(u.data_as_bytes[1], u.data_as_bytes[2]);
		break;
	default:
		//CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE code = %u :: voice = %d, byte 1 = %d, byte 2 = %d %s\r\n",
		//				tick, type, voice, SEVEN_BITS_VALID(u.data_as_bytes[1]), SEVEN_BITS_VALID(u.data_as_bytes[2]), "(NOT IMPLEMENTED YET)");
		break;
	}
	return 0;
}

/**********************************************************************************/

uint32_t s_total_message_number = 0;

static int fetch_midi_tick_message(uint32_t index, struct _tick_message *p_tick_message)
{
	uint32_t tick;
	uint32_t message;
	int ret = s_handler_get_midi_message(index, &tick, &message);
	do {
		if(0 > ret){
			SET_TICK_MESSAGE_NULL(*p_tick_message);
			break;
		}

		p_tick_message->tick = tick;
		p_tick_message->message = message;
	} while(0);

	return ret;
}

/**********************************************************************************/

static int release_all_channels_damper_pedal(const uint32_t tick)
{
	int ret = 0;
	int16_t oscillator_index = get_event_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();
	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
		channel_controller_t * const p_channel_controller
				= get_channel_controller_pointer_from_index(p_oscillator->voice);

		if(true == p_channel_controller->is_damper_pedal_on){
			ret = 1;
			put_event(EVENT_FREE, oscillator_index, tick);
		}
		oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
	}

	return ret;
}

/**********************************************************************************/

int process_ending(const uint32_t tick)
{
	return release_all_channels_damper_pedal(tick);
}

/**********************************************************************************/

uint32_t s_previous_run_timely_tick = NULL_TICK;

uint32_t s_midi_messge_index = 0;
struct _tick_message s_fetched_tick_message = {NULL_TICK, NULL_MESSAGE};
uint32_t s_fetched_event_tick = NULL_TICK;

static int process_timely_midi_message_and_event(void)
{
	if(CURRENT_TICK() == s_previous_run_timely_tick){
		return 1;
	}

	int ret = 0;
	while(1)
	{
		do{
			if(false == IS_NULL_TICK_MESSAGE(s_fetched_tick_message)){
				break;
			}
			fetch_midi_tick_message(s_midi_messge_index, &s_fetched_tick_message);
			s_midi_messge_index += 1;
		} while(0);

		if(NULL_TICK == s_fetched_event_tick){
			s_fetched_event_tick = get_next_event_triggering_tick();
		}

		if(true == IS_NULL_TICK_MESSAGE(s_fetched_tick_message)
				&& NULL_TICK == s_fetched_event_tick){
			if(0 == process_ending(CURRENT_TICK())){
				ret = -1;
				break;
			}
		}

		bool is_both_after_current_tick = true;

		do {
			if(true == IS_AFTER_CURRENT_TIME(s_fetched_tick_message.tick)){
				break;
			}
			process_midi_message(s_fetched_tick_message);
			SET_TICK_MESSAGE_NULL(s_fetched_tick_message);
			is_both_after_current_tick = false;

			s_fetched_event_tick = get_next_event_triggering_tick();
		} while(0);

		do {
			if(NULL_TICK == s_fetched_event_tick){
				break;
			}
			if(true == IS_AFTER_CURRENT_TIME(s_fetched_event_tick)){
				break;
			}
			process_events(s_fetched_event_tick);
			s_fetched_event_tick = NULL_TICK;
			is_both_after_current_tick = false;
		} while(0);

		if(true == is_both_after_current_tick){
			break;
		}
	}

	s_previous_run_timely_tick = CURRENT_TICK();
	return ret;
}

/**********************************************************************************/

static int32_t get_max_simultaneous_loudness(void)
{
	SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(false);
	uint32_t midi_messge_index = 0;
	int32_t max_loudness = 0;
	uint32_t previous_tick;

	struct _tick_message tick_message;
	uint32_t event_tick = NULL_TICK;

	fetch_midi_tick_message(midi_messge_index, &tick_message);
	midi_messge_index += 1;
	process_midi_message(tick_message);
	previous_tick = tick_message.tick;
	SET_TICK_MESSAGE_NULL(tick_message);

	uint32_t tick = NULL_TICK;
	while(1)
	{
		do {
			if(false == IS_NULL_TICK_MESSAGE(tick_message)){
				break;
			}

			fetch_midi_tick_message(midi_messge_index, &tick_message);
			midi_messge_index += 1;
		} while(0);

		if(NULL_TICK == event_tick){
			event_tick = get_next_event_triggering_tick();
		}

		if(true == IS_NULL_TICK_MESSAGE(tick_message)
				&& NULL_TICK == event_tick){
			if(0 == process_ending(tick)){
				break;
			}
		}

		tick = (tick_message.tick < event_tick) ? tick_message.tick : event_tick;

		do
		{
			if(tick == previous_tick){
				break;
			}
			previous_tick = tick;

			int32_t sum_loudness = 0;

			int16_t oscillator_index = get_event_occupied_oscillator_head_index();
			int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();
			for(int16_t i = 0; i < occupied_oscillator_number; i++){
				oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
				if(false == IS_ACTIVATED(p_oscillator->state_bits)){
					continue;
				}
				int16_t loudness = p_oscillator->loudness;
				if(false == IS_NOTE_ON(p_oscillator->state_bits)){
					channel_controller_t const *p_channel_controller
							= get_channel_controller_pointer_from_index(p_oscillator->voice);
					uint8_t damper_on_but_note_off_loudness_level
							= p_channel_controller->damper_on_but_note_off_loudness_level;
					loudness = LOUNDNESS_AS_DAMPING_PEDAL_ON_BUT_NOTE_OFF(loudness,
																		  damper_on_but_note_off_loudness_level);
				}
				sum_loudness += loudness;
				oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
			}

			if(sum_loudness > max_loudness){
				max_loudness = sum_loudness;
			}
		}while(0);

		if(tick == tick_message.tick){
			process_midi_message(tick_message);
			SET_TICK_MESSAGE_NULL(tick_message);

			event_tick = get_next_event_triggering_tick();
		}

		if(tick == event_tick){
			process_events(event_tick);
			event_tick = NULL_TICK;
		}
	}

	SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(true);

	if(0 != get_upcoming_event_number()){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: not all events are released, remain %d events\r\n",
						get_upcoming_event_number());
	}

	if(0 != get_event_occupied_oscillator_number()){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: not all oscillators are released\r\n");

		int16_t oscillator_index = get_event_occupied_oscillator_head_index();
		int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();
		for(int16_t i = 0; i < occupied_oscillator_number; i++){
			oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);

			CHIPTUNE_PRINTF(cDeveloping, "oscillator = %d, voice = %d, note = 0x%02x(%d)\r\n",
							oscillator_index, p_oscillator->voice, p_oscillator->note, p_oscillator->note);

			oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
		}
	}

	return max_loudness;
}

#ifdef _RIGHT_SHIFT_FOR_NORMALIZING_AMPLITUDE

/**********************************************************************************/

uint32_t number_of_roundup_to_power2_left_shift_bits(uint32_t const value)
{
	uint32_t v = value; // compute the next highest power of 2 of 32-bit v

	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	uint32_t i = 0;
	for(i = 0; i < sizeof(uint32_t) * 8; i++){
		if(0x01 & (v >> i)){
			break;
		}
	}

	return i;
}

int32_t g_loudness_nomalization_right_shift = 16;
#define UPDATE_AMPLITUDE_NORMALIZER()				\
													do { \
														uint32_t max_loudness = get_max_simultaneous_loudness(); \
														g_loudness_nomalization_right_shift \
															= number_of_roundup_to_power2_left_shift_bits(max_loudness);\
													} while(0)
#else
int32_t g_max_loudness = 1 << 16;
#define UPDATE_AMPLITUDE_NORMALIZER()				\
													do { \
														g_max_loudness = get_max_simultaneous_loudness(); \
													} while(0)
#endif

/**********************************************************************************/

void chiptune_initialize(bool is_stereo,
						 uint32_t const sampling_rate, uint32_t const resolution, uint32_t const total_message_number)
{
	s_is_stereo = is_stereo;
	s_is_processing_left_channel = true;
	s_sampling_rate = sampling_rate;
	s_resolution = resolution;
	s_total_message_number = total_message_number;

	RESET_CURRENT_TIME();
	s_is_tune_ending = false;
	s_midi_messge_index = 0;
	s_previous_run_timely_tick = NULL_TICK;
	SET_TICK_MESSAGE_NULL(s_fetched_tick_message);

	for(int i = 0; i < SINE_TABLE_LENGTH; i++){
		s_sine_table[i] = (int16_t)(INT16_MAX * sinf((float)(2.0 * M_PI * i/SINE_TABLE_LENGTH)));
	}

	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_flat_table[i] = INT8_MAX;
	}
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		int ii = (CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH  - i);
		s_linear_decline_table[i] = (int8_t)(INT8_MAX * (ii/(float)CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH));
	}
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_linear_growth_table[i]
				= s_linear_decline_table[(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1) - i];
	}

	/*
	 * exponential :
	 *  INT8_MAX * exp(-alpha * (TABLE_LENGTH -1)) = 1 -> alpha = -ln(1/INT8_MAX)/(TABLE_LENGTH -1)
	*/
	const float alpha = -logf(1/(float)INT8_MAX)/(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1);
	s_exponential_decline_table[0] = INT8_MAX;
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_exponential_decline_table[i] = (int8_t)(INT8_MAX * expf( -alpha * i) + 0.5);
	}
	for(int16_t i = 0; i < CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH; i++){
		s_exponential_growth_table[i]
				= s_exponential_decline_table[(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1) - i];
	}

	initialize_channel_controller();
	clean_all_events();

	UPDATE_BASE_TIME_UNIT();
	UPDATE_AMPLITUDE_NORMALIZER();
	process_timely_midi_message_and_event();
	return ;
}

/**********************************************************************************/

void chiptune_set_tempo(float const tempo)
{
	CHIPTUNE_PRINTF(cMidiSetup, "%s :: tempo = %3.1f\r\n", __FUNCTION__,tempo);
	CORRECT_BASE_TIME();
	s_tempo = tempo;
	UPDATE_BASE_TIME_UNIT();
	UPDATE_CHORUS_DELTA_TICK();
	update_channel_controller_parameters_related_to_tempo();
}

/**********************************************************************************/

#ifdef _INCREMENTAL_SAMPLE_INDEX
#define INCREMENT_BASE_TIME()						\
													do { \
														s_current_sample_index += 1; \
													} while(0)
#else
#define INCREMENT_BASE_TIME()						\
													do { \
														s_current_tick += s_delta_tick_per_sample; \
													} while(0)
#endif

/**********************************************************************************/

void perform_vibrato(oscillator_t * const p_oscillator)
{
	do {
		if(true == is_stereo()
				&& false == is_processing_left_channel()){
			break;
		}

		if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0 == p_oscillator->voice ||
				MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1 == p_oscillator->voice){
			break;
		}

		channel_controller_t *p_channel_controller = get_channel_controller_pointer_from_index(p_oscillator->voice);
		int8_t const modulation_wheel = p_channel_controller->modulation_wheel;
		if(0 >= modulation_wheel){
			break;
		}
		uint16_t const vibrato_same_index_number = p_channel_controller->vibrato_same_index_number;
		p_oscillator->current_phase += DELTA_VIBTRATO_PHASE(modulation_wheel, p_oscillator->max_delta_vibrato_phase,
															p_channel_controller->p_vibrato_phase_table[
																p_oscillator->vibrato_table_index]);

		p_oscillator->vibrato_same_index_count += 1;
		if(vibrato_same_index_number == p_oscillator->vibrato_same_index_count){
			p_oscillator->vibrato_same_index_count = 0;
			p_oscillator->vibrato_table_index = REMAINDER_OF_DIVIDE_BY_CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH(
						p_oscillator->vibrato_table_index  + 1);
		}
	} while(0);
}


/**********************************************************************************/

void perform_melody_envelope(oscillator_t * const p_oscillator)
{
	do {
		if(true == is_stereo()
				&& false == is_processing_left_channel()){
			break;
		}

		if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0 == p_oscillator->voice ||
				MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1 == p_oscillator->voice){
			break;
		}

		channel_controller_t const *p_channel_controller
				= get_channel_controller_pointer_from_index(p_oscillator->voice);

		if(ENVELOPE_SUSTAIN == p_oscillator->envelope_state){
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
		case ENVELOPE_ATTACK:
			envelope_same_index_number = p_channel_controller->envelope_attack_same_index_number;
			break;
		case ENVELOPE_DECAY:
			envelope_same_index_number = p_channel_controller->envelope_decay_same_index_number;
			break;
		case ENVELOPE_SUSTAIN:
			envelope_same_index_number = p_channel_controller->envelope_damper_on_but_note_off_sustain_same_index_number;
			break;
		case ENVELOPE_RELEASE:
			envelope_same_index_number = p_channel_controller->envelope_release_same_index_number;
			break;
		};

		p_oscillator->envelope_same_index_count += 1;
		if(envelope_same_index_number > p_oscillator->envelope_same_index_count){
			break;
		}

		p_oscillator->envelope_same_index_count = 0;
		p_oscillator->envelope_table_index += 1;

		bool is_out_of_lookup_table_range = false;
		do
		{
			if(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH > p_oscillator->envelope_table_index){
				break;
			}

			p_oscillator->envelope_table_index = 0;
			switch(p_oscillator->envelope_state)
			{
			case ENVELOPE_ATTACK:
				p_oscillator->envelope_state = ENVELOPE_DECAY;
				p_oscillator->amplitude = p_oscillator->loudness;
				break;
			case ENVELOPE_DECAY:
				p_oscillator->envelope_state = ENVELOPE_SUSTAIN;
				p_oscillator->amplitude = SUSTAIN_AMPLITUDE(p_oscillator->loudness,
								  p_channel_controller->envelope_sustain_level);
				break;
			case ENVELOPE_SUSTAIN:
			case ENVELOPE_RELEASE:
				SET_DEACTIVATED(p_oscillator->state_bits);
				break;
			}
			p_oscillator->release_reference_amplitude = p_oscillator->amplitude;
			is_out_of_lookup_table_range = true;
		} while(0);
		if(true == is_out_of_lookup_table_range){
			break;
		}

		int8_t const * p_envelope_table = NULL;
		int16_t delta_amplitude = 0;
		int16_t shift_amplitude = 0;
		switch(p_oscillator->envelope_state)
		{
		default:
		case ENVELOPE_ATTACK:
			p_envelope_table = p_channel_controller->p_envelope_attack_table;
			delta_amplitude = p_oscillator->loudness;
			break;
		case ENVELOPE_DECAY: {
			p_envelope_table = p_channel_controller->p_envelope_decay_table;
			int16_t sustain_ampitude = SUSTAIN_AMPLITUDE(p_oscillator->loudness,
														 p_channel_controller->envelope_sustain_level);
			delta_amplitude = p_oscillator->loudness - sustain_ampitude;
			shift_amplitude = sustain_ampitude;
		}	break;
		case ENVELOPE_SUSTAIN :
			p_envelope_table = p_channel_controller->p_envelope_damper_on_but_note_off_sustain_table;
			delta_amplitude = p_oscillator->loudness;
			break;
		case ENVELOPE_RELEASE:
			p_envelope_table = p_channel_controller->p_envelope_release_table;
			delta_amplitude = p_oscillator->release_reference_amplitude;
			break;
		}

		p_oscillator->amplitude = ENVELOPE_AMPLITUDE(delta_amplitude,
													 p_envelope_table[p_oscillator->envelope_table_index]);
		p_oscillator->amplitude	+= shift_amplitude;
		if(ENVELOPE_RELEASE == p_oscillator->envelope_state){
			break;
		}
		p_oscillator->release_reference_amplitude = p_oscillator->amplitude;
	} while(0);
}

/**********************************************************************************/

void perform_percussion(oscillator_t * const p_oscillator)
{
	do {
		if(true == is_stereo()
				&& false == is_processing_left_channel()){
			break;
		}

		if(false == (MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0 == p_oscillator->voice ||
				MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1 == p_oscillator->voice)){
			break;
		}
		percussion_t const * const p_percussion = get_percussion_pointer_from_index(p_oscillator->note);

		p_oscillator->percussion_duration_sample_count += 1;
		int8_t waveform_index = p_oscillator->percussion_waveform_index;
		if (p_percussion->waveform_duration_sample_number[waveform_index]
				== p_oscillator->percussion_duration_sample_count){
			p_oscillator->percussion_duration_sample_count = 0;
			p_oscillator->percussion_waveform_index += 1;
			if(MAX_WVEFORM_CHANGE_NUMBER == p_oscillator->percussion_waveform_index){
				p_oscillator->percussion_waveform_index = MAX_WVEFORM_CHANGE_NUMBER - 1;
			}
		}
		p_oscillator->current_phase +=
				(uint16_t)((((uint32_t)p_percussion->max_delta_modulation_phase)
				* p_percussion->p_modulation_envelope_table[p_oscillator->percussion_table_index]) >> 7);

		p_oscillator->percussion_same_index_count += 1;
		if(p_percussion->envelope_same_index_number > p_oscillator->percussion_same_index_count){
			break;
		}
		p_oscillator->percussion_same_index_count = 0;
		p_oscillator->percussion_table_index += 1;
		p_oscillator->amplitude
				= (int16_t)(((uint32_t)p_oscillator->loudness
							 * p_percussion->p_modulation_envelope_table[p_oscillator->percussion_table_index]) >> 7);

		if(CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH == p_oscillator->percussion_table_index){
			SET_DEACTIVATED(p_oscillator->state_bits);
			p_oscillator->percussion_table_index = CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH - 1;
			break;
		}

	}while(0);

}

/**********************************************************************************/

static inline int16_t obtain_sine_wave(uint16_t phase)
{
#define DIVIDE_BY_UINT16_MAX_PLUS_ONE(VALUE)		(((int32_t)(VALUE)) >> 16)
	return s_sine_table[DIVIDE_BY_UINT16_MAX_PLUS_ONE(phase * SINE_TABLE_LENGTH)];
}

/**********************************************************************************/

static uint16_t s_noise_random_seed = 1;

static uint16_t obtain_noise_random(void)
{
	uint8_t feedback;
	feedback=((s_noise_random_seed>>13)&1)^((s_noise_random_seed>>14)&1);
	s_noise_random_seed = (s_noise_random_seed<<1)+feedback;
	s_noise_random_seed &= 0x7fff;
	return s_noise_random_seed;
}


/**********************************************************************************/
#define SINE_WAVE(PHASE)							(obtain_sine_wave(PHASE))

#define INT16_MAX_PLUS_1							(INT16_MAX + 1)
#define MULTIPLY_BY_2(VALUE)						((VALUE) << 1)

int32_t generate_mono_wave_amplitude(oscillator_t * const p_oscillator)
{
	channel_controller_t const *p_channel_controller
			= get_channel_controller_pointer_from_index(p_oscillator->voice);
	int16_t wave = 0;
	int8_t waveform = p_channel_controller->waveform;
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0 == p_oscillator->voice ||
			MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1 == p_oscillator->voice){
		percussion_t const * const p_percussion = get_percussion_pointer_from_index(p_oscillator->note);
		waveform = p_percussion->waveform[p_oscillator->percussion_waveform_index];
	}

	switch(waveform)
	{
	case WAVEFORM_SQUARE:
		wave = (p_oscillator->current_phase > p_channel_controller->duty_cycle_critical_phase)
				? -INT16_MAX_PLUS_1 : INT16_MAX;
		break;
	case WAVEFORM_TRIANGLE:
		do {
			if(p_oscillator->current_phase < INT16_MAX_PLUS_1){
				wave = -INT16_MAX_PLUS_1 + MULTIPLY_BY_2(p_oscillator->current_phase);
				break;
			}
			wave = INT16_MAX - MULTIPLY_BY_2(p_oscillator->current_phase - INT16_MAX_PLUS_1);
		} while(0);
		break;
	case WAVEFORM_SAW:
		wave = -INT16_MAX_PLUS_1 + p_oscillator->current_phase;
		break;
	case WAVEFORM_SINE:
		wave = SINE_WAVE(p_oscillator->current_phase);
		break;
	case WAVEFORM_NOISE:
		wave = (int16_t)obtain_noise_random();
		break;
	default:
		wave = 0;
		break;
	}

	if(false == is_stereo()
			|| false == is_processing_left_channel()){
		p_oscillator->current_phase += p_oscillator->delta_phase;
	}

	return wave * p_oscillator->amplitude;
}

/**********************************************************************************/
#define DIVIDE_BY_128(VALUE)						((VALUE) >> 7)
#define CHANNEL_WAVE_AMPLITUDE(MONO_WAVE_AMPLITUDE, CHANNEL_PANNING_WEIGHT) \
													MULTIPLY_BY_2( \
														DIVIDE_BY_128((int64_t)(MONO_WAVE_AMPLITUDE) * (CHANNEL_PANNING_WEIGHT)) \
													)

int32_t generate_channel_wave_amplitude(oscillator_t * const p_oscillator,
				   int32_t const mono_wave_amplitude)
{
	int32_t channel_wave_amplitude = mono_wave_amplitude;
	do{
		if(false == is_stereo()){
			break;
		}

		channel_controller_t * const p_channel_controller
				= get_channel_controller_pointer_from_index(p_oscillator->voice);
		int8_t channel_panning_weight = p_channel_controller->pan;
		channel_panning_weight += !channel_panning_weight;//if zero, as 1
		if(true == is_processing_left_channel()){
			channel_panning_weight = 2 * MIDI_CC_CENTER_VALUE - channel_panning_weight;
		}
		channel_wave_amplitude = CHANNEL_WAVE_AMPLITUDE(mono_wave_amplitude, channel_panning_weight);
	} while(0);

	return channel_wave_amplitude;
}

/**********************************************************************************/

#ifdef _RIGHT_SHIFT_FOR_NORMALIZING_AMPLITUDE
#define NORMALIZE_WAVE_AMPLITUDE(WAVE_AMPLITUDE)		((int32_t)((WAVE_AMPLITUDE) >> g_loudness_nomalization_right_shift))
#else
#define NORMALIZE_WAVE_AMPLITUDE(WAVE_AMPLITUDE)		((int32_t)((WAVE_AMPLITUDE)/(int32_t)g_max_loudness))
#endif

int16_t chiptune_fetch_16bit_wave(void)
{
	if(-1 == process_timely_midi_message_and_event()){
		s_is_tune_ending = true;
	}

	int64_t accumulated_wave_amplitude = 0;
	int16_t oscillator_index = get_event_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();
	for(int16_t k = 0; k < occupied_oscillator_number; k++){
		oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
		do {
			if(false == IS_ACTIVATED(p_oscillator->state_bits)){
				break;
			}

			perform_vibrato(p_oscillator);
			perform_melody_envelope(p_oscillator);
			perform_percussion(p_oscillator);

			int32_t mono_wave_amplitude = generate_mono_wave_amplitude(p_oscillator);
			int32_t channel_wave_amplitude
					= generate_channel_wave_amplitude(p_oscillator, mono_wave_amplitude);
			accumulated_wave_amplitude += (int32_t)channel_wave_amplitude;
		} while(0);
		oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
	}

	int32_t out_wave = NORMALIZE_WAVE_AMPLITUDE(accumulated_wave_amplitude);
	do {
		if(INT16_MAX < out_wave){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: out_value = %d, greater than UINT8_MAX\r\n",
							out_wave);
			break;
		}

		if(-INT16_MAX_PLUS_1 > out_wave){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: out_value = %d, less than 0\r\n",
							out_wave);
			break;
		}
	}while(0);

	if(false == is_stereo()
			|| false == is_processing_left_channel()){
		INCREMENT_BASE_TIME();
	}

	if(true == is_stereo()){
		swap_processing_channel();
	}
	return (int16_t)out_wave;
}

/**********************************************************************************/

#define INT8_MAX_PLUS_1								(INT8_MAX + 1)
#define REDUCE_INT16_PRECISION_TO_INT8(VALUE)		((VALUE) >> 8)

uint8_t chiptune_fetch_8bit_wave(void)
{
	return (uint8_t)(REDUCE_INT16_PRECISION_TO_INT8(chiptune_fetch_16bit_wave()) + INT8_MAX_PLUS_1);
}

/**********************************************************************************/

bool chiptune_is_tune_ending(void)
{
	return s_is_tune_ending;
}
