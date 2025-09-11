//NOLINTBEGIN(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)

#include <string.h>
#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_oscillator_internal.h"
#include "chiptune_event_internal.h"
#include "chiptune_envelope_internal.h"

#include "chiptune_midi_control_change_internal.h"
#include "chiptune_midi_effect_internal.h"

#include "chiptune.h"

#ifdef _INCREMENTAL_SAMPLE_INDEX
typedef float chiptune_float;
#else
typedef double chiptune_float;
#endif

#define DEFAULT_SAMPLING_RATE						(16000)
#define DEFAULT_PLAYING_SPEED_RATIO					(1.0)

static float s_tempo = MIDI_DEFAULT_TEMPO;
static float s_playing_speed_ratio = DEFAULT_PLAYING_SPEED_RATIO;
static uint32_t s_sampling_rate = DEFAULT_SAMPLING_RATE;
static uint32_t s_resolution = MIDI_DEFAULT_RESOLUTION;
static bool s_is_stereo = false;
bool s_is_processing_left_channel = true;

#ifdef _INCREMENTAL_SAMPLE_INDEX
static uint32_t s_current_sample_index = 0;
static chiptune_float s_tick_to_sample_index_ratio = \
		(chiptune_float)(DEFAULT_SAMPLING_RATE * 1.0/(MIDI_DEFAULT_TEMPO/DEFAULT_PLAYING_SPEED_RATIO/60.0)/MIDI_DEFAULT_RESOLUTION);

#define UPDATE_SAMPLES_TO_TICK_RATIO()				\
													do { \
														s_tick_to_sample_index_ratio \
														= (chiptune_float)(s_sampling_rate * 60.0/s_tempo/s_playing_speed_ratio/s_resolution); \
													} while(0)

#define UPDATE_SAMPLING_RATE(SAMPLING_RATE)			\
													do { \
														s_sampling_rate = (SAMPLING_RATE); \
														UPDATE_SAMPLES_TO_TICK_RATIO(); \
													} while(0)

#define UPDATE_RESOLUTION(RESOLUTION)				\
													do { \
														s_resolution = (RESOLUTION); \
														UPDATE_SAMPLES_TO_TICK_RATIO(); \
													} while(0)

#define UPDATE_TEMPO(TEMPO)							\
													do { \
														s_current_sample_index = (uint32_t)(s_current_sample_index * s_tempo/(TEMPO)); \
														s_tempo = (TEMPO); \
														UPDATE_SAMPLES_TO_TICK_RATIO(); \
													} while(0)


#define UPDATE_PLAYING_SPEED_RATIO(PLAYING_SPEED_RATIO) \
													do { \
														s_current_sample_index = (uint32_t)(s_current_sample_index * s_playing_speed_ratio/(PLAYING_SPEED_RATIO)); \
														s_playing_speed_ratio = (PLAYING_SPEED_RATIO); \
														UPDATE_SAMPLES_TO_TICK_RATIO(); \
													} while(0)

#define TICK_TO_SAMPLE_INDEX(TICK)					((uint32_t)(s_tick_to_sample_index_ratio * (chiptune_float)(TICK) + 0.5))

#define RESET_CURRENT_TICK()						\
													do { \
														s_current_sample_index = 0; \
													} while(0)

#define CURRENT_TICK()								( (uint32_t)(s_current_sample_index/(s_tick_to_sample_index_ratio) + 0.5))

#define INCREMENT_CURRENT_TICK_BY_ONE_SAMPLE()			\
													do { \
														s_current_sample_index += 1; \
													} while(0)

#define UPDATE_CURRENT_TICK(TICK)					do { \
														s_current_sample_index = (uint32_t)((TICK) * s_tick_to_sample_index_ratio + 0.5); \
													} while(0)

#else
static chiptune_float s_current_tick = (chiptune_float)0.0;
static chiptune_float s_delta_tick_per_sample = (MIDI_DEFAULT_RESOLUTION / ( (chiptune_float)DEFAULT_SAMPLING_RATE/((chiptune_float)MIDI_DEFAULT_TEMPO / (chiptune_float)60.0) ) );

#define	UPDATE_DELTA_TICK_PER_SAMPLE()				\
													do { \
														s_delta_tick_per_sample = ( s_resolution * s_tempo * s_playing_speed_ratio/ (chiptune_float)s_sampling_rate/(chiptune_float)60.0 ); \
													} while(0)

#define UPDATE_SAMPLING_RATE(SAMPLING_RATE)			\
													do { \
														s_sampling_rate = (SAMPLING_RATE); \
														UPDATE_DELTA_TICK_PER_SAMPLE(); \
													} while(0)

#define UPDATE_RESOLUTION(RESOLUTION)				\
													do { \
														s_resolution = (RESOLUTION); \
														UPDATE_DELTA_TICK_PER_SAMPLE(); \
													} while(0)

#define UPDATE_TEMPO(TEMPO)							\
													do { \
														s_tempo = (TEMPO); \
														UPDATE_DELTA_TICK_PER_SAMPLE(); \
													} while(0)

#define UPDATE_PLAYING_SPEED_RATIO(PLAYING_SPEED_RATIO) \
													do { \
														s_playing_speed_ratio = (PLAYING_SPEED_RATIO); \
														UPDATE_TEMPO(s_tempo); \
													} while(0)

#define RESET_CURRENT_TICK()						\
													do { \
														s_current_tick = 0.0; \
													} while(0)

#define CURRENT_TICK()								((uint32_t)(s_current_tick + 0.5))

#define INCREMENT_CURRENT_TICK_BY_ONE_SAMPLE()			\
													do { \
														s_current_tick += s_delta_tick_per_sample; \
													} while(0)

#define UPDATE_CURRENT_TICK(TICK)					do { \
														s_current_tick = (chiptune_float)(TICK); \
													} while(0)

#endif

#define SINE_TABLE_LENGTH							(2048)
static int16_t s_sine_table[SINE_TABLE_LENGTH]		= {0};

/**********************************************************************************/

static chiptune_get_midi_message_callback_t s_handler_get_midi_message = NULL;
static bool s_is_tune_ending = false;

/**********************************************************************************/

uint32_t const get_sampling_rate(void) { return s_sampling_rate; }

/**********************************************************************************/

uint32_t const get_resolution(void) { return s_resolution; }

/**********************************************************************************/

static float const get_playing_speed_ratio(void) {return s_playing_speed_ratio; }

/**********************************************************************************/

float const get_playing_tempo(void)
{
	return s_tempo * s_playing_speed_ratio;
}

/**********************************************************************************/

static inline bool const is_stereo() { return s_is_stereo;}

/**********************************************************************************/

static inline bool const is_processing_left_channel() { return s_is_processing_left_channel; }

/**********************************************************************************/

static inline void swap_processing_channel() { s_is_processing_left_channel = !s_is_processing_left_channel; }

/**********************************************************************************/

int setup_pitch_oscillator(uint32_t const tick, int8_t const voice, int8_t const note, int8_t const velocity,
									oscillator_t * const p_oscillator)
{
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
		return 1;
	}
	(void)tick;
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);

	p_oscillator->amplitude = 0;

	p_oscillator->pitch_chorus_bend_in_semitones = 0;
	p_oscillator->delta_phase
			= calculate_oscillator_delta_phase(voice, p_oscillator->note,
											   p_oscillator->pitch_chorus_bend_in_semitones);

	p_oscillator->max_delta_vibrato_phase
			= calculate_oscillator_delta_phase(voice,
				p_oscillator->note + p_channel_controller->vibrato_modulation_in_semitones,
				p_oscillator->pitch_chorus_bend_in_semitones) - p_oscillator->delta_phase;

	p_oscillator->vibrato_table_index = 0;
	p_oscillator->vibrato_same_index_count = 0;

	p_oscillator->envelope_same_index_count = 0;
	p_oscillator->envelope_table_index = 0;
	p_oscillator->release_reference_amplitude = 0;

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

	return 0;
}

/**********************************************************************************/

static void rest_occupied_oscillator_with_same_voice_note(uint32_t const tick,
														  int8_t const voice, int8_t const note, int8_t const velocity)
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
			if(false == IS_NATIVE_OSCILLATOR(p_oscillator->state_bits)){
				break;
			}
			if(true == IS_FREEING_OR_PREPARE_TO_FREE(p_oscillator->state_bits)){
				break;
			}
			if(true == IS_RESTING_OR_PREPARE_TO_REST(p_oscillator->state_bits)){
				break;
			}
			put_event(EVENT_REST, oscillator_index, tick);
			process_effects(tick, EVENT_REST, voice, note, velocity, oscillator_index);
		} while(0);
		oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
	}
}

/**********************************************************************************/

static int process_note_message(uint32_t const tick, bool const is_note_on,
						 int8_t const voice, int8_t const note, int8_t const velocity)
{
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
		if(NULL == get_percussion_pointer_from_index(note)){
			CHIPTUNE_PRINTF(cDeveloping, "WARNING:: tick = %u, PERCUSSION_INSTRUMENT = %d (%s)"
										 " does not be defined in the MIDI standard, ignored\r\n",
							tick, note, is_note_on ? "on" : "off");
			return 1;
		}
	}

	do {
		if(true == is_note_on){

			do {
				if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
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
			rest_occupied_oscillator_with_same_voice_note(tick, voice, note, velocity);

			int16_t oscillator_index;
			oscillator_t * const p_oscillator = acquire_oscillator(&oscillator_index);
			if(NULL == p_oscillator){
				return -1;
			}
			memset(p_oscillator, 0, sizeof(oscillator_t));
			RESET_STATE_BITES(p_oscillator->state_bits);
			SET_NOTE_ON(p_oscillator->state_bits);
			p_oscillator->voice = voice;
			p_oscillator->note = note;

			int16_t expression_added_pressure = p_channel_controller->expression
					+ NORMALIZE_PRESSURE(p_channel_controller->pressure);
			int32_t temp = (velocity * expression_added_pressure * p_channel_controller->volume)/INT8_MAX;
			if(temp > INT16_MAX){
				CHIPTUNE_PRINTF(cDeveloping, "WARNING :: loudness over IN16_MAX in %s\r\n", __func__);
				temp = INT16_MAX;
			}
			p_oscillator->loudness = temp;
			memset(&p_oscillator->associate_oscillators[0], UNOCCUPIED_OSCILLATOR, MAX_ASSOCIATE_OSCILLATOR_NUMBER * sizeof(int16_t));

			p_oscillator->current_phase = 0;
			do {
				if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
					setup_percussion_oscillator(tick, voice, note, velocity, p_oscillator);
					break;
				}

				setup_pitch_oscillator(tick, voice, note, velocity, p_oscillator);
			} while(0);

			put_event(EVENT_ACTIVATE, oscillator_index, tick);
			process_effects(tick, EVENT_ACTIVATE, voice, note, velocity, oscillator_index);
			break;
		}

		do {
			if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
				CHIPTUNE_PRINTF(cMidiNote, "tick = %u, %s :: voice = %d, %s, velocity = %d\r\n",
								tick,  "MIDI_MESSAGE_NOTE_OFF",
								voice, get_percussion_name_string(note), velocity);
				break;
			}

			CHIPTUNE_PRINTF(cMidiNote, "tick = %u, %s :: voice = %d, note = %d, velocity = %d\r\n",
							tick,  "MIDI_MESSAGE_NOTE_OFF",
							voice, note, velocity);
		} while(0);

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

				if(false == IS_NATIVE_OSCILLATOR(p_oscillator->state_bits)){
					break;
				}
				if(false == IS_NOTE_ON(p_oscillator->state_bits)){
					break;
				}
				if(true == IS_FREEING_OR_PREPARE_TO_FREE(p_oscillator->state_bits)){
					break;
				}
				put_event(EVENT_FREE, oscillator_index, tick);
				process_effects(tick, EVENT_FREE, voice, note, velocity, oscillator_index);
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

		do {
			if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == voice){
				break;
			}
			if(false == p_channel_controller->is_damper_pedal_on){
				break;
			}
			int16_t const original_oscillator_index = oscillator_index;
			int16_t reduced_loundness_oscillator_index;
			oscillator_t * const p_oscillator = acquire_oscillator(&reduced_loundness_oscillator_index);
			memcpy(p_oscillator, get_oscillator_pointer_from_index(original_oscillator_index),
				   sizeof(oscillator_t));
			RESET_STATE_BITES(p_oscillator->state_bits);
			SET_NOTE_OFF(p_oscillator->state_bits);
			memset(&p_oscillator->associate_oscillators[0], UNOCCUPIED_OSCILLATOR, MAX_ASSOCIATE_OSCILLATOR_NUMBER * sizeof(int16_t));
			p_oscillator->loudness
					= LOUNDNESS_AS_DAMPING_PEDAL_ON_BUT_NOTE_OFF(
						p_oscillator->loudness,
						MAP_MIDI_VALUE_RANGE_TO_0_128(p_channel_controller->envelop_damper_on_but_note_off_sustain_level));
			p_oscillator->amplitude = 0;
			p_oscillator->envelope_same_index_count = 0;
			p_oscillator->envelope_table_index = 0;
			p_oscillator->release_reference_amplitude = 0;
			put_event(EVENT_ACTIVATE, reduced_loundness_oscillator_index, tick);
			process_effects(tick, EVENT_ACTIVATE, voice, note, velocity, reduced_loundness_oscillator_index);
		} while(0);

	} while(0);

	return 0;
}

/**********************************************************************************/

static int process_program_change_message(uint32_t const tick, int8_t const voice, int8_t const number)
{
	do {
		if(CHANNEL_CONTROLLER_INSTRUMENT_NOT_SPECIFIED != get_channel_controller_pointer_from_index(voice)->instrument
				&& CHANNEL_CONTROLLER_INSTRUMENT_UNUSED_CHANNEL != get_channel_controller_pointer_from_index(voice)->instrument){
			if(number != get_channel_controller_pointer_from_index(voice)->instrument){
				CHIPTUNE_PRINTF(cMidiProgramChange, "tick = %u, voice = %d, MIDI_MESSAGE_PROGRAM_CHANGE: from %s(%d) to %s(%d)\r\n",
								tick, voice,
								get_instrument_name_string(get_channel_controller_pointer_from_index(voice)->instrument),
								get_channel_controller_pointer_from_index(voice)->instrument,
								get_instrument_name_string(number),
								number);
			}
			break;
		}
		CHIPTUNE_PRINTF(cMidiProgramChange, "tick = %u, MIDI_MESSAGE_PROGRAM_CHANGE :: voice = %d, instrument = %s(%d)\r\n",
						tick, voice, get_instrument_name_string(number), number);
	}while(0);

	get_channel_controller_pointer_from_index(voice)->instrument = number;
	return 0;
}

/**********************************************************************************/

static int process_channel_pressure_message(uint32_t const tick, int8_t const voice, int8_t const value)
{
	CHIPTUNE_PRINTF(cMidiChannelPressure, "tick = %u, MIDI_MESSAGE_CHANNEL_PRESSURE :: voice = %d, value = %d, and expression = %d\r\n",
					tick, voice, value, get_channel_controller_pointer_from_index(voice)->expression);

	process_loudness_change(tick, voice, value, LoundnessChangePressure);
	return 0;
}

/**********************************************************************************/

static int process_pitch_wheel_message(uint32_t const tick, int8_t const voice, int16_t const value)
{
	channel_controller_t * const p_channel_controller = get_channel_controller_pointer_from_index(voice);
	p_channel_controller->pitch_wheel_bend_in_semitones
			= ((value - MIDI_FOURTEEN_BITS_CENTER_VALUE)/(float)MIDI_FOURTEEN_BITS_CENTER_VALUE)
			* DIVIDE_BY_2(p_channel_controller->pitch_wheel_bend_range_in_semitones);

	int16_t oscillator_index = get_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_occupied_oscillator_number();

	CHIPTUNE_PRINTF(cMidiPitchWheel, "tick = %u, MIDI_MESSAGE_PITCH_WHEEL :: voice = %d, pitch_wheel_bend_in_semitones = %+3.2f\r\n",
					tick, voice, p_channel_controller->pitch_wheel_bend_in_semitones);
	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
		do {
			if(voice != p_oscillator->voice){
				break;
			}
			p_oscillator->delta_phase = calculate_oscillator_delta_phase(voice, p_oscillator->note,
																		 p_oscillator->pitch_chorus_bend_in_semitones);
		} while(0);
		oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
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
#define NULL_MESSAGE								(0)

#define IS_NULL_TICK_MESSAGE(MESSAGE_TICK)			\
								(((NULL_TICK == (MESSAGE_TICK).tick) && (NULL_MESSAGE == (MESSAGE_TICK).message) ) ? true : false)

#define SET_TICK_MESSAGE_NULL(MESSAGE_TICK)			\
								do { \
									(MESSAGE_TICK).tick = NULL_TICK; \
									(MESSAGE_TICK).message = NULL_MESSAGE; \
								} while(0)

#define SEVEN_BITS_VALID(VALUE)						((0x7F) & (VALUE))

int8_t s_ending_instrument_array[MIDI_MAX_CHANNEL_NUMBER];
#ifndef _KEEP_NOTELESS_CHANNELS
bool s_is_chase_to_last_done = false;
#endif

static int process_midi_message(struct _tick_message const tick_message)
{
	int ret = -1;
	do
	{
		if(true == IS_NULL_TICK_MESSAGE(tick_message)){
			break;
		}

		union {
			uint32_t data_as_uint32_t;
			unsigned char data_as_bytes[4];
		} u;

		const uint32_t tick = tick_message.tick;
		u.data_as_uint32_t = tick_message.message;

		uint8_t type = u.data_as_bytes[0] & 0xF0;
		int8_t voice = u.data_as_bytes[0] & 0x0F;
#ifndef _KEEP_NOTELESS_CHANNELS
		if(true == s_is_chase_to_last_done){
			if(CHIPTUNE_INSTRUMENT_UNUSED_CHANNEL == s_ending_instrument_array[voice]){
				ret = 1;
				break;
			}
		}
#endif
		switch(type)
		{
		case MIDI_MESSAGE_NOTE_OFF:
		case MIDI_MESSAGE_NOTE_ON:
			process_note_message(tick, (MIDI_MESSAGE_NOTE_OFF == type) ? false : true,
				voice, SEVEN_BITS_VALID(u.data_as_bytes[1]), SEVEN_BITS_VALID(u.data_as_bytes[2]));
		 break;
		case MIDI_MESSAGE_KEY_PRESSURE:
			CHIPTUNE_PRINTF(cMidiControlChange,
							"tick = %u, MIDI_MESSAGE_KEY_PRESSURE :: note = %d, amount = %d %s\r\n",
							tick, voice, SEVEN_BITS_VALID(u.data_as_bytes[1]), "(NOT IMPLEMENTED YET)");
			break;
		case MIDI_MESSAGE_CONTROL_CHANGE:
			process_control_change_message(tick, voice, SEVEN_BITS_VALID(u.data_as_bytes[1]),
										   SEVEN_BITS_VALID(u.data_as_bytes[2]));
			break;
		case MIDI_MESSAGE_PROGRAM_CHANGE:
			process_program_change_message(tick, voice, SEVEN_BITS_VALID(u.data_as_bytes[1]));
			break;
		case MIDI_MESSAGE_CHANNEL_PRESSURE:
			process_channel_pressure_message(tick, voice, SEVEN_BITS_VALID(u.data_as_bytes[1]));
			break;
		case MIDI_MESSAGE_PITCH_WHEEL:
#define COMBINE_AS_PITCH_WHEEL_14BITS(BYTE1, BYTE2)	\
														(SEVEN_BITS_VALID(BYTE2) << 7) | SEVEN_BITS_VALID(BYTE1))
			process_pitch_wheel_message(tick, voice,
										COMBINE_AS_PITCH_WHEEL_14BITS(u.data_as_bytes[1], u.data_as_bytes[2]);
			break;
		default:
			//CHIPTUNE_PRINTF(cMidiControlChange, "tick = %u, MIDI_MESSAGE code = %u :: voice = %d, byte 1 = %d, byte 2 = %d %s\r\n",
			//				tick, type, voice, SEVEN_BITS_VALID(u.data_as_bytes[1]), SEVEN_BITS_VALID(u.data_as_bytes[2]), "(NOT IMPLEMENTED YET)");
			break;
		}
		ret = 0;
	}while(0);

	return ret;
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
	int16_t oscillator_index = get_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
	for(int16_t i = 0; i < occupied_oscillator_number; i++){
		oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
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
		oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
	}

	return ret;
}

/**********************************************************************************/

static int free_remaining_oscillators(const uint32_t tick)
{
	int ret = 0;
	do {
		int16_t const occupied_oscillator_number = get_occupied_oscillator_number();

		int16_t oscillator_index = get_occupied_oscillator_head_index();
		for(int16_t i = 0; i < occupied_oscillator_number; i++){
			oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);

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
			oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
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
									RESET_CURRENT_TICK(); \
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
		do {
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

			if(CURRENT_TICK() < s_fetched_tick_message.tick){
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
			if(CURRENT_TICK() < s_fetched_event_tick){
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

		p_oscillator->current_phase += DELTA_VIBRATO_PHASE(modulation_wheel, p_oscillator->max_delta_vibrato_phase,
										p_channel_controller->p_vibrato_phase_table[p_oscillator->vibrato_table_index]);
		p_oscillator->vibrato_same_index_count += 1;
		if(p_channel_controller->vibrato_same_index_number == p_oscillator->vibrato_same_index_count){
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

		advance_pitch_amplitude(p_oscillator);
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

		advance_percussion_waveform_and_amplitude(p_oscillator);
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
#if(0)
#define CHANNEL_WAVE_AMPLITUDE(MONO_WAVE_AMPLITUDE, CHANNEL_PANNING_WEIGHT) \
													MULTIPLY_BY_2( \
														DIVIDE_BY_128((int64_t)(MONO_WAVE_AMPLITUDE) * (CHANNEL_PANNING_WEIGHT)) \
													)
#else
#define CHANNEL_WAVE_AMPLITUDE(MONO_WAVE_AMPLITUDE, CHANNEL_PANNING_WEIGHT) \
														\
														DIVIDE_BY_128((int64_t)(MONO_WAVE_AMPLITUDE) * (CHANNEL_PANNING_WEIGHT))

#endif

int32_t generate_channel_wave_amplitude(oscillator_t * const p_oscillator,
				   int32_t const mono_wave_amplitude)
{
	int32_t channel_wave_amplitude = mono_wave_amplitude;
	do {
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
static bool s_is_channels_output_enabled_array[CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER];

static int64_t chiptune_fetch_64bit_wave(void)
{
	if(-1 == process_timely_midi_message_and_event()){
		s_is_tune_ending = true;
	}

	int64_t accumulated_wave_amplitude = 0;
	int16_t oscillator_index = get_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
	for(int16_t k = 0; k < occupied_oscillator_number; k++){
		oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
		do {
			if(false == s_is_channels_output_enabled_array[p_oscillator->voice]){
				break;
			}
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
		oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
	}

	if(false == is_stereo()
			|| false == is_processing_left_channel()){
		INCREMENT_CURRENT_TICK_BY_ONE_SAMPLE();
	}

	if(true == is_stereo()){
		swap_processing_channel();
	}

	return accumulated_wave_amplitude;
}

/**********************************************************************************/

static void clear_all_oscillators_and_events(void)
{
	clear_all_oscillators();
	clear_all_events();
}

/**********************************************************************************/

static void destroy_all_oscillators_and_events(void)
{
	destroy_all_oscillators();
	destroy_all_events();
}

/**********************************************************************************/
#define TO_LAST_MESSAGE_INDEX						(-1)

static void chase_midi_messages(const uint32_t end_midi_message_index)
{
	for(int8_t voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		reset_channel_controller_all_parameters(voice);
	}

	clear_all_oscillators_and_events();
	RESET_STATIC_INDEX_MESSAGE_TICK_VARIABLES();
	if(0 == end_midi_message_index){
		return ;
	}

	SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(false);
#ifndef _KEEP_NOTELESS_CHANNELS
	bool is_has_note[CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER];
	memset(&is_has_note[0], false, sizeof(bool)*CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER);
#endif
	int16_t max_event_occupied_oscillator_number = 0;

	fetch_midi_tick_message(s_midi_messge_index, &s_fetched_tick_message);
	s_midi_messge_index += 1;
	process_midi_message(s_fetched_tick_message);
	s_previous_timely_tick = s_fetched_tick_message.tick;
	SET_TICK_MESSAGE_NULL(s_fetched_tick_message);

	while(1)
	{
		bool is_reach_end_midi_message_index = false;
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
			int16_t oscillator_index = get_occupied_oscillator_head_index();
			int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
			for(int16_t i = 0; i < occupied_oscillator_number; i++){
				oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
				do {
					if(true == IS_NOTE_ON(p_oscillator->state_bits)){
						break;
					}
					channel_controller_t const * const p_channel_controller
							= get_channel_controller_pointer_from_index(p_oscillator->voice);
					if(false == p_channel_controller->is_damper_pedal_on){
						break;
					}
					put_event(EVENT_DEACTIVATE, oscillator_index, CURRENT_TICK());
				} while(0);
				oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
			}
			is_reach_end_midi_message_index = true;
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
			UPDATE_CURRENT_TICK(tick);
		}

		do
		{
			if(CURRENT_TICK() == s_previous_timely_tick){
				break;
			}

			s_previous_timely_tick = CURRENT_TICK();

			if(max_event_occupied_oscillator_number < get_occupied_oscillator_number()){
				max_event_occupied_oscillator_number = get_occupied_oscillator_number();
			}

			int16_t oscillator_index = get_occupied_oscillator_head_index();
			int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
			for(int16_t i = 0; i < occupied_oscillator_number; i++,
				oscillator_index = get_occupied_oscillator_next_index(oscillator_index)){
				oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
				if(CHANNEL_CONTROLLER_INSTRUMENT_UNUSED_CHANNEL
						== get_channel_controller_pointer_from_index(p_oscillator->voice)->instrument){
					get_channel_controller_pointer_from_index(p_oscillator->voice)->instrument = CHIPTUNE_INSTRUMENT_NOT_SPECIFIED;
				}
#ifndef _KEEP_NOTELESS_CHANNELS
				is_has_note[p_oscillator->voice] = true;
#endif
			}
		}while(0);

		if(CURRENT_TICK() == s_fetched_tick_message.tick){
			process_midi_message(s_fetched_tick_message);
			SET_TICK_MESSAGE_NULL(s_fetched_tick_message);

			s_fetched_event_tick = get_next_event_triggering_tick();
		}

		if(CURRENT_TICK() == s_fetched_event_tick){
			process_events(CURRENT_TICK());
			s_fetched_event_tick = NULL_TICK;
		}

		if(true == is_reach_end_midi_message_index){
			break;
		}
	}

	if(-1 == end_midi_message_index){
		CHIPTUNE_PRINTF(cDeveloping, "max_event_occupied_oscillator_number = %d\r\n",
						max_event_occupied_oscillator_number);
	}

#ifndef _KEEP_NOTELESS_CHANNELS
	for(int8_t voice = 0; voice < CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER; voice++){
		if(false == is_has_note[voice]){
			get_channel_controller_pointer_from_index(voice)->instrument
					= CHANNEL_CONTROLLER_INSTRUMENT_UNUSED_CHANNEL;
		}
	}
#endif
	SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(true);
}

/**********************************************************************************/
#ifdef _AMPLITUDE_NORMALIZATION_BY_RIGHT_SHIFT

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

/**********************************************************************************/
#define DEFAULT_AMPLITUDE_NORMALIZATION_RIGHT_SHIFT_BIT_NUMBER	(13)
int32_t s_loudness_nomalization_right_shift = DEFAULT_AMPLITUDE_NORMALIZATION_RIGHT_SHIFT_BIT_NUMBER;
#define RESET_AMPLITUDE_NORMALIZATION_GAIN()		\
													do { \
														s_loudness_nomalization_right_shift \
															= DEFAULT_AMPLITUDE_NORMALIZATION_RIGHT_SHIFT_BIT_NUMBER; \
													}while(0)

#define AMPLITUDE_NORMALIZATION_GAIN()				(1 << s_loudness_nomalization_right_shift)
#define UPDATE_AMPLITUDE_NORMALIZATION_GAIN(AMPLITUDE_NORMALIZATION_GAIN)	\
													do { \
														s_loudness_nomalization_right_shift \
															= number_of_roundup_to_power2_left_shift_bits(AMPLITUDE_NORMALIZATION_GAIN); \
													}while(0)
#define NORMALIZE_WAVE_AMPLITUDE(WAVE_AMPLITUDE)		((int32_t)((WAVE_AMPLITUDE) >> s_loudness_nomalization_right_shift))
#else
#define DEFAULT_AMPLITUDE_NORMALIZATION_GAIN		(1024)
static int32_t s_amplitude_normaliztion_gain = DEFAULT_AMPLITUDE_NORMALIZATION_GAIN;
#define RESET_AMPLITUDE_NORMALIZATION_GAIN()		\
													do { \
														s_amplitude_normaliztion_gain = DEFAULT_AMPLITUDE_NORMALIZATION_GAIN; \
													}while(0)

#define AMPLITUDE_NORMALIZATION_GAIN()				(s_amplitude_normaliztion_gain)
#define UPDATE_AMPLITUDE_NORMALIZATION_GAIN(AMPLITUDE_NORMALIZATION_GAIN)	\
													do { \
														s_amplitude_normaliztion_gain = (AMPLITUDE_NORMALIZATION_GAIN); \
													}while(0)

#define NORMALIZE_WAVE_AMPLITUDE(WAVE_AMPLITUDE)		((int32_t)((WAVE_AMPLITUDE)/(int32_t)s_amplitude_normaliztion_gain))
#endif

static void get_ending_instruments(int8_t instrument_array[MIDI_MAX_CHANNEL_NUMBER])
{
#ifndef _KEEP_NOTELESS_CHANNELS
	s_is_chase_to_last_done = false;
#endif
	chase_midi_messages(TO_LAST_MESSAGE_INDEX);
	for(int8_t voice = 0; voice < CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER; voice++){
		instrument_array[voice] = get_channel_controller_pointer_from_index(voice)->instrument;
	}
#ifndef _KEEP_NOTELESS_CHANNELS
	s_is_chase_to_last_done = true;
#endif
	for(int8_t voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		reset_channel_controller_all_parameters(voice);
	}
	clear_all_oscillators_and_events();
	RESET_STATIC_INDEX_MESSAGE_TICK_VARIABLES();
	RESET_AMPLITUDE_NORMALIZATION_GAIN();
}

/**********************************************************************************/

void chiptune_initialize(bool const is_stereo, uint32_t const sampling_rate,
						 chiptune_get_midi_message_callback_t get_midi_message_callback)
{
	s_is_stereo = is_stereo;
	s_is_processing_left_channel = true;
	s_handler_get_midi_message = get_midi_message_callback;
	UPDATE_SAMPLING_RATE(sampling_rate);
	UPDATE_RESOLUTION(MIDI_DEFAULT_RESOLUTION);
	UPDATE_TEMPO(MIDI_DEFAULT_TEMPO);
	for(int16_t i = 0; i < SINE_TABLE_LENGTH; i++){
		s_sine_table[i] = (int16_t)(INT16_MAX * sinf((float)(2.0 * M_PI * i/SINE_TABLE_LENGTH)));
	}
	initialize_channel_controllers();
	return ;
}
/**********************************************************************************/

void chiptune_finalize(void)
{
	destroy_all_oscillators_and_events();
}

/**********************************************************************************/

void chiptune_prepare_song(uint32_t const resolution)
{
	UPDATE_RESOLUTION(resolution);
	for(int8_t voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		s_is_channels_output_enabled_array[voice] = true;
	}
	clear_all_oscillators_and_events();
	for(int8_t voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		reset_channel_controller_all_parameters(voice);
	}
	get_ending_instruments(&s_ending_instrument_array[0]);

	RESET_STATIC_INDEX_MESSAGE_TICK_VARIABLES();
	RESET_AMPLITUDE_NORMALIZATION_GAIN();
}

/**********************************************************************************/

int32_t chiptune_get_amplitude_gain(void){ return AMPLITUDE_NORMALIZATION_GAIN(); }

/**********************************************************************************/

void chiptune_set_amplitude_gain(int32_t amplitude_gain) { UPDATE_AMPLITUDE_NORMALIZATION_GAIN(amplitude_gain); }

/**********************************************************************************/

void chiptune_set_current_message_index(uint32_t const index)
{
	chase_midi_messages(index);
	RESET_AMPLITUDE_NORMALIZATION_GAIN();
	return ;
}

/**********************************************************************************/

void chiptune_set_tempo(float const tempo)
{
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %d, set tempo as %3.1f\r\n", CURRENT_TICK(), tempo);
	adjust_event_triggering_tick_by_playing_tempo(CURRENT_TICK(), tempo * get_playing_speed_ratio());
	UPDATE_TEMPO(tempo);
	update_effect_tick();
	update_channel_controllers_parameters_related_to_playing_tempo();
}

/**********************************************************************************/

float chiptune_get_tempo(void)
{
	return s_tempo;
}

/**********************************************************************************/

void chiptune_set_playing_speed_ratio(float playing_speed_ratio)
{
	adjust_event_triggering_tick_by_playing_tempo(CURRENT_TICK(), chiptune_get_tempo() * playing_speed_ratio);
	UPDATE_PLAYING_SPEED_RATIO(playing_speed_ratio);
	update_effect_tick();
	update_channel_controllers_parameters_related_to_playing_tempo();
}
/**********************************************************************************/

float chiptune_get_playing_effective_tempo(void)
{
	return get_playing_tempo();
}

/**********************************************************************************/

int chiptune_get_ending_instruments(int8_t instrument_array[CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER])
{
	memcpy(&instrument_array[0], &s_ending_instrument_array[0], CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER * sizeof(int8_t));
	return 0;
}

/**********************************************************************************/

int16_t chiptune_fetch_16bit_wave(void)
{
	int64_t wave_64bit = chiptune_fetch_64bit_wave();
	int32_t original_amplitude_normaliztion_gain = AMPLITUDE_NORMALIZATION_GAIN();
	while(1)
	{
		int32_t wave_32bit;
		wave_32bit = NORMALIZE_WAVE_AMPLITUDE(wave_64bit);
		do {
			if(INT16_MAX < wave_32bit){
				break;
			}
			if(-INT16_MAX_PLUS_1 > wave_32bit){
				break;
			}

			if(AMPLITUDE_NORMALIZATION_GAIN() != original_amplitude_normaliztion_gain){
				CHIPTUNE_PRINTF(cDeveloping, "change NORMALIZE_WAVE_AMPLITUDE as %d\r\n", AMPLITUDE_NORMALIZATION_GAIN());
			}
			return (int16_t)wave_32bit;
		}while(0);

		UPDATE_AMPLITUDE_NORMALIZATION_GAIN(AMPLITUDE_NORMALIZATION_GAIN() + 256);
	} while(1);

	return 0;
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

/**********************************************************************************/

void chiptune_set_channel_output_enabled(int8_t const channel_index, bool const is_enabled)
{
	if( 0 > channel_index  || channel_index >= MIDI_MAX_CHANNEL_NUMBER){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: channel_index = %d is not acceptable for %s\r\n",
						channel_index, __func__);
		return ;
	}

	s_is_channels_output_enabled_array[channel_index] = is_enabled;
}

/**********************************************************************************/

int chiptune_set_pitch_channel_timbre(int8_t const channel_index, int8_t const waveform,
									  int8_t const envelope_attack_curve, float const envelope_attack_duration_in_seconds,
									  int8_t const envelope_decay_curve, float const envelope_decay_duration_in_seconds,
									  uint8_t const envelope_sustain_level,
									  int8_t const envelope_release_curve, float const envelope_release_duration_in_seconds,
									  uint8_t const envelope_damper_on_but_note_off_sustain_level,
									  int8_t const envelope_damper_on_but_note_off_sustain_curve,
									  float const envelope_damper_on_but_note_off_sustain_duration_in_seconds)
{
	if( 0 > channel_index  || channel_index >= MIDI_MAX_CHANNEL_NUMBER){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: channel_index = %d is not acceptable for %s\r\n",
						channel_index, __func__);
		return -1;
	}
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == channel_index){
		CHIPTUNE_PRINTF(cDeveloping, "WARNING :: channel_index = %d is MIDI_PERCUSSION_INSTRUMENT_CHANNEL %s\r\n",
						MIDI_PERCUSSION_INSTRUMENT_CHANNEL, __func__);
		return 1;
	}

	int8_t channel_controller_waveform = WAVEFORM_SQUARE;
	uint16_t dutycycle_critical_phase = DUTY_CYLCE_NONE;
	switch(waveform)
	{
	case CHIPTUNE_WAVEFORM_SQUARE_DUDYCYCLE_50:
		dutycycle_critical_phase = DUTY_CYLCE_50_CRITICAL_PHASE;
		break;
	case CHIPTUNE_WAVEFORM_SQUARE_DUDYCYCLE_25:
		dutycycle_critical_phase = DUTY_CYLCE_25_CRITICAL_PHASE;
		break;
	case CHIPTUNE_WAVEFORM_SQUARE_DUDYCYCLE_125:
		dutycycle_critical_phase = DUTY_CYLCE_125_CRITICAL_PHASE;
		break;
	case CHIPTUNE_WAVEFORM_SQUARE_DUDYCYCLE_75:
		dutycycle_critical_phase = DUTY_CYLCE_75_CRITICAL_PHASE;
		break;
	case CHIPTUNE_WAVEFORM_TRIANGLE:
		channel_controller_waveform = WAVEFORM_TRIANGLE;
		break;
	case CHIPTUNE_WAVEFORM_SAW:
		channel_controller_waveform = WAVEFORM_SAW;
		break;
	default:
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: waveform = %d is not acceptable for %s\r\n",
						waveform, __func__);
		return -2;
	}

	return set_pitch_channel_parameters(channel_index, channel_controller_waveform, dutycycle_critical_phase,
									  envelope_attack_curve, envelope_attack_duration_in_seconds,
									  envelope_decay_curve, envelope_decay_duration_in_seconds,
									  envelope_sustain_level,
									  envelope_release_curve, envelope_release_duration_in_seconds,
									  envelope_damper_on_but_note_off_sustain_level,
									  envelope_damper_on_but_note_off_sustain_curve,
									  envelope_damper_on_but_note_off_sustain_duration_in_seconds);
}

/**********************************************************************************/

void chiptune_set_pitch_shift_in_semitones(int8_t pitch_shift_in_semitones)
{
	set_pitch_shift_in_semitones((int16_t)pitch_shift_in_semitones);
}
/**********************************************************************************/

int8_t chiptune_get_pitch_shift_in_semitones(void)
{
	return (int8_t)get_pitch_shift_in_semitones();
}

// NOLINTEND(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
