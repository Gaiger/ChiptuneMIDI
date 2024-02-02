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

#define CORRECT_TIME_BASE()							\
													do { \
														s_current_sample_index = (uint32_t)(s_current_sample_index * s_tempo/tempo); \
													} while(0)

#define UPDATE_TIME_BASE_UNIT()						UPDATE_SAMPLES_TO_TICK_RATIO()

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

#define CORRECT_TIME_BASE()							\
													do { \
														(void)0; \
													} while(0)

#define UPDATE_TIME_BASE_UNIT()						UPDATE_DELTA_TICK_PER_SAMPLE()

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

#define DEMPER_PEDAL_ATTENUATION_TIME_IN_SECOND						(4.0)
static uint32_t s_damper_pedal_attenuation_tick	= (uint32_t)((DEMPER_PEDAL_ATTENUATION_TIME_IN_SECOND) * (DEFAULT_RESOLUTION) * (DEFAULT_TEMPO / 60.0) + 0.5);

#define UPDATE_DAMPER_PEDAL_ATTENUATION_TICK()		\
													do { \
														s_damper_pedal_attenuation_tick \
															= (uint32_t)(DEMPER_PEDAL_ATTENUATION_TIME_IN_SECOND * (s_resolution) * (s_tempo/60.0) + 0.5); \
													} while(0)

static int(*s_handler_get_midi_message)(uint32_t index, uint32_t * const p_tick, uint32_t * const p_message) = NULL;

static bool s_is_tune_ending = false;

/**********************************************************************************/

void chiptune_set_midi_message_callback( int(*handler_get_midi_message)(uint32_t index, uint32_t * const p_tick, uint32_t * const p_message) )
{
	s_handler_get_midi_message = handler_get_midi_message;
}

/**********************************************************************************/

uint32_t get_sampling_rate(void) { return s_sampling_rate; }

/**********************************************************************************/

uint32_t get_resolution(void) { return s_resolution; }

/**********************************************************************************/

float get_tempo(void) { return s_tempo; }

/**********************************************************************************/

static void process_program_change_message(uint32_t const tick, int8_t const voice, uint8_t const number)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_PROGRAM_CHANGE :: ", tick);
#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0		(9)
#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1		(10)
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	do
	{
		if(false == (MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0 == voice || MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1 == voice)){
			break;
		}
		p_channel_controller->waveform = WAVEFORM_NOISE;
	}while(0);

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
}

/**********************************************************************************/

#define DIVIDE_BY_2(VALUE)							((VALUE) >> 1)

static uint16_t calculate_delta_phase(int16_t const note, int8_t tuning_in_semitones,
									  int8_t const pitch_wheel_bend_range_in_semitones, int16_t const pitch_wheel,
									  float pitch_chorus_bend_in_semitones, float *p_pitch_wheel_bend_in_semitone)
{
	// TO DO : too many float variable
	float pitch_wheel_bend_in_semitone = ((pitch_wheel - MIDI_PITCH_WHEEL_CENTER)/(float)MIDI_PITCH_WHEEL_CENTER) * DIVIDE_BY_2(pitch_wheel_bend_range_in_semitones);
	float corrected_note = (float)(note + (int16_t)tuning_in_semitones) + pitch_wheel_bend_in_semitone + pitch_chorus_bend_in_semitones;
	/*
	 * freq = 440 * 2**((note - 69)/12)
	*/
	float frequency = 440.0f * powf(2.0f, (corrected_note - 69.0f)/12.0f);
	frequency = roundf(frequency * 100.0f + 0.5f)/100.0f;
	/*
	 * sampling_rate/frequency = samples_per_cycle  = (UINT16_MAX + 1)/phase
	*/
	uint16_t delta_phase = (uint16_t)((UINT16_MAX + 1) * frequency / s_sampling_rate);
	*p_pitch_wheel_bend_in_semitone = pitch_wheel_bend_in_semitone;
	return delta_phase;
}

/**********************************************************************************/

//xor-shift pesudo random https://en.wikipedia.org/wiki/Xorshift
static uint32_t s_chorus_random_seed = 20240129;

static uint16_t chorus_ramdom(void)
{
	s_chorus_random_seed ^= s_chorus_random_seed << 13;
	s_chorus_random_seed ^= s_chorus_random_seed >> 17;
	s_chorus_random_seed ^= s_chorus_random_seed << 5;
	return (uint16_t)(s_chorus_random_seed);
}

/**********************************************************************************/

#define RAMDON_RANGE_TO_PLUS_MINUS_ONE(VALUE)	\
												(((DIVIDE_BY_2(UINT16_MAX) + 1) - (VALUE))/(float)(DIVIDE_BY_2(UINT16_MAX) + 1))

static float pitch_chorus_bend_in_semitone(int8_t const voice)
{
	channel_controller_t *p_channel_controller =  get_channel_controller_pointer_from_index(voice);
	int8_t const chorus = p_channel_controller->chorus;
	if(0 == chorus){
		return 0.0;
	}
	float const max_pitch_chorus_bend_in_semitones = p_channel_controller->max_pitch_chorus_bend_in_semitones;

	uint16_t random = chorus_ramdom();
	float pitch_chorus_bend_in_semitone;
	pitch_chorus_bend_in_semitone = RAMDON_RANGE_TO_PLUS_MINUS_ONE(random) *  chorus/(float)INT8_MAX;
	pitch_chorus_bend_in_semitone *= max_pitch_chorus_bend_in_semitones;
	//CHIPTUNE_PRINTF(cDeveloping, "pitch_chorus_bend_in_semitone = %3.2f\r\n", pitch_chorus_bend_in_semitone);
	return pitch_chorus_bend_in_semitone;
}

/**********************************************************************************/

#define DIVIDE_BY_8(VALUE)							((VALUE) >> 3)
#define REDUCE_LOUNDNESS_AS_DAMPING_PEDAL_ON_BUT_NOTE_OFF(VALUE)	\
													DIVIDE_BY_8(VALUE)

#define DIVIDE_BY_16(VALUE)							((VALUE) >> 4)
#define OSCILLATOR_NUMBER_FOR_CHORUS(VALUE)			(DIVIDE_BY_16(((VALUE) + 15)))

int process_chorus_effect(uint32_t const tick, bool const is_note_on,
						  int8_t const voice, int8_t const note, int8_t const velocity,
						  int16_t const native_oscillator_index)
{
	(void)velocity;
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	if(0 >= p_channel_controller->chorus){
		return 1;
	}
	oscillator_t  * const p_native_oscillator = get_oscillator_pointer_from_index(native_oscillator_index);
#define ASSOCIATE_CHORUS_OSCILLATOR_NUMBER			(4 - 1)
	int oscillator_indexes[ASSOCIATE_CHORUS_OSCILLATOR_NUMBER] = {UNUSED_OSCILLATOR, UNUSED_OSCILLATOR, UNUSED_OSCILLATOR};

	do {
		if(true == is_note_on){
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
			int16_t i;
			for(int16_t j = 0; j < ASSOCIATE_CHORUS_OSCILLATOR_NUMBER;j++){
				oscillator_t * const p_oscillator = acquire_oscillator(&i);
				if(NULL == p_oscillator){
					return -1;
				}
				int8_t const vibrato_modulation_in_semitone = p_channel_controller->vibrato_modulation_in_semitone;
				memcpy(p_oscillator, p_native_oscillator, sizeof(oscillator_t));
				p_oscillator->loudness = oscillator_loudness;
				p_oscillator->pitch_chorus_bend_in_semitone = pitch_chorus_bend_in_semitone(voice);
				p_oscillator->delta_phase = calculate_delta_phase(p_oscillator->note, p_channel_controller->tuning_in_semitones,
																  p_channel_controller->pitch_wheel_bend_range_in_semitones,
																  p_channel_controller->pitch_wheel,
																  p_oscillator->pitch_chorus_bend_in_semitone,
																  &pitch_wheel_bend_in_semitone);
				p_oscillator->delta_vibrato_phase = calculate_delta_phase(p_oscillator->note + vibrato_modulation_in_semitone,
																			p_channel_controller->tuning_in_semitones,
																			p_channel_controller->pitch_wheel_bend_range_in_semitones,
																			p_channel_controller->pitch_wheel,
																			p_oscillator->pitch_chorus_bend_in_semitone,
																			&pitch_wheel_bend_in_semitone) - p_oscillator->delta_phase;
				p_oscillator->native_oscillator = native_oscillator_index;
				SET_CHORUS_OSCILLATOR(p_oscillator->state_bits);
				oscillator_indexes[j] = i;
			}
			break;
		}

		int16_t kk = 0;
		int16_t ii = 0;
		int16_t oscillator_index = get_head_occupied_oscillator_index();
		int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
		for(ii = 0; ii < occupied_oscillator_number; ii++){
			oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
			do {
				if(true != IS_CHORUS_OSCILLATOR(p_oscillator->state_bits)){
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
				//if(ENVELOPE_RELEASE == p_oscillator->envelope_state){
				//	break;
				//	}
				oscillator_indexes[kk] = oscillator_index;
				kk += 1;
			} while(0);

			if(ASSOCIATE_CHORUS_OSCILLATOR_NUMBER == kk){
				break;
			}
			oscillator_index = get_next_occupied_oscillator_index(oscillator_index);
		}

		if(occupied_oscillator_number == ii){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR::targeted oscillator could not be found\r\n");
			return -2;
		}
	} while(0);

	int8_t event_type = (true == is_note_on) ? EVENT_ACTIVATE : EVENT_RELEASE;
	for(int16_t j = 0; j  < ASSOCIATE_CHORUS_OSCILLATOR_NUMBER; j++){
		put_event(event_type, oscillator_indexes[j], tick + (j + 1) * s_chorus_delta_tick);
	}

	return 0;
}

/**********************************************************************************/

static int process_note_message(uint32_t const tick, bool const is_note_on,
						 int8_t const voice, int8_t const note, int8_t const velocity)
{
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	float pitch_wheel_bend_in_semitone = 0.0f;
	int8_t actual_velocity = velocity;
	int16_t ii = 0;
	do {
		if(true == is_note_on){
#if(0)
			int16_t oscillator_index = get_head_occupied_oscillator_index();
			int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
			for(ii = 0; ii < occupied_oscillator_number; ii++){
				oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
				do {
					if(note != p_oscillator->note){
						break;
					}
					if(voice != p_oscillator->voice){
						break;
					}

					do {
						if(UNUSED_OSCILLATOR != p_oscillator->native_oscillator){
							break;
						}
						if(true == IS_NOTE_ON(p_oscillator->state_bits)){
							break;
						}
						if(ENVELOPE_RELEASE == p_oscillator->envelope_state){
							break;
						}
						put_event(EVENT_RELEASE, oscillator_index, tick);
						process_chorus_effect(tick, false, voice, note, velocity, oscillator_index);
					} while(0);

					// TODO : the associate chorus oscillators loudness should be decreased
					actual_velocity -= p_oscillator->loudness/p_channel_controller->playing_volume;
					if(0 > actual_velocity){
						actual_velocity = DIVIDE_BY_2(velocity);
						p_oscillator->loudness = (actual_velocity + (actual_velocity & 0x01))
								* p_channel_controller->playing_volume;
					}
				} while(0);
				oscillator_index = get_next_occupied_oscillator_index(oscillator_index);
			}
#endif
			oscillator_t * const p_oscillator = acquire_oscillator(&ii);
			if(NULL == p_oscillator){
				return -1;
			}
			RESET_STATE_BITES(p_oscillator->state_bits);
			SET_NOTE_ON(p_oscillator->state_bits);
			p_oscillator->voice = voice;
			p_oscillator->note = note;
			p_oscillator->pitch_chorus_bend_in_semitone = 0;
			p_oscillator->delta_phase = calculate_delta_phase(p_oscillator->note, p_channel_controller->tuning_in_semitones,
															  p_channel_controller->pitch_wheel_bend_range_in_semitones,
															  p_channel_controller->pitch_wheel,
															  p_oscillator->pitch_chorus_bend_in_semitone,
															  &pitch_wheel_bend_in_semitone);
			p_oscillator->current_phase = 0;
			p_oscillator->loudness = (uint16_t)actual_velocity * (uint16_t)p_channel_controller->playing_volume;

			p_oscillator->delta_vibrato_phase = calculate_delta_phase(p_oscillator->note + p_channel_controller->vibrato_modulation_in_semitone,
																		p_channel_controller->tuning_in_semitones,
																		p_channel_controller->pitch_wheel_bend_range_in_semitones,
																		p_channel_controller->pitch_wheel,
																		p_oscillator->pitch_chorus_bend_in_semitone,
																		&pitch_wheel_bend_in_semitone) - p_oscillator->delta_phase;
			p_oscillator->vibrato_table_index = 0;
			p_oscillator->vibrato_same_index_count = 0;

			p_oscillator->amplitude = 0;
			p_oscillator->envelope_same_index_count = 0;
			p_oscillator->envelope_table_index = 0;
			p_oscillator->release_reference_amplitude = 0;

			p_oscillator->native_oscillator = UNUSED_OSCILLATOR;
			put_event(EVENT_ACTIVATE, ii, tick);
			process_chorus_effect(tick, is_note_on, voice, note, velocity, ii);
			break;
		}

		bool is_found = false;
		int16_t oscillator_index = get_head_occupied_oscillator_index();
		int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
		for(ii = 0; ii < occupied_oscillator_number; ii++){
			oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
			bool is_leave_loop = false;
			do {
				if(note != p_oscillator->note){
					break;
				}
				if(voice != p_oscillator->voice){
					break;
				}
				do {
					if(true == p_channel_controller->is_damper_pedal_on){
						SET_NOTE_OFF(p_oscillator->state_bits);
						p_oscillator->loudness
								= REDUCE_LOUNDNESS_AS_DAMPING_PEDAL_ON_BUT_NOTE_OFF(p_oscillator->loudness);
						is_found = true;
						break;
					}
					if(UNUSED_OSCILLATOR != p_oscillator->native_oscillator){
						break;
					}
					if(ENVELOPE_RELEASE == p_oscillator->envelope_state){
						break;
					}
					put_event(EVENT_RELEASE, oscillator_index, tick);
					process_chorus_effect(tick, is_note_on, voice, note, velocity, oscillator_index);
					is_found = true;
					is_leave_loop = true;
				} while(0);
			} while(0);
			if(true == is_leave_loop){
				break;
			}
			oscillator_index = get_next_occupied_oscillator_index(oscillator_index);
		}

		if(false == is_found){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR::no corresponding note for off :: tick = %u, voice = %d,  note = %u\r\n",
							tick, voice, note);
			return -2;
		}
	} while(0);

	char pitch_wheel_bend_string[32] = "";
	if(true == is_note_on && 0.0f != pitch_wheel_bend_in_semitone){
		snprintf(&pitch_wheel_bend_string[0], sizeof(pitch_wheel_bend_string),
			", pitch wheel bend = %+3.2f", pitch_wheel_bend_in_semitone);
	}

	CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %d, note = %d, velocity = %d\r\n",
					tick, is_note_on ? "MIDI_MESSAGE_NOTE_ON" : "MIDI_MESSAGE_NOTE_OFF" ,
					voice, note, velocity, velocity, &pitch_wheel_bend_string[0]);

#ifdef _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING
	#ifdef _INCREMENTAL_SAMPLE_INDEX
	if(TICK_TO_SAMPLE_INDEX(818880) < s_current_sample_index){
		CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %d, note = %d, velocity = %d\r\n",
						tick, is_note_on ? "MIDI_MESSAGE_NOTE_ON" : "MIDI_MESSAGE_NOTE_OFF" , voice, note, velocity);
	#else
	if(818880 < s_current_tick){
		CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %d, note = %d, velocity = %d\r\n",
						tick, is_note_on ? "MIDI_MESSAGE_NOTE_ON" : "MIDI_MESSAGE_NOTE_OFF" , voice, note, velocity);
	}
	#endif
#endif

	return 0;
}

/**********************************************************************************/

static void process_pitch_wheel_message(uint32_t const tick, int8_t const voice, int16_t const value)
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
	CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, MIDI_MESSAGE_PITCH_WHEEL :: voice = %d, value = 0x%04x %s\r\n",
					tick, voice, value, &delta_hex_string[0]);

	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	p_channel_controller->pitch_wheel = value;

	int16_t oscillator_index = get_head_occupied_oscillator_index();
	int16_t const occupied_oscillator_number = get_occupied_oscillator_number();

	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
		do {
			if(voice != p_oscillator->voice){
				break;
			}
			float pitch_bend_in_semitone;
			p_oscillator->delta_phase = calculate_delta_phase(p_oscillator->note, p_channel_controller->tuning_in_semitones,
															   p_channel_controller->pitch_wheel_bend_range_in_semitones,
															   p_channel_controller->pitch_wheel, p_oscillator->pitch_chorus_bend_in_semitone,
															  &pitch_bend_in_semitone);

			CHIPTUNE_PRINTF(cNoteOperation, "---- voice = %d, note = %d, pitch bend in semitone = %+3.2f\r\n",
							voice, p_oscillator->note, pitch_bend_in_semitone);
		} while(0);
		oscillator_index = get_next_occupied_oscillator_index(oscillator_index);
	}
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

static void process_midi_message(struct _tick_message const tick_message)
{
	if(true == IS_NULL_TICK_MESSAGE(tick_message)){
		return ;
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
}

/**********************************************************************************/

uint32_t s_midi_messge_index = 0;
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

static void release_all_channels_damper_pedal(const uint32_t tick)
{
	for(int8_t k = 0; k < MIDI_MAX_CHANNEL_NUMBER; k++){
		do {
			channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(k);
			if(false == p_channel_controller->is_damper_pedal_on){
				break;
			}

			int16_t oscillator_index = get_head_occupied_oscillator_index();
			int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
			for(int16_t i = 0; i < occupied_oscillator_number; i++){
				oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
				if(k == p_oscillator->voice){
					put_event(EVENT_RELEASE, oscillator_index, tick);
				}
				oscillator_index = get_next_occupied_oscillator_index(oscillator_index);
			}
		}while(0);
	}
}

/**********************************************************************************/

void process_ending(const uint32_t tick)
{
	release_all_channels_damper_pedal(tick);
	process_events(tick);
}

/**********************************************************************************/

struct _tick_message s_fetched_tick_message = {NULL_TICK, NULL_MESSAGE};
uint32_t s_fetched_event_tick = NULL_TICK;

static int process_timely_midi_message(void)
{
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
			process_ending(CURRENT_TICK());
			ret = -1;
			break;
		}

		bool is_both_after_current_tick = true;

		do
		{
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
			process_ending(tick);
			break;
		}

		tick = (tick_message.tick < event_tick) ? tick_message.tick : event_tick;

		do
		{
			if(tick == previous_tick){
				break;
			}
			previous_tick = tick;

			int32_t sum_loudness = 0;

			int16_t oscillator_index = get_head_occupied_oscillator_index();
			int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
			for(int16_t i = 0; i < occupied_oscillator_number; i++){
				oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
				sum_loudness += p_oscillator->loudness;
				oscillator_index = get_next_occupied_oscillator_index(oscillator_index);
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
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: not all events are released\r\n");
	}

	if(0 != get_occupied_oscillator_number()){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: not all oscillators are released\r\n");

		int16_t oscillator_index = get_head_occupied_oscillator_index();
		int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
		for(int16_t i = 0; i < occupied_oscillator_number; i++){
			oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);

			CHIPTUNE_PRINTF(cDeveloping, "oscillator = %d, voice = %d, note = 0x%02x(%d)\r\n",
							oscillator_index, p_oscillator->voice, p_oscillator->note, p_oscillator->note);

			oscillator_index = get_next_occupied_oscillator_index(oscillator_index);
		}
	}

	return max_loudness;
}

#ifdef _RIGHT_SHIFT_FOR_NORMALIZING_LOUNDNESS

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
#define UPDATE_LOUNDNESS_NORMALIZER()				\
													do { \
														uint32_t max_loudness = get_max_simultaneous_loudness(); \
														g_loudness_nomalization_right_shift \
															= number_of_roundup_to_power2_left_shift_bits(max_loudness);\
													} while(0)
#else
int32_t g_max_loudness = 1 << 16;
#define UPDATE_LOUNDNESS_NORMALIZER()				\
													do { \
														g_max_loudness = get_max_simultaneous_loudness(); \
													} while(0)
#endif

/**********************************************************************************/

static int8_t s_vibrato_phase_table[VIBRATO_PHASE_TABLE_LENGTH] = {0};
#define CALCULATE_VIBRATO_TABLE_INDEX_REMAINDER(INDEX)		\
															((INDEX) & (VIBRATO_PHASE_TABLE_LENGTH - 1))

/**********************************************************************************/

void chiptune_initialize(uint32_t const sampling_rate, uint32_t const resolution, uint32_t const total_message_number)
{
	s_sampling_rate = sampling_rate;
	s_resolution = resolution;
	s_total_message_number = total_message_number;

	RESET_CURRENT_TIME();
	s_is_tune_ending = false;
	s_midi_messge_index = 0;
	SET_TICK_MESSAGE_NULL(s_fetched_tick_message);

	initialize_channel_controller();
	reset_all_oscillators();
	clean_all_events();

	UPDATE_TIME_BASE_UNIT();
	UPDATE_LOUNDNESS_NORMALIZER();
	process_timely_midi_message();
	return ;
}

/**********************************************************************************/

void chiptune_set_tempo(float const tempo)
{
	CHIPTUNE_PRINTF(cMidiSetup, "%s :: tempo = %3.1f\r\n", __FUNCTION__,tempo);
	CORRECT_TIME_BASE();
	s_tempo = tempo;
	UPDATE_TIME_BASE_UNIT();
	UPDATE_DAMPER_PEDAL_ATTENUATION_TICK();
	UPDATE_CHORUS_DELTA_TICK();
	update_all_channel_controllers_envelope();
}

/**********************************************************************************/

#ifdef _INCREMENTAL_SAMPLE_INDEX
#define INCREMENT_TIME_BASE()						\
													do { \
														s_current_sample_index += 1; \
													} while(0)
#else
#define INCREMENT_TIME_BASE()						\
													do { \
														s_current_tick += s_delta_tick_per_sample; \
													} while(0)
#endif

#ifdef _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING

/**********************************************************************************/

inline static void increase_time_base_for_fast_to_ending(void)
{
#ifdef _INCREMENTAL_SAMPLE_INDEX
	if(TICK_TO_SAMPLE_INDEX(818880) > s_current_sample_index){
		s_current_sample_index += 99;
	}
#else
	if(818880 > s_current_tick){
		s_current_tick += 99.0 * s_delta_tick_per_sample;
	}
#endif
}

#endif

/**********************************************************************************/

#define DIVIDE_BY_128(VALUE)						((VALUE) >> 7)
#define NORMALIZE_VIBRTO_DELTA_PHASE(VALUE)			\
													DIVIDE_BY_128(DIVIDE_BY_128(VALUE))
#define REGULATE_MODULATION_WHEEL(VALUE)			((VALUE) + 1)

void perform_vibrato(oscillator_t * const p_oscillator)
{
	do {
		channel_controller_t *p_channel_controller = get_channel_controller_pointer_from_index(p_oscillator->voice);
		int8_t const modulation_wheel = p_channel_controller->modulation_wheel;
		if(0 >= modulation_wheel){
			break;
		}
		uint16_t const vibrato_same_index_number = p_channel_controller->vibrato_same_index_number;
		uint16_t delta_vibrato_phase = p_oscillator->delta_vibrato_phase;
		uint32_t vibrato_modulation
				= REGULATE_MODULATION_WHEEL(modulation_wheel)
					* s_vibrato_phase_table[p_oscillator->vibrato_table_index];

		delta_vibrato_phase = NORMALIZE_VIBRTO_DELTA_PHASE(vibrato_modulation * delta_vibrato_phase);
		p_oscillator->current_phase += delta_vibrato_phase;

		p_oscillator->vibrato_same_index_count += 1;
		if(vibrato_same_index_number == p_oscillator->vibrato_same_index_count){
			p_oscillator->vibrato_same_index_count = 0;
			p_oscillator->vibrato_table_index = CALCULATE_VIBRATO_TABLE_INDEX_REMAINDER(p_oscillator->vibrato_table_index  + 1);
		}
	} while(0);
}

/**********************************************************************************/

void perform_envelope(oscillator_t * const p_oscillator)
{
	do
	{
		if(ENVELOPE_SUSTAIN == p_oscillator->envelope_state){
			break;
		}

		if(ENVELOPE_TABLE_LENGTH == p_oscillator->envelope_table_index){
			break;
		}

		channel_controller_t *p_channel_controller = get_channel_controller_pointer_from_index(p_oscillator->voice);
		uint16_t envelope_same_index_number = 0;
		switch(p_oscillator->envelope_state)
		{
		case ENVELOPE_ATTACK:
			envelope_same_index_number = p_channel_controller->envelope_attack_same_index_number;
			break;
		case ENVELOPE_DECAY:
			envelope_same_index_number = p_channel_controller->envelope_decay_same_index_number;
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
		if(ENVELOPE_TABLE_LENGTH == p_oscillator->envelope_table_index){
			do {
				if(ENVELOPE_RELEASE == p_oscillator->envelope_state){
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
				}
				p_oscillator->release_reference_amplitude = p_oscillator->amplitude;
			} while(0);
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
			shift_amplitude = 0;
			break;
		case ENVELOPE_DECAY: {
			p_envelope_table = p_channel_controller->p_envelope_decay_table;
			int16_t sustain_ampitude = SUSTAIN_AMPLITUDE(p_oscillator->loudness,
														 p_channel_controller->envelope_sustain_level);
			delta_amplitude = p_oscillator->loudness - sustain_ampitude;
			shift_amplitude = sustain_ampitude;
		}	break;
		case ENVELOPE_RELEASE: {
			p_envelope_table = p_channel_controller->p_envelope_release_table;
			delta_amplitude = p_oscillator->release_reference_amplitude;
			} break;
		}

		p_oscillator->amplitude = REDUCE_AMPLITUDE_BY_ENVELOPE_TABLE_VALUE(delta_amplitude,
																		   p_envelope_table[p_oscillator->envelope_table_index]);
		p_oscillator->amplitude	+= shift_amplitude;
		if(ENVELOPE_RELEASE == p_oscillator->envelope_state){
			break;
		}
		p_oscillator->release_reference_amplitude = p_oscillator->amplitude;
	} while(0);
}

/**********************************************************************************/

#ifdef _RIGHT_SHIFT_FOR_NORMALIZING_LOUNDNESS
#define NORMALIZE_LOUNDNESS(VALUE)					((int32_t)((VALUE) >> g_loudness_nomalization_right_shift))
#else
#define NORMALIZE_LOUNDNESS(VALUE)					((int32_t)((VALUE)/(int32_t)g_max_loudness))
#endif

#define INT16_MAX_PLUS_1							(INT16_MAX + 1)
#define MULTIPLY_BY_2(VALUE)						((VALUE) << 1)

int16_t chiptune_fetch_16bit_wave(void)
{
	if(-1 == process_timely_midi_message()){
		if(0 == get_occupied_oscillator_number()){
			s_is_tune_ending = true;
		}
	}

	int64_t accumulated_value = 0;
	int16_t oscillator_index = get_head_occupied_oscillator_index();
	int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
	for(int16_t k = 0; k < occupied_oscillator_number; k++){
		oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
		if(false == IS_ACTIVATED(p_oscillator->state_bits)){
			goto Flag_oscillator_take_effect_end;
		}

		int16_t value = 0;
		perform_vibrato(p_oscillator);
		perform_envelope(p_oscillator);
		channel_controller_t *p_channel_controller = get_channel_controller_pointer_from_index(p_oscillator->voice);
		switch(p_channel_controller->waveform)
		{
		case WAVEFORM_SQUARE:
			value = (p_oscillator->current_phase > p_channel_controller->duty_cycle_critical_phase) ? -INT16_MAX_PLUS_1 : INT16_MAX;
			break;
		case WAVEFORM_TRIANGLE:
			do {
				if(p_oscillator->current_phase < INT16_MAX_PLUS_1){
					value = -INT16_MAX_PLUS_1 + MULTIPLY_BY_2(p_oscillator->current_phase);
					break;
				}
				value = INT16_MAX - MULTIPLY_BY_2(p_oscillator->current_phase - INT16_MAX_PLUS_1);
			} while(0);
			break;
		case WAVEFORM_SAW:
			value =  -INT16_MAX_PLUS_1 + p_oscillator->current_phase;
			break;
		case WAVEFORM_NOISE:
			break;
		default:
			break;
		}
		accumulated_value += (value * p_oscillator->amplitude);

		p_oscillator->current_phase += p_oscillator->delta_phase;
Flag_oscillator_take_effect_end:
		oscillator_index = get_next_occupied_oscillator_index(oscillator_index);
	}

	INCREMENT_TIME_BASE();
#ifdef _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING
	increase_time_base_for_fast_to_ending();
#endif

	int32_t out_value = NORMALIZE_LOUNDNESS(accumulated_value);
	do {
		if(INT16_MAX < out_value){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: out_value = %d, greater than UINT8_MAX\r\n",
							out_value);
			break;
		}

		if(-INT16_MAX_PLUS_1 > out_value){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: out_value = %d, less than 0\r\n",
							out_value);
			break;
		}
	}while(0);

	return (int16_t)out_value;
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
