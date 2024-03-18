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
#include "chiptune_midi_effect_internal.h"

#include "chiptune.h"

#ifdef _INCREMENTAL_SAMPLE_INDEX
typedef float chiptune_float;
#else
typedef double chiptune_float;
#endif

#define DEFAULT_SAMPLING_RATE						(16000)

static float s_tempo = MIDI_DEFAULT_TEMPO;
static uint32_t s_sampling_rate = DEFAULT_SAMPLING_RATE;
static uint32_t s_resolution = MIDI_DEFAULT_RESOLUTION;
static bool s_is_stereo = false;
bool s_is_processing_left_channel = true;

#ifdef _INCREMENTAL_SAMPLE_INDEX
static uint32_t s_current_sample_index = 0;
static chiptune_float s_tick_to_sample_index_ratio = (chiptune_float)(DEFAULT_SAMPLING_RATE * 1.0/(MIDI_DEFAULT_TEMPO/60.0)/MIDI_DEFAULT_RESOLUTION);

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
static chiptune_float s_delta_tick_per_sample = (MIDI_DEFAULT_RESOLUTION / ( (chiptune_float)DEFAULT_SAMPLING_RATE/(MIDI_DEFAULT_TEMPO /60.0) ) );

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

#define SINE_TABLE_LENGTH							(2048)
static int16_t s_sine_table[SINE_TABLE_LENGTH]		= {0};

/**********************************************************************************/

static int(*s_handler_get_midi_message)(uint32_t const index, uint32_t * const p_tick, uint32_t * const p_message) = NULL;

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
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
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

int setup_pitch_oscillator(uint32_t const tick, int8_t const voice, int8_t const note, int8_t const velocity,
									oscillator_t * const p_oscillator)
{
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
		return 1;
	}
	(void)tick;
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	float pitch_wheel_bend_in_semitones = 0.0f;

	p_oscillator->amplitude = 0;

	p_oscillator->pitch_chorus_bend_in_semitones = 0;
	p_oscillator->delta_phase
			= calculate_oscillator_delta_phase(voice, p_oscillator->note,
											   p_oscillator->pitch_chorus_bend_in_semitones,
											   &pitch_wheel_bend_in_semitones);

	p_oscillator->max_delta_vibrato_phase
			= calculate_oscillator_delta_phase(voice,
				p_oscillator->note + p_channel_controller->vibrato_modulation_in_semitones,
				p_oscillator->pitch_chorus_bend_in_semitones, &pitch_wheel_bend_in_semitones)
			- p_oscillator->delta_phase;

	p_oscillator->vibrato_table_index = 0;
	p_oscillator->vibrato_same_index_count = 0;

	p_oscillator->envelope_same_index_count = 0;
	p_oscillator->envelope_table_index = 0;
	p_oscillator->release_reference_amplitude = 0;

	char pitch_wheel_bend_string[32] = "";
	if(0.0f != pitch_wheel_bend_in_semitones){
		snprintf(&pitch_wheel_bend_string[0], sizeof(pitch_wheel_bend_string),
				", pitch wheel bend in semitone = %+3.2f", pitch_wheel_bend_in_semitones);
	}

	CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %d, note = %d, velocity = %d%s\r\n",
					tick,  "MIDI_MESSAGE_NOTE_ON",
					voice, note, velocity, &pitch_wheel_bend_string[0]);
	return 0;
}

/**********************************************************************************/

int setup_percussion_oscillator(uint32_t const tick, int8_t const voice, int8_t const note, int8_t const velocity,
									oscillator_t * const p_oscillator)
{
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL != voice){
		return 1;
	}
	(void)tick;

	percussion_t const * const p_percussion = get_percussion_pointer_from_index(note);
	p_oscillator->amplitude = PERCUSSION_ENVELOPE(p_oscillator->loudness,
					p_percussion->p_amplitude_envelope_table[p_oscillator->percussion_table_index]);

	p_oscillator->percussion_waveform_index = 0;
	p_oscillator->percussion_duration_sample_count = 0;
	p_oscillator->percussion_same_index_count = 0;
	p_oscillator->percussion_table_index = 0;
	p_oscillator->delta_phase = p_percussion->delta_phase;

	char not_implemented_string[24] = {0};
	if(false == p_percussion->is_implemented){
		snprintf(&not_implemented_string[0], sizeof(not_implemented_string), "%s", "(NOT IMPLEMENTED)");
	}
	CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %d, %s%s, velocity = %d\r\n",
					tick,  "MIDI_MESSAGE_NOTE_ON",
					voice, get_percussion_name_string(note), not_implemented_string, velocity);
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
			if(false == IS_NATIVE_OSCILLATOR(p_oscillator->state_bits)){
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
			process_reverb_effect(tick, EVENT_REST, voice, note, velocity, oscillator_index);
		} while(0);
		oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
	}
}

/**********************************************************************************/

static int process_note_message(uint32_t const tick, bool const is_note_on,
						 int8_t const voice, int8_t const note, int8_t const velocity)
{
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
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
			p_oscillator->loudness = (int16_t)(
						(velocity * p_channel_controller->expression * p_channel_controller->volume)/INT8_MAX);
			memset(&p_oscillator->chorus_asscociate_oscillators[0], UNOCCUPIED_OSCILLATOR, 3 * sizeof(int16_t));
			memset(&p_oscillator->reverb_asscociate_oscillators[0], UNOCCUPIED_OSCILLATOR, 3 * sizeof(int16_t));

			p_oscillator->current_phase = 0;
			do
			{
				if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
					setup_percussion_oscillator(tick, voice, note, velocity, p_oscillator);
					break;
				}

				setup_pitch_oscillator(tick, voice, note, velocity, p_oscillator);
			}while(0);

			put_event(EVENT_ACTIVATE, oscillator_index, tick);
			process_reverb_effect(tick, EVENT_ACTIVATE, voice, note, velocity, oscillator_index);
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

				if(false == IS_NATIVE_OSCILLATOR(p_oscillator->state_bits)){
					break;
				}
				if(false == IS_NOTE_ON(p_oscillator->state_bits)){
					break;
				}
				if(true == IS_FREEING(p_oscillator->state_bits)){
					break;
				}
				put_event(EVENT_FREE, oscillator_index, tick);
				process_reverb_effect(tick, EVENT_FREE, voice, note, velocity, oscillator_index);
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

		do
		{
			if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
				break;
			}
			if(false == p_channel_controller->is_damper_pedal_on){
				break;
			}
			int16_t const original_oscillator_index = oscillator_index;
			int16_t reduced_loundness_oscillator_index;
			oscillator_t * const p_oscillator = acquire_event_freed_oscillator(&reduced_loundness_oscillator_index);
			memcpy(p_oscillator, get_event_oscillator_pointer_from_index(original_oscillator_index),
				   sizeof(oscillator_t));
			RESET_STATE_BITES(p_oscillator->state_bits);
			SET_NOTE_OFF(p_oscillator->state_bits);
			memset(&p_oscillator->chorus_asscociate_oscillators[0], UNOCCUPIED_OSCILLATOR, 3 * sizeof(int16_t));
			memset(&p_oscillator->reverb_asscociate_oscillators[0], UNOCCUPIED_OSCILLATOR, 3 * sizeof(int16_t));
			p_oscillator->loudness =
					LOUNDNESS_AS_DAMPING_PEDAL_ON_BUT_NOTE_OFF(p_oscillator->loudness,
															   p_channel_controller->damper_on_but_note_off_loudness_level);
			p_oscillator->amplitude = 0;
			p_oscillator->envelope_same_index_count = 0;
			p_oscillator->envelope_table_index = 0;
			p_oscillator->release_reference_amplitude = 0;
			put_event(EVENT_ACTIVATE, reduced_loundness_oscillator_index, tick);
			process_reverb_effect(tick, EVENT_ACTIVATE, voice, note, velocity, reduced_loundness_oscillator_index);
			process_chorus_effect(tick, EVENT_ACTIVATE, voice, note, velocity, reduced_loundness_oscillator_index);
		} while(0);

		do
		{
			if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
				CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %d, %s, velocity = %d\r\n",
								tick,  "MIDI_MESSAGE_NOTE_OFF",
								voice, get_percussion_name_string(note), velocity);
				break;
			}

			CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %d, note = %d, velocity = %d\r\n",
							tick,  "MIDI_MESSAGE_NOTE_OFF",
							voice, note, velocity);
		}while(0);
	} while(0);

	return 0;
}

/**********************************************************************************/

static int process_pitch_wheel_message(uint32_t const tick, int8_t const voice, int16_t const value)
{
	char delta_hex_string[12] = "";
	do {
		if(value == MIDI_FOURTEEN_BITS_CENTER_VALUE){
			break;
		}
		if(value > MIDI_FOURTEEN_BITS_CENTER_VALUE){
			snprintf(&delta_hex_string[0], sizeof(delta_hex_string),"(+0x%04x)", value - MIDI_FOURTEEN_BITS_CENTER_VALUE);
			break;
		}
		snprintf(&delta_hex_string[0], sizeof(delta_hex_string),"(-0x%04x)", MIDI_FOURTEEN_BITS_CENTER_VALUE - value);
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
			p_oscillator->delta_phase = calculate_oscillator_delta_phase(voice, p_oscillator->note,
																		 p_oscillator->pitch_chorus_bend_in_semitones,
																		 &pitch_bend_in_semitone);
			CHIPTUNE_PRINTF(cNoteOperation, "---- voice = %d, note = %d, pitch bend in semitone = %+3.2f\r\n",
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
		uint32_t data_as_uint32_t;
		unsigned char data_as_bytes[4];
	} u;

	const uint32_t tick = tick_message.tick;
	u.data_as_uint32_t = tick_message.message;

	uint8_t type = u.data_as_bytes[0] & 0xF0;
	int8_t voice = u.data_as_bytes[0] & 0x0F;
	switch(type)
	{
	case MIDI_MESSAGE_NOTE_OFF:
	case MIDI_MESSAGE_NOTE_ON:
		process_note_message(tick, (MIDI_MESSAGE_NOTE_OFF == type) ? false : true,
			voice, SEVEN_BITS_VALID(u.data_as_bytes[1]), SEVEN_BITS_VALID(u.data_as_bytes[2]));
	 break;
	case MIDI_MESSAGE_KEY_PRESSURE:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_KEY_PRESSURE :: note = %d, amount = %d %s\r\n",
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

static int free_note_off_but_damper_pedal_on_oscillators(const uint32_t tick)
{
	int ret = 0;
	int16_t oscillator_index = get_event_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();
	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
		do {
			if(true == IS_NOTE_ON(p_oscillator->state_bits)){
				break;
			}
			channel_controller_t const * const p_channel_controller
					= get_channel_controller_pointer_from_index(p_oscillator->voice);
			if(false == p_channel_controller->is_damper_pedal_on){
				break;
			}

			ret = 1;
			put_event(EVENT_FREE, oscillator_index, tick);
		} while(0);
		oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
	}

	return ret;
}

/**********************************************************************************/

static int free_remaining_oscillators(const uint32_t tick)
{
	int ret = 0;
	do {
		int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();

		int16_t oscillator_index = get_event_occupied_oscillator_head_index();
		for(int16_t i = 0; i < occupied_oscillator_number; i++){
			oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);

			do {
				if(true == IS_FREEING(p_oscillator->state_bits)){
					break;
				}
				channel_controller_t const * const p_channel_controller
						= get_channel_controller_pointer_from_index(p_oscillator->voice);
				if(true == p_channel_controller->is_damper_pedal_on){
					break;
				}

				ret = 1;
				CHIPTUNE_PRINTF(cDeveloping, "oscillator = %d, voice = %d, note = %d is not freed but tune is ending\r\n",
							oscillator_index, p_oscillator->voice, p_oscillator->note);
				put_event(EVENT_FREE, oscillator_index, tick);
			} while(0);
			oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
		}
	} while(0);

	return ret;
}

/**********************************************************************************/

int process_ending(const uint32_t tick)
{
	int ret = 0;
	ret += free_note_off_but_damper_pedal_on_oscillators(tick);
	ret += free_remaining_oscillators(tick);
	return ret;
}

/**********************************************************************************/

static uint32_t s_midi_messge_index = 0;

static uint32_t s_previous_timely_tick = NULL_TICK;
static struct _tick_message s_fetched_tick_message = {NULL_TICK, NULL_MESSAGE};
static uint32_t s_fetched_event_tick = NULL_TICK;

#define RESET_STATIC_INDEX_MESSAGE_TICK_VARIABLES()	\
								do { \
									s_is_tune_ending = false; \
									RESET_CURRENT_TIME(); \
									s_midi_messge_index = 0; \
									s_previous_timely_tick = NULL_TICK; \
									SET_TICK_MESSAGE_NULL(s_fetched_tick_message); \
									s_fetched_event_tick = NULL_TICK; \
								} while(0)

static int process_timely_midi_message_and_event(void)
{
	if(CURRENT_TICK() == s_previous_timely_tick){
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

	s_previous_timely_tick = CURRENT_TICK();
	return ret;
}

/**********************************************************************************/

static void pass_through_midi_messages(const uint32_t end_midi_message_index,
									   int32_t * const p_max_loudness,
									   int16_t * const p_max_event_occupied_oscillator_number)
{
	if(0 == end_midi_message_index){
		return ;
	}

	SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(false);
	int32_t max_loudness = 0;
	int16_t max_event_occupied_oscillator_number = 0;

	RESET_STATIC_INDEX_MESSAGE_TICK_VARIABLES();
	fetch_midi_tick_message(s_midi_messge_index, &s_fetched_tick_message);
	s_midi_messge_index += 1;
	process_midi_message(s_fetched_tick_message);
	s_previous_timely_tick = s_fetched_tick_message.tick;
	SET_TICK_MESSAGE_NULL(s_fetched_tick_message);

	while(1)
	{
		do {
			if(false == IS_NULL_TICK_MESSAGE(s_fetched_tick_message)){
				break;
			}

			fetch_midi_tick_message(s_midi_messge_index, &s_fetched_tick_message);
			if(false == IS_NULL_TICK_MESSAGE(s_fetched_tick_message)){
				s_midi_messge_index += 1;
			}
		} while(0);

		if(end_midi_message_index == s_midi_messge_index){
			int16_t oscillator_index = get_event_occupied_oscillator_head_index();
			int16_t const occupied_oscillator_number = get_event_occupied_oscillator_number();
			for(int16_t i = 0; i < occupied_oscillator_number; i++){
				oscillator_t * const p_oscillator = get_event_oscillator_pointer_from_index(oscillator_index);
				do {
					if(true == IS_NOTE_ON(p_oscillator->state_bits)){
						break;
					}
					channel_controller_t const * const p_channel_controller
							= get_channel_controller_pointer_from_index(p_oscillator->voice);
					if(false == p_channel_controller->is_damper_pedal_on){
						break;
					}
					put_event(EVENT_DEACTIVATE, oscillator_index, (uint32_t)s_current_tick);
				} while(0);
				oscillator_index = get_event_occupied_oscillator_next_index(oscillator_index);
			}
			break;
		}

		if(NULL_TICK == s_fetched_event_tick){
			s_fetched_event_tick = get_next_event_triggering_tick();
		}

		if(true == IS_NULL_TICK_MESSAGE(s_fetched_tick_message)
				&& NULL_TICK == s_fetched_event_tick){
			if(0 == process_ending(CURRENT_TICK())){
				break;
			}
		}

		{
			uint32_t const tick = (s_fetched_tick_message.tick < s_fetched_event_tick) ? s_fetched_tick_message.tick : s_fetched_event_tick;
			s_current_tick = (chiptune_float)tick;
		}

		do
		{
			if((uint32_t)s_current_tick == s_previous_timely_tick){
				break;
			}
			s_previous_timely_tick = (uint32_t)s_current_tick;

			if(max_event_occupied_oscillator_number < get_event_occupied_oscillator_number()){
				max_event_occupied_oscillator_number = get_event_occupied_oscillator_number();
			}

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

		if((uint32_t)s_current_tick == s_fetched_tick_message.tick){
			process_midi_message(s_fetched_tick_message);
			SET_TICK_MESSAGE_NULL(s_fetched_tick_message);

			s_fetched_event_tick = get_next_event_triggering_tick();
		}

		if((uint32_t)s_current_tick == s_fetched_event_tick){
			process_events((uint32_t)s_current_tick);
			s_fetched_event_tick = NULL_TICK;
		}
	}

	SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(true);

	if(NULL != p_max_loudness){
		*p_max_loudness = max_loudness;
	}
	if(NULL != p_max_event_occupied_oscillator_number){
		*p_max_event_occupied_oscillator_number = max_event_occupied_oscillator_number;
	}
}

/**********************************************************************************/

static int32_t get_max_simultaneous_loudness(void)
{
	int32_t max_loudness = 0;
	int16_t max_event_occupied_oscillator_number = 0;
	pass_through_midi_messages(UINT32_MAX, &max_loudness, &max_event_occupied_oscillator_number);

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

	for(int8_t i = 0; i < MIDI_MAX_CHANNEL_NUMBER; i++){
		reset_channel_controller_midi_parameters_from_index(i);
	}

	CHIPTUNE_PRINTF(cDeveloping, "max_event_occupied_oscillator_number = %d\r\n",
					max_event_occupied_oscillator_number);
	CHIPTUNE_PRINTF(cDeveloping, "max_loudness = %d\r\n", max_loudness);
	if(true == is_stereo()){
		max_loudness = max_loudness * 23 / 64;
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

void chiptune_initialize(bool const is_stereo, uint32_t const sampling_rate, uint32_t const resolution)
{
	s_is_stereo = is_stereo;
	s_is_processing_left_channel = true;
	s_sampling_rate = sampling_rate;
	s_resolution = resolution;

	for(int i = 0; i < SINE_TABLE_LENGTH; i++){
		s_sine_table[i] = (int16_t)(INT16_MAX * sinf((float)(2.0 * M_PI * i/SINE_TABLE_LENGTH)));
	}

	initialize_channel_controller();
	clean_all_events();

	UPDATE_BASE_TIME_UNIT();
	UPDATE_AMPLITUDE_NORMALIZER();
	RESET_STATIC_INDEX_MESSAGE_TICK_VARIABLES();
	process_timely_midi_message_and_event();
	return ;
}

/**********************************************************************************/

void chiptune_set_next_midi_message_index(uint32_t const next_midi_message_index)
{
	clean_all_events();
	for(int8_t i = 0; i < MIDI_MAX_CHANNEL_NUMBER; i++){
		reset_channel_controller_midi_parameters_from_index(i);
	}
	RESET_STATIC_INDEX_MESSAGE_TICK_VARIABLES();
	pass_through_midi_messages(next_midi_message_index, NULL, NULL);
	return ;
}

/**********************************************************************************/

void chiptune_set_tempo(float const tempo)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %d, set tempo as %3.1f\r\n", CURRENT_TICK(), tempo);
	CORRECT_BASE_TIME();
	adjust_event_triggering_tick_by_tempo(CURRENT_TICK(), tempo);
	s_tempo = tempo;
	UPDATE_BASE_TIME_UNIT();
	update_effect_tick();
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

		if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == p_oscillator->voice){
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

void perform_pitch_envelope(oscillator_t * const p_oscillator)
{
	do {
		if(true == is_stereo()
				&& false == is_processing_left_channel()){
			break;
		}

		if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == p_oscillator->voice){
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

		if(false == (MIDI_PERCUSSION_INSTRUMENT_CHANNEL == p_oscillator->voice)){
			break;
		}
		percussion_t const * const p_percussion = get_percussion_pointer_from_index(p_oscillator->note);

		p_oscillator->percussion_duration_sample_count += 1;
		int8_t waveform_index = p_oscillator->percussion_waveform_index;
		if (p_percussion->waveform_duration_sample_number[waveform_index]
				== p_oscillator->percussion_duration_sample_count){
			p_oscillator->percussion_duration_sample_count = 0;
			p_oscillator->percussion_waveform_index += 1;
			if(MAX_WAVEFORM_CHANGE_NUMBER == p_oscillator->percussion_waveform_index){
				p_oscillator->percussion_waveform_index = MAX_WAVEFORM_CHANGE_NUMBER - 1;
			}
		}

		p_oscillator->current_phase += PERCUSSION_ENVELOPE(p_percussion->max_delta_modulation_phase,
											p_percussion->p_modulation_envelope_table[p_oscillator->percussion_table_index]);
		p_oscillator->percussion_same_index_count += 1;
		if(p_percussion->envelope_same_index_number > p_oscillator->percussion_same_index_count){
			break;
		}

		p_oscillator->percussion_same_index_count = 0;
		p_oscillator->percussion_table_index += 1;
		p_oscillator->amplitude = PERCUSSION_ENVELOPE(p_oscillator->loudness,
											p_percussion->p_amplitude_envelope_table[p_oscillator->percussion_table_index]);

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
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == p_oscillator->voice){
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

		channel_controller_t const * const p_channel_controller
				= get_channel_controller_pointer_from_index(p_oscillator->voice);
		int8_t channel_panning_weight = p_channel_controller->pan;
		channel_panning_weight += !channel_panning_weight;//if zero, as 1
		if(true == is_processing_left_channel()){
			channel_panning_weight = 2 * MIDI_SEVEN_BITS_CENTER_VALUE - channel_panning_weight;
		}
		channel_wave_amplitude = CHANNEL_WAVE_AMPLITUDE(mono_wave_amplitude, channel_panning_weight)/2;
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
			perform_pitch_envelope(p_oscillator);
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
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: out_value = %d, greater than INT16_MAX\r\n",
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
	return (uint8_t const)(REDUCE_INT16_PRECISION_TO_INT8(chiptune_fetch_16bit_wave()) + INT8_MAX_PLUS_1);
}

/**********************************************************************************/

uint32_t chiptune_get_current_tick(void)
{
	return CURRENT_TICK();
}

/**********************************************************************************/

bool chiptune_is_tune_ending(void)
{
	return s_is_tune_ending;
}
