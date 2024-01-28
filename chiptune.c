#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include<stdarg.h>
#include <stdio.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_midi_control_change_internal.h"
#include "chiptune_event_internal.h"

#include "chiptune.h"

#ifdef _INCREMENTAL_SAMPLE_INDEX
typedef float chiptune_float;
#else
typedef double chiptune_float;
#endif

#define DEFAULT_TEMPO								(120.0)
#define DEFAULT_SAMPLING_RATE						(16000)
#define DEFAULT_RESOLUTION							(960)

static chiptune_float s_tempo = DEFAULT_TEMPO;
static uint32_t s_sampling_rate = DEFAULT_SAMPLING_RATE;
static uint32_t s_resolution = DEFAULT_RESOLUTION;

#ifdef _INCREMENTAL_SAMPLE_INDEX
static uint32_t s_current_sample_index = 0;
static chiptune_float s_tick_to_sample_index_ratio = (chiptune_float)(DEFAULT_SAMPLING_RATE * 1.0/(DEFAULT_TEMPO/60.0)/DEFAULT_RESOLUTION);
#define UPDATE_SAMPLES_TO_TICK_RATIO()				\
													do { \
														s_tick_to_sample_index_ratio \
														= (chiptune_float)(s_sampling_rate * 60.0/s_tempo/s_resolution); \
													} while(0)

#define CORRECT_TIME_BASE()							do { \
														s_current_sample_index = (uint32_t)(s_current_sample_index * s_tempo/tempo); \
													} while(0)

#define UPDATE_TIME_BASE_UNIT()						UPDATE_SAMPLES_TO_TICK_RATIO()

#define TICK_TO_SAMPLE_INDEX(TICK)					((uint32_t)(s_tick_to_sample_index_ratio * (chiptune_float)(TICK) + 0.5 ))

#define	IS_AFTER_CURRENT_TIME(TICK)					( (TICK_TO_SAMPLE_INDEX(TICK) > s_current_sample_index) ? true : false)
#else
static chiptune_float s_current_tick = 0.0;
static chiptune_float s_delta_tick_per_sample = (DEFAULT_RESOLUTION / ( (chiptune_float)DEFAULT_SAMPLING_RATE/(DEFAULT_TEMPO /60.0) ) );

#define	UPDATE_DELTA_TICK_PER_SAMPLE()				\
													do { \
														s_delta_tick_per_sample = ( s_resolution * s_tempo / (chiptune_float)s_sampling_rate/ 60.0 ); \
													} while(0)

#define CORRECT_TIME_BASE()							do { \
														(void)0; \
													} while(0)

#define UPDATE_TIME_BASE_UNIT()						UPDATE_DELTA_TICK_PER_SAMPLE()

#define	IS_AFTER_CURRENT_TIME(TICK)					(((chiptune_float)(TICK) > s_current_tick) ? true : false)
#endif

#define EACH_CHORUS_OSCILLAOTER_TIME_INTERVAL_IN_SECOND				(0.008)

uint32_t s_chorus_delta_tick = (uint32_t)(EACH_CHORUS_OSCILLAOTER_TIME_INTERVAL_IN_SECOND * DEFAULT_TEMPO * DEFAULT_RESOLUTION / (60.0) + 0.5);

#define	UPDATE_CHORUS_DELTA_TICK()					\
													do { \
														 s_chorus_delta_tick \
															= (uint32_t)(EACH_CHORUS_OSCILLAOTER_TIME_INTERVAL_IN_SECOND * s_tempo * s_resolution/ 60.0 + 0.5); \
													} while(0);


static int(*s_handler_get_midi_message)(uint32_t index, uint32_t * const p_tick, uint32_t * const p_message) = NULL;

static bool s_is_tune_ending = false;

/**********************************************************************************/

void chiptune_set_midi_message_callback( int(*handler_get_midi_message)(uint32_t index, uint32_t * const p_tick, uint32_t * const p_message) )
{
	s_handler_get_midi_message = handler_get_midi_message;
}

/**********************************************************************************/

static struct _channel_controller s_channel_controllers[MAX_VOICE_NUMBER];

static struct _oscillator s_oscillators[MAX_OSCILLATOR_NUMBER];
static uint32_t s_occupied_oscillator_number = 0;

void discard_oscillator(int16_t index)
{
	s_oscillators[index].voice = UNUSED_OSCILLATOR;
	s_occupied_oscillator_number -= 1;
}

/**********************************************************************************/

static void process_program_change_message(uint32_t const tick, uint8_t const voice, uint8_t const number)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_PROGRAM_CHANGE :: ", tick);
#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0		(9)
#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1		(10)
	do
	{
		if(false == (MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0 == voice || MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1 == voice)){
			break;
		}
		s_channel_controllers[voice].waveform = WAVEFORM_NOISE;
	}while(0);

	switch(s_channel_controllers[voice].waveform)
	{
	case WAVEFORM_SQUARE:
		CHIPTUNE_PRINTF(cMidiSetup, "%voice = %u instrument = %u, is WAVEFORM_SQUARE\r\n", voice, number);
		break;
	case WAVEFORM_TRIANGLE:
		CHIPTUNE_PRINTF(cMidiSetup, "%voice = %u instrument = %u, is WAVEFORM_TRIANGLE\r\n", voice, number);
		break;
	case WAVEFORM_SAW:
		CHIPTUNE_PRINTF(cMidiSetup, "%voice = %u instrument = %u, is WAVEFORM_SAW\r\n", voice, number);
		break;
	case WAVEFORM_NOISE:
		CHIPTUNE_PRINTF(cMidiSetup, "%voice = %u instrument = %u, is WAVEFORM_NOISE\r\n", voice, number);
		break;
	default:
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: tick = %u, MIDI_MESSAGE_PROGRAM_CHANGE :: "
									 " %voice = %u instrument = %u, is WAVEFORM_UNKOWN\r\n", voice, number);
	}
}

/**********************************************************************************/


#define DIVIDE_BY_2(VALUE)							((VALUE) >> 1)

static  uint16_t calculate_delta_phase(uint8_t const note, int8_t tuning_in_semitones,
											 uint16_t const pitch_wheel_bend_range_in_semitones, uint16_t const pitch_wheel,
											float pitch_chorus_bend_in_semitones, float *p_pitch_wheel_bend_in_semitone)
{
	// TO DO : too many float variable
	float pitch_wheel_bend_in_semitone = (((int16_t)pitch_wheel - MIDI_PITCH_WHEEL_CENTER)/(float)MIDI_PITCH_WHEEL_CENTER) * DIVIDE_BY_2(pitch_wheel_bend_range_in_semitones);
	float corrected_note = (float)((int16_t)note + (int16_t)tuning_in_semitones) + pitch_wheel_bend_in_semitone + pitch_chorus_bend_in_semitones;
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

#include <stdlib.h>

#define RAMDON_RANGE_TO_PLUS_MINUS_ONE(VALUE)	\
												(((DIVIDE_BY_2(RAND_MAX) + 1)- (VALUE))/(float)(DIVIDE_BY_2(RAND_MAX) + 1))

static float pitch_chorus_bend_in_semitone(uint8_t const voice)
{
	if(0 == s_channel_controllers[voice].chorus){
		return 0.0;
	}

	//TODO :: too complex
	int random = rand();
	float pitch_chorus_bend_in_semitone;
#define	MAX_CHORUS_PITCH_BEND_IN_SEMITONE			(0.25f)
	pitch_chorus_bend_in_semitone = RAMDON_RANGE_TO_PLUS_MINUS_ONE(random) * MAX_CHORUS_PITCH_BEND_IN_SEMITONE;
	pitch_chorus_bend_in_semitone *= s_channel_controllers[voice].chorus/127.0f;
	//CHIPTUNE_PRINTF(cDeveloping, "pitch_chorus_bend_in_semitone = %3.2f\r\n", pitch_chorus_bend_in_semitone);
	return pitch_chorus_bend_in_semitone;
}

/**********************************************************************************/

#define DIVIDE_BY_8(VALUE)							((VALUE) >> 3)
#define REDUCE_VOOLUME_AS_DAMPING_PEDAL_ON_BUT_NOTE_OFF(VALUE)	\
													DIVIDE_BY_8(VALUE)

#define	VIBRATION_AMPLITUDE_IN_SEMITINE				(1)

#define DIVIDE_BY_16(VALUE)							((VALUE) >> 4)
#define OSCILLATOR_NUMBER_FOR_CHORUS(VALUE)			(DIVIDE_BY_16(((VALUE) + 15)))

int process_chorus_effect(uint32_t const tick, bool const is_note_on,
						   uint8_t const voice, uint8_t const note, uint8_t const velocity,
						   int const original_oscillator_index)
{
	(void)velocity;
	if(0 == s_channel_controllers[voice].chorus){
		return 1;
	}

#define CHORUS_OSCILLATOR_NUMBER					(4)
	int oscillator_indexes[CHORUS_OSCILLATOR_NUMBER - 1] = {UNUSED_OSCILLATOR, UNUSED_OSCILLATOR, UNUSED_OSCILLATOR};
	do {
		if(true == is_note_on){

			if(MAX_OSCILLATOR_NUMBER < s_occupied_oscillator_number + (CHORUS_OSCILLATOR_NUMBER - 1)){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR::available oscillators is not enough for chorus effect\r\n");
				return -1;
			}

			const uint16_t volume = s_oscillators[original_oscillator_index].volume;
			uint16_t averaged_volume = DIVIDE_BY_16(volume);
			// oscillator 1 : 4 * averaged_volume
			// oscillator 2 : 5 * averaged_volume
			// oscillator 3 : 6 * averaged_volume
			// oscillator 0 : volume - (4 + 5  + 6) * averaged_volume
			uint16_t oscillator_volume = volume - (4 + 5 + 6) * averaged_volume;
			s_oscillators[original_oscillator_index].volume = oscillator_volume;

			float pitch_wheel_bend_in_semitone = 0.0f;
			int kk = 0;
			oscillator_volume = 4 * averaged_volume;
			int i;
			for(i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
				 if(UNUSED_OSCILLATOR != s_oscillators[i].voice){
					 continue;
				 }

				 memcpy(&s_oscillators[i], &s_oscillators[original_oscillator_index], sizeof(struct _oscillator));
				 s_oscillators[i].volume = oscillator_volume;
				 s_oscillators[i].pitch_chorus_bend_in_semitone = pitch_chorus_bend_in_semitone(voice);
				 s_oscillators[i].delta_phase = calculate_delta_phase(s_oscillators[i].note, s_channel_controllers[voice].tuning_in_semitones,
																	 s_channel_controllers[voice].pitch_wheel_bend_range_in_semitones,
																	 s_channel_controllers[voice].pitch_wheel,
																	   s_oscillators[i].pitch_chorus_bend_in_semitone,
																	   &pitch_wheel_bend_in_semitone);
				 s_oscillators[i].delta_vibration_phase = calculate_delta_phase(s_oscillators[i].note + VIBRATION_AMPLITUDE_IN_SEMITINE,
																				s_channel_controllers[voice].tuning_in_semitones,
																			s_channel_controllers[voice].pitch_wheel_bend_range_in_semitones,
																			s_channel_controllers[voice].pitch_wheel,
																				s_oscillators[i].pitch_chorus_bend_in_semitone,
																				&pitch_wheel_bend_in_semitone) - s_oscillators[i].delta_phase;
				 s_oscillators[i].native_oscillator = original_oscillator_index;
				 SET_CHORUS_OSCILLATOR(s_oscillators[i].state_bits);
				 SET_ACTIVATED_OFF(s_oscillators[i].state_bits);

				 oscillator_indexes[kk] = i;

				 oscillator_volume += averaged_volume;
				 kk += 1;
				 if((CHORUS_OSCILLATOR_NUMBER - 1) == kk){
					 break;
				 }
			}
			if(MAX_OSCILLATOR_NUMBER == i){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR::available oscillator could not be found\r\n");
				return -3;
			}
			s_occupied_oscillator_number += (CHORUS_OSCILLATOR_NUMBER - 1);
			break;
		}

		int kk = 0;
		int ii;
		for(ii = 0; ii < MAX_OSCILLATOR_NUMBER; ii++){
			if(original_oscillator_index != s_oscillators[ii].native_oscillator){
				continue;
			}
			if(voice == s_oscillators[ii].voice){
				if(note == s_oscillators[ii].note){
					if(true == IS_CHORUS_OSCILLATOR(s_oscillators[ii].state_bits)){
						oscillator_indexes[kk] = ii;
						kk += 1;
						if(CHORUS_OSCILLATOR_NUMBER - 1 == kk){
							break;
						}
					}
				}
			}
		}
		if(MAX_OSCILLATOR_NUMBER == ii){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR::targeted oscillator could not be found\r\n");
			return -4;
		}
	} while(0);


	uint8_t event_type = (true == is_note_on) ? ACTIVATE_EVENT : RELEASE_EVENT;
	for(int j = 0; j  < CHORUS_OSCILLATOR_NUMBER - 1; j++){
		put_event(event_type, oscillator_indexes[j], tick + (j + 1) * s_chorus_delta_tick);
	}

	return 0;
}

/**********************************************************************************/

static int process_note_message(uint32_t const tick, bool const is_note_on,
						 uint8_t const voice, uint8_t const note, uint8_t const velocity)
{
	float pitch_wheel_bend_in_semitone = 0.0f;

	int ii = 0;
	do {
		if(true == is_note_on){

			if(MAX_OSCILLATOR_NUMBER == s_occupied_oscillator_number){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR::all oscillators are used\r\n");
				return -1;
			}

			for(ii = 0; ii < MAX_OSCILLATOR_NUMBER; ii++){
				 if(UNUSED_OSCILLATOR == s_oscillators[ii].voice){
					 break;
				 }
			}

			RESET_STATE_BITES(s_oscillators[ii].state_bits);
			SET_NOTE_ON(s_oscillators[ii].state_bits);
			s_oscillators[ii].voice = voice;
			s_oscillators[ii].note = note;
			s_oscillators[ii].pitch_chorus_bend_in_semitone = 0;
			s_oscillators[ii].delta_phase = calculate_delta_phase(s_oscillators[ii].note, s_channel_controllers[voice].tuning_in_semitones,
															   s_channel_controllers[voice].pitch_wheel_bend_range_in_semitones,
															   s_channel_controllers[voice].pitch_wheel,
																 s_oscillators[ii].pitch_chorus_bend_in_semitone,
																 &pitch_wheel_bend_in_semitone);
			s_oscillators[ii].current_phase = 0;
			s_oscillators[ii].volume = (uint16_t)velocity * (uint16_t)s_channel_controllers[voice].playing_volume;
			s_oscillators[ii].waveform = s_channel_controllers[voice].waveform;
			s_oscillators[ii].duty_cycle_critical_phase = s_channel_controllers[voice].duty_cycle_critical_phase;
			s_oscillators[ii].delta_vibration_phase = calculate_delta_phase(s_oscillators[ii].note + VIBRATION_AMPLITUDE_IN_SEMITINE,
																		   s_channel_controllers[voice].tuning_in_semitones,
																	   s_channel_controllers[voice].pitch_wheel_bend_range_in_semitones,
																	   s_channel_controllers[voice].pitch_wheel,
																		   s_oscillators[ii].pitch_chorus_bend_in_semitone,
																		   &pitch_wheel_bend_in_semitone);
			s_oscillators[ii].delta_vibration_phase -= s_oscillators[ii].delta_phase;
			s_oscillators[ii].vibration_table_index = 0;
			s_oscillators[ii].vibration_same_index_count = 0;
			s_oscillators[ii].native_oscillator = UNUSED_OSCILLATOR;
			SET_ACTIVATED_ON(s_oscillators[ii].state_bits);
			s_occupied_oscillator_number += 1;
			if(0 != s_channel_controllers[voice].chorus){
				process_chorus_effect(tick, is_note_on, voice, note, velocity, ii);
			}
			break;
		}

		bool is_found = false;
		do {
			if(true == s_channel_controllers[voice].is_damper_pedal_on){
				for(ii = 0; ii < MAX_OSCILLATOR_NUMBER; ii++){
					if(voice == s_oscillators[ii].voice){
						if(note == s_oscillators[ii].note){
							SET_NOTE_OFF(s_oscillators[ii].state_bits);
							s_oscillators[ii].volume
									= REDUCE_VOOLUME_AS_DAMPING_PEDAL_ON_BUT_NOTE_OFF(s_oscillators[ii].volume);
							is_found = true;
						}
					}
				}
				break;
			}

			for(ii = 0; ii < MAX_OSCILLATOR_NUMBER; ii++){
				if(voice == s_oscillators[ii].voice){
					if(note == s_oscillators[ii].note){
						if(UNUSED_OSCILLATOR == s_oscillators[ii].native_oscillator){
							if(0 < s_channel_controllers[voice].chorus){
								process_chorus_effect(tick, is_note_on, voice, note, velocity, ii);
							}
							discard_oscillator(ii);
							is_found = true;
							break;
						}
					}
				}
			}
		} while(0);

		if(false == is_found){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR::no corresponding note for off :: tick = %u, voice = %u,  note = %u\r\n",
							tick, voice, note);
			return -2;
		}
	} while(0);

	char pitch_wheel_bend_string[32] = "";
	if(true == is_note_on && 0.0f != pitch_wheel_bend_in_semitone){
		snprintf(&pitch_wheel_bend_string[0], sizeof(pitch_wheel_bend_string),
			", pitch wheel bend = %+3.2f", pitch_wheel_bend_in_semitone);
	}

	CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %u, note = %u, velocity = %u\r\n",
					tick, is_note_on ? "MIDI_MESSAGE_NOTE_ON" : "MIDI_MESSAGE_NOTE_OFF" ,
					voice, note, velocity, s_oscillators[ii].volume/s_channel_controllers[voice].playing_volume,
					&pitch_wheel_bend_string[0]);


#ifdef _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING
	#ifdef _INCREMENTAL_SAMPLE_INDEX
	if(TICK_TO_SAMPLE_INDEX(818880) < s_current_sample_index){
		CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %u, note = %u, velocity = %u\r\n",
						tick, is_note_on ? "MIDI_MESSAGE_NOTE_ON" : "MIDI_MESSAGE_NOTE_OFF" , voice, note, velocity);
	#else
	if(818880 < s_current_tick){
		CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %u, note = %u, velocity = %u\r\n",
						tick, is_note_on ? "MIDI_MESSAGE_NOTE_ON" : "MIDI_MESSAGE_NOTE_OFF" , voice, note, velocity);
	}
	#endif
#endif

	return 0;
}

/**********************************************************************************/

static void process_pitch_wheel_message(uint32_t const tick, uint8_t const voice, uint16_t const value)
{
	CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, MIDI_MESSAGE_PITCH_WHEEL :: voice = %u, value = %u\r\n",
					tick, voice, value);
	s_channel_controllers[voice].pitch_wheel = value;
	do {
		if( 0 == s_occupied_oscillator_number){
			break;
		}

		for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
			if(voice != s_oscillators[i].voice){
				continue;
			}
			float pitch_bend_in_semitone;
			s_oscillators[i].delta_phase = calculate_delta_phase(s_oscillators[i].note, s_channel_controllers[voice].tuning_in_semitones,
															   s_channel_controllers[voice].pitch_wheel_bend_range_in_semitones,
															   s_channel_controllers[voice].pitch_wheel, s_oscillators[i].pitch_chorus_bend_in_semitone, &pitch_bend_in_semitone);

			CHIPTUNE_PRINTF(cNoteOperation, "---- voice = %u, note = %u, pitch bend = %+3.2f\r\n",voice, s_oscillators[i].note, pitch_bend_in_semitone);
		}
	} while(0);
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

	uint8_t type =  u.data_as_bytes[0] & 0xF0;
	uint8_t voice = u.data_as_bytes[0] & 0x0F;

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
			voice, u.data_as_bytes[1], u.data_as_bytes[2]);
	 break;
	case MIDI_MESSAGE_KEY_PRESSURE:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_CHANNEL_PRESSURE :: note = %u, amount = %u %s\r\n",
						tick, voice, u.data_as_bytes[1], "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_MESSAGE_CONTROL_CHANGE:
		process_control_change_message(&s_channel_controllers[0], &s_oscillators[0],
				tick, voice, u.data_as_bytes[1], u.data_as_bytes[2]);
		break;
	case MIDI_MESSAGE_PROGRAM_CHANGE:
		process_program_change_message(tick, voice, u.data_as_bytes[1]);
		break;
	case MIDI_MESSAGE_CHANNEL_PRESSURE:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_CHANNEL_PRESSURE :: voice = %u, amount = %u %s\r\n",
						tick, voice, u.data_as_bytes[1], "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_MESSAGE_PITCH_WHEEL:
#define COMBINE_AS_PITCH_WHEEL_14BITS(BYTE1, BYTE2)	\
													((0x7F & (BYTE2)) << 7) | (0x7F & (BYTE1))
		process_pitch_wheel_message(tick, voice, COMBINE_AS_PITCH_WHEEL_14BITS(u.data_as_bytes[1], u.data_as_bytes[2]) );
		break;
	default:
		//CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE code = %u :: voice = %u, byte 1 = %u, byte 2 = %u %s\r\n",
		//				tick, type, voice, u.data_as_bytes[1], u.data_as_bytes[2], "(NOT IMPLEMENTED YET)");
		break;
	}
}

/**********************************************************************************/

uint32_t s_midi_messge_index = 0;
uint32_t s_total_message_number = 0;

int fetch_midi_tick_message(uint32_t index, struct _tick_message *p_tick_message)
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
	}while(0);

	return ret;
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
			ret = -1;
			break;
		}

		bool is_both_after_current_tick = true;

		if(false == IS_AFTER_CURRENT_TIME(s_fetched_tick_message.tick)){
			process_midi_message(s_fetched_tick_message);
			SET_TICK_MESSAGE_NULL(s_fetched_tick_message);
			is_both_after_current_tick = false;

			s_fetched_event_tick = get_next_event_triggering_tick();
		}

		do {
			if(NULL_TICK == s_fetched_event_tick){
				break;
			}
			if(false == IS_AFTER_CURRENT_TIME(s_fetched_event_tick)){
				process_events(s_fetched_event_tick, &s_oscillators[0]);
				s_fetched_event_tick = NULL_TICK;
				is_both_after_current_tick = false;
			}
		} while(0);

		if(true == is_both_after_current_tick){
			break;
		}
	}
	return ret;
}

/**********************************************************************************/

static uint32_t get_max_simultaneous_amplitude(void)
{
	SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(false);
	uint32_t midi_messge_index = 0;
	uint32_t max_amplitude = 0;
	uint32_t previous_tick;

	struct _tick_message tick_message;
	uint32_t event_tick = NULL_TICK;

	fetch_midi_tick_message(midi_messge_index, &tick_message);
	midi_messge_index += 1;
	process_midi_message(tick_message);
	previous_tick = tick_message.tick;
	SET_TICK_MESSAGE_NULL(tick_message);

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
			break;
		}

		int const tick = (tick_message.tick < event_tick) ? tick_message.tick : event_tick;

		do
		{
			if(tick == previous_tick){
				break;
			}
			previous_tick = tick;

			uint32_t sum_amplitude = 0;
			for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
				if(UNUSED_OSCILLATOR == s_oscillators[i].voice){
					continue;
				}
				sum_amplitude += s_oscillators[i].volume;
			}
			if(sum_amplitude > max_amplitude){
				max_amplitude = sum_amplitude;
			}
		}while(0);

		if(tick == tick_message.tick){
			process_midi_message(tick_message);
			SET_TICK_MESSAGE_NULL(tick_message);

			event_tick = get_next_event_triggering_tick();
		}

		if(tick == event_tick){
			process_events(event_tick, &s_oscillators[0]);
			event_tick = NULL_TICK;
		}
	}

	SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(true);
	if(0 != s_occupied_oscillator_number){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: not all oscillators are released\r\n");
		for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
			if(UNUSED_OSCILLATOR == s_oscillators[i].voice){
				continue;
			}

			CHIPTUNE_PRINTF(cDeveloping, "oscillators = %u, voice = %u, note = 0x%02x(%u)\r\n",
							i, s_oscillators[i].voice, s_oscillators[i].note, s_oscillators[i].note);
		}
	}
	if(0 != get_upcoming_event_number()){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: not all events are released\r\n");
	}
	return max_amplitude;
}

#ifdef _RIGHT_SHIFT_FOR_NORMALIZING_AMPLITUDE

/**********************************************************************************/

uint32_t number_of_roundup_to_power2_left_shift_bits(uint32_t const value)
{
	unsigned int v = value; // compute the next highest power of 2 of 32-bit v

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

uint32_t g_amplitude_nomalization_right_shift = 16;
#define UPDATE_AMPLITUDE_NORMALIZER()				\
													do { \
														uint32_t max_amplitude = get_max_simultaneous_amplitude(); \
														g_amplitude_nomalization_right_shift \
															= number_of_roundup_to_power2_left_shift_bits(max_amplitude);\
													} while(0)
#else
uint32_t g_max_amplitude = 1 << 16;
#define UPDATE_AMPLITUDE_NORMALIZER()				\
													do { \
														g_max_amplitude = get_max_simultaneous_amplitude(); \
													} while(0)
#endif

/**********************************************************************************/

#define VIBRATION_PHASE_TABLE_LENGTH				(64)
static int8_t s_vibration_phase_table[VIBRATION_PHASE_TABLE_LENGTH] = {0};
#define CALCULATE_VIBRATION_TABLE_INDEX_REMAINDER(INDEX) \
													((INDEX) & (VIBRATION_PHASE_TABLE_LENGTH - 1))

#define VIBRATION_FREQUENCY							(4.0)
static uint32_t  s_vibration_same_index_count_number = (uint32_t)(DEFAULT_SAMPLING_RATE/VIBRATION_PHASE_TABLE_LENGTH/(float)VIBRATION_FREQUENCY);

void chiptune_initialize(uint32_t const sampling_rate, uint32_t const resolution, uint32_t const total_message_number)
{
#ifdef _INCREMENTAL_SAMPLE_INDEX
	s_current_sample_index = 0;
#else
	s_current_tick = 0;
#endif
	s_is_tune_ending = false;
	s_midi_messge_index = 0;
	SET_TICK_MESSAGE_NULL(s_fetched_tick_message);

	for(int i = 0; i < MAX_VOICE_NUMBER; i++){
		reset_channel_controller(&s_channel_controllers[i]);
	}

	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		s_oscillators[i].voice = UNUSED_OSCILLATOR;
	}
	s_occupied_oscillator_number = 0;

	clean_all_events();

	s_sampling_rate = sampling_rate;
	s_resolution = resolution;
	s_total_message_number = total_message_number;
	for(int i = 0; i < VIBRATION_PHASE_TABLE_LENGTH; i++){
		s_vibration_phase_table[i] = (int8_t)(INT8_MAX * sinf( 2.0f * (float)M_PI * i / (float)VIBRATION_PHASE_TABLE_LENGTH));
	}
	s_vibration_same_index_count_number = (uint32_t)(s_sampling_rate/(VIBRATION_PHASE_TABLE_LENGTH * (float)VIBRATION_FREQUENCY));

	UPDATE_TIME_BASE_UNIT();
	UPDATE_AMPLITUDE_NORMALIZER();
	process_timely_midi_message();
	return ;
}

/**********************************************************************************/


void chiptune_set_tempo(float const tempo)
{
	CHIPTUNE_PRINTF(cMidiSetup, "%s :: tempo = %3.1f\r\n", __FUNCTION__,tempo);
	CORRECT_TIME_BASE();
	s_tempo = (chiptune_float)tempo;
	UPDATE_TIME_BASE_UNIT();
	UPDATE_CHORUS_DELTA_TICK();
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

#ifdef _RIGHT_SHIFT_FOR_NORMALIZING_AMPLITUDE
#define NORMALIZE_AMPLITUDE(VALUE)					((int32_t)((VALUE) >> g_amplitude_nomalization_right_shift))
#else
#define NORMALIZE_AMPLITUDE(VALUE)					((int32_t)((VALUE)/(int32_t)g_max_amplitude))
#endif

#define INT16_MAX_PLUS_1							(INT16_MAX + 1)
#define MULTIPLY_BY_2(VALUE)						((VALUE) << 1)

#define DIVIDE_BY_128(VALUE)						((VALUE) >> 7)
#define NORMALIZE_VIBRAION_DELTA_PHASE_AMPLITUDE(VALUE)	\
													DIVIDE_BY_128(DIVIDE_BY_128(VALUE))
#define REGULATE_MODULATION_WHEEL(VALUE)			(VALUE + 1)

int16_t chiptune_fetch_16bit_wave(void)
{
	if(-1 == process_timely_midi_message()){
		if(0 == s_occupied_oscillator_number){
			s_is_tune_ending = true;
		}
	}

	uint32_t kk = 0;
	int64_t accumulated_value = 0;
	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		if(UNUSED_OSCILLATOR == s_oscillators[i].voice){
			continue;
		}

		if(false == IS_ACTIVATED(s_oscillators[i].state_bits)){
			goto Flag_oscillator_take_effect_end;
		}

		int16_t value = 0;
		do {
			if(0 ==  s_channel_controllers[s_oscillators[i].voice].modulation_wheel){
				break;
			}

			uint16_t delta_vibration_phase = s_oscillators[i].delta_vibration_phase;
			uint32_t vibration_amplitude
					= REGULATE_MODULATION_WHEEL(s_channel_controllers[s_oscillators[i].voice].modulation_wheel)
						* s_vibration_phase_table[s_oscillators[i].vibration_table_index];

			delta_vibration_phase = NORMALIZE_VIBRAION_DELTA_PHASE_AMPLITUDE(vibration_amplitude * delta_vibration_phase);
			s_oscillators[i].current_phase += delta_vibration_phase;

			s_oscillators[i].vibration_same_index_count += 1;
			if(s_vibration_same_index_count_number == s_oscillators[i].vibration_same_index_count){
				s_oscillators[i].vibration_same_index_count = 0;
				s_oscillators[i].vibration_table_index = (s_oscillators[i].vibration_table_index  + 1) % VIBRATION_PHASE_TABLE_LENGTH;
			}
		} while(0);

		switch(s_oscillators[i].waveform)
		{
		case WAVEFORM_SQUARE:
			value = (s_oscillators[i].current_phase > s_oscillators[i].duty_cycle_critical_phase) ? -INT16_MAX_PLUS_1 : INT16_MAX;
			break;
		case WAVEFORM_TRIANGLE:
			do {
				if(s_oscillators[i].current_phase < INT16_MAX_PLUS_1){
					value = -INT16_MAX_PLUS_1 + MULTIPLY_BY_2(s_oscillators[i].current_phase);
					break;
				}
				value = INT16_MAX - MULTIPLY_BY_2(s_oscillators[i].current_phase - INT16_MAX_PLUS_1);
			} while(0);
			break;
		case WAVEFORM_SAW:
			value =  -INT16_MAX_PLUS_1 + s_oscillators[i].current_phase;
			break;
		case WAVEFORM_NOISE:
			break;
		default:
			break;
		}
		accumulated_value += (value * s_oscillators[i].volume);

		s_oscillators[i].current_phase += s_oscillators[i].delta_phase;

Flag_oscillator_take_effect_end:
		kk += 1;
		if(kk == s_occupied_oscillator_number){
			break;
		}
	}

	INCREMENT_TIME_BASE();
#ifdef _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING
	increase_time_base_for_fast_to_ending();
#endif

	int32_t out_value = NORMALIZE_AMPLITUDE(accumulated_value);
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
