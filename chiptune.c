#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <stdio.h>

#include "chiptune.h"

#define DEFAULT_TEMPO								(120.0)
#define DEFAULT_SAMPLING_RATE						(16000)
#define DEFAULT_RESOLUTION							(960)

uint16_t s_phase_table[MIDI_FREQUENCY_TABLE_SIZE] = {0};

static float s_tempo = DEFAULT_TEMPO;
static uint32_t s_sampling_rate = DEFAULT_SAMPLING_RATE;

static uint32_t s_resolution = DEFAULT_RESOLUTION;
static float s_time_tick = 0.0f;
static float s_delta_tick = (float)(DEFAULT_SAMPLING_RATE * 60.0/DEFAULT_TEMPO/DEFAULT_RESOLUTION);

#define UPDATE_DELTA_TICK()				do{			\
											s_delta_tick = (float)(1.0/(s_sampling_rate * 60.0/s_tempo/(float)s_resolution)); \
										} while(0)

static int(*s_handler_get_next_midi_message)(uint32_t * const p_message, uint32_t * const p_tick) = NULL;
static void(*s_handler_tune_ending_notification)(void) = NULL;

/**********************************************************************************/

void chiptune_set_midi_message_callback( int(*handler_get_next_midi_message)(uint32_t * const p_message, uint32_t * const p_tick) )
{
	s_handler_get_next_midi_message = handler_get_next_midi_message;
}


/**********************************************************************************/
void chiptune_set_tune_ending_notfication_callback( void(*handler_ending_notification)(void))
{
	s_handler_tune_ending_notification = handler_ending_notification;
}

/**********************************************************************************/

enum
{
	WAVEFORM_SILENCE		= -1,
	WAVEFORM_SQUARE			= 0,
	WAVEFORM_TRIANGLE		= 1,
	WAVEFORM_SAW			= 2,
	WAVEFORM_NOISE			= 3,
};

#define MAX_TRACK_NUMBER							(16)
#define MAX_OSCILLATOR_NUMBER						(MAX_TRACK_NUMBER * 2)

struct _oscillator
{
	int8_t		track_index;
	uint8_t		volume;
	uint8_t		waveform;
	uint8_t		note;
	uint16_t	phase;
	uint16_t	duty;
	uint32_t	start_tick;
	uint32_t	end_tick;
	bool		is_completed;
} s_oscillator[MAX_OSCILLATOR_NUMBER];

#define UNSED_OSCILLATOR							(-1)

struct _track_info
{
	uint8_t		waveform;
	uint16_t	duty;
	uint8_t		volume;
	uint8_t		pan;
}s_track_info[MAX_OSCILLATOR_NUMBER];

#define MIDI_MESSAGE_NOTE_OFF						(0x80)
#define MIDI_MESSAGE_NOTE_ON						(0x90)
#define MIDI_MESSAGE_KEY_PRESSURE					(0xA0)
#define MIDI_MESSAGE_CONTROL_CHANGE					(0xB0)
#define MIDI_MESSAGE_PROGRAM_CHANGE					(0xC0)
#define MIDI_MESSAGE_CHANNEL_PRESSURE				(0xD0)
#define MIDI_MESSAGE_PITCH_WHEEL					(0xE0)

//https://anotherproducer.com/online-tools-for-musicians/midi-cc-list/
#define MIDI_CC_DATA_ENTRY_MSB						(6)
#define MIDI_CC_VOLUME								(7)
#define MIDI_CC_PAN									(10)
#define MIDI_CC_EXPRESSION							(11)

#define MIDI_CC_DATA_ENTRY_LSB						(32 + MIDI_CC_DATA_ENTRY_MSB)

#define MIDI_CC_EFFECT_1_DEPTH						(91)
#define MIDI_CC_EFFECT_2_DEPTH						(92)
#define MIDI_CC_EFFECT_3_DEPTH						(93)
#define MIDI_CC_EFFECT_4_DEPTH						(94)
#define MIDI_CC_EFFECT_5_DEPTH						(95)

//https://zh.wikipedia.org/zh-tw/General_MIDI
#define MIDI_CC_RON_LSB								(100)
#define MIDI_CC_RON_MSB								(101)

static int setup_control_change_into_track_info(uint8_t const voice, uint8_t const number, uint8_t const value)
{
	switch(number){
	case MIDI_CC_DATA_ENTRY_MSB:
		break;
	case MIDI_CC_VOLUME:
		do
		{
			if(0 == number){
				for(int i = 0; i < MAX_TRACK_NUMBER; i++){
					s_track_info[i].volume = value;
				}
				break;
			}
			s_track_info[voice].volume = value;
		}while(0);
		break;
	case MIDI_CC_PAN:
		do
		{
			if(0 == number){
				for(int i = 0; i < MAX_TRACK_NUMBER; i++){
					s_track_info[i].pan = value;
				}
				break;
			}
			s_track_info[voice].pan = value;
		}while(0);
		break;
	case MIDI_CC_EXPRESSION:
	case MIDI_CC_DATA_ENTRY_LSB:
	case MIDI_CC_EFFECT_1_DEPTH:
	case MIDI_CC_EFFECT_2_DEPTH:
	case MIDI_CC_EFFECT_3_DEPTH:
	case MIDI_CC_EFFECT_4_DEPTH:
	case MIDI_CC_EFFECT_5_DEPTH:
		break;
	default:
		return 0;
		break;
	}

	return 0;
}

/**********************************************************************************/

static int setup_program_change_into_track_info(uint8_t const voice, uint8_t const number)
{
	do
	{
		if(0 == number){
			for(int i = 0; i < MAX_TRACK_NUMBER; i++){
				s_track_info[voice].waveform = WAVEFORM_TRIANGLE;
			}
			break;
		}
		s_track_info[voice].waveform = WAVEFORM_TRIANGLE;
	}while(0);

	return 0;
}

/**********************************************************************************/

static bool is_all_oscillators_completed(void)
{
	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		if(UNSED_OSCILLATOR == s_oscillator[i].track_index){
			continue;
		}
		if(false == s_oscillator[i].is_completed){
			//printf("oscillator %d, not completed, track = %u, note = %u\r\n", i, s_oscillator[i].track_index, s_oscillator[i].note);

			return false;
		}
	}

	return true;
}

/**********************************************************************************/

static bool is_all_oscillators_unused(void)
{
	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		if(UNSED_OSCILLATOR == s_oscillator[i].track_index){
			continue;
		}
		return false;
	}

	return true;
}

/**********************************************************************************/

static int process_note_message(uint32_t const tick, bool const is_note_on,
						 uint8_t const voice, uint8_t const note, uint8_t const velocity)
{

	//printf("track = %u, note %u %u\r\n", voice, note, is_note_on);
	do
	{
		int ii = 0;
		if(true == is_note_on){
			for(ii = 0; ii < MAX_OSCILLATOR_NUMBER; ii++){
				 if(UNSED_OSCILLATOR == s_oscillator[ii].track_index){
					 break;
				 }
			}
			//printf("ii = %u\r\n", ii);
			s_oscillator[ii].track_index = voice;
			s_oscillator[ii].waveform = s_track_info[voice].waveform;
			s_oscillator[ii].duty = s_track_info[voice].duty;
			s_oscillator[ii].volume = 2 * velocity;
			s_oscillator[ii].note = note;
			s_oscillator[ii].phase = 0;
			s_oscillator[ii].start_tick = tick;
			s_oscillator[ii].is_completed = false;
			break;
		}
		for(ii = 0; ii < MAX_OSCILLATOR_NUMBER; ii++){
			if(voice != s_oscillator[ii].track_index){
				continue;
			}
			if(note != s_oscillator[ii].note){
				continue;
			}

			if(true == s_oscillator[ii].is_completed){
				continue;
			}

			if(tick > s_oscillator[ii].start_tick){
				break;
			}
		}

		s_oscillator[ii].end_tick = tick;
		s_oscillator[ii].is_completed = true;
	}while(0);

	return 0;
}

/**********************************************************************************/

static void process_midi_message(uint32_t const message, uint32_t const tick, bool * const p_is_note_message)
{
	union {
		uint32_t data_as_uint32;
		unsigned char data_as_bytes[4];
	} u;

	u.data_as_uint32 = message;

	uint8_t type =  u.data_as_bytes[0]  & 0xF0;
	uint8_t voice = u.data_as_bytes[0] & 0x0F;

	*p_is_note_message = false;
	switch(type)
	{
	case MIDI_MESSAGE_NOTE_OFF:
	case MIDI_MESSAGE_NOTE_ON:
		*p_is_note_message = true;
		//p_midi_msg->note = u.data_as_bytes[1];
		//p_midi_msg->velocity = u.data_as_bytes[2];
		process_note_message(tick, (type == MIDI_MESSAGE_NOTE_OFF) ? false : true,
							 voice, u.data_as_bytes[1], u.data_as_bytes[2]);
		break;
	case MIDI_MESSAGE_KEY_PRESSURE:
		printf("MIDI_MESSAGE_KEY_PRESSURE\r\n");
		printf("voice = %u, ", voice);
		printf("note = %u, amount = %u\r\n",  u.data_as_bytes[1], u.data_as_bytes[2]);
		break;
	case MIDI_MESSAGE_CONTROL_CHANGE:
#if(0)
		printf("MIDI_MESSAGE_CONTROL_CHANGE\r\n");
		printf("voice = %u, ", voice);
		printf("number = %u, value = %u\r\n",  u.data_as_bytes[1], u.data_as_bytes[2]);
#endif
		setup_control_change_into_track_info(voice, u.data_as_bytes[1], u.data_as_bytes[2]);
		break;
	case MIDI_MESSAGE_PROGRAM_CHANGE:
#if(0)
		printf("MIDI_MESSAGE_PROGRAM_CHANGE\r\n");
		printf("voice = %u, ", voice);
		printf("number = %u\r\n",  u.data_as_bytes[1]);
		//p_midi_msg->number = u.data_as_bytes[1];
#endif
		setup_program_change_into_track_info(voice, u.data_as_bytes[1]);
		break;
	case MIDI_MESSAGE_CHANNEL_PRESSURE:
		printf("MIDI_MESSAGE_CHANNEL_PRESSURE\r\n");
		printf("voice = %u, ", voice);
		printf("amount = %u\r\n",  u.data_as_bytes[1]);
		break;
	case MIDI_MESSAGE_PITCH_WHEEL:
		printf("MIDI_MESSAGE_PITCH_WHEEL\r\n");
		printf("voice = %u, ", voice);
		printf("value = 0x%04X\r\n", (u.data_as_bytes[2] << 7) | u.data_as_bytes[1]);
		break;
	default:
		break;
	}

}

/**********************************************************************************/

inline static int process_next_midi_message(bool * const p_is_note_message)
{
	uint32_t message;
	uint32_t tick;
	do
	{
		if(-1 == s_handler_get_next_midi_message(&message, &tick)){
			return -1;
		}
		process_midi_message(message, tick, p_is_note_message);
	}while(0);

	return 0;
}

/**********************************************************************************/

void chiptune_initialize(uint32_t const sampling_rate)
{
	s_time_tick = 0.0;
	s_sampling_rate = sampling_rate;
	UPDATE_DELTA_TICK();

	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		s_oscillator[i].track_index = UNSED_OSCILLATOR;
	}

	for(int i = 0; i < MIDI_FREQUENCY_TABLE_SIZE; i++){
		//freq = 440 * 2**((n-69)/12)
#if(1)
		double frequency = 440.0 * pow(2.0, (double)(i- 69)/12.0);
		frequency = round(frequency * 100.0 + 0.5)/100.0;
#else
		float frequency = 440.0f * powf(2.0f, (double)(i- 69)/12.0f);
		frequency = roundf(frequency * 100.0f + 0.5f)/100.0f;
#endif
		// sampling_rate/frequency = samples_per_cycle  = (UINT16_MAX = + 1)/phase
		s_phase_table[i] = (uint16_t)((UINT16_MAX + 1) * frequency / sampling_rate);
		//printf("i = %d, freq = %f, phase = %04X\r\n", i, frequency, s_phase_table[i]);
	}

	bool is_note_message;
	do
	{
		process_next_midi_message(&is_note_message);
	}while(false == is_note_message);

	return ;
}

/**********************************************************************************/

void chiptune_set_tempo(float const tempo)
{
	s_tempo = tempo;
	UPDATE_DELTA_TICK();
}

/**********************************************************************************/

void chiptune_set_resolution(uint32_t const resolution)
{
	s_resolution = resolution;
	UPDATE_DELTA_TICK();
}

/**********************************************************************************/

uint8_t chiptune_fetch_wave(void)
{
	while(false == is_all_oscillators_completed()
		  || true == is_all_oscillators_unused())
	{
		bool is_note_message;
		if(0 != process_next_midi_message(&is_note_message)){
			if(NULL != s_handler_tune_ending_notification){
				s_handler_tune_ending_notification();
			}
			break;
		}
	}

	int16_t accumulated_value = 0;
	for(int i = 0; i < MAX_OSCILLATOR_NUMBER;i++){

		if(UNSED_OSCILLATOR == s_oscillator[i].track_index){
			continue;
		}

		if(s_time_tick > s_oscillator[i].end_tick){
			s_oscillator[i].track_index = UNSED_OSCILLATOR;
			continue;
		}

		if(s_time_tick < s_oscillator[i].start_tick){
			continue;
		}

		int8_t value = 0;
		switch(s_oscillator[i].waveform)
		{
		case WAVEFORM_SQUARE:
			value = (s_oscillator[i].phase > s_oscillator[i].duty) ? -32 : 31;
			break;
		case WAVEFORM_TRIANGLE:
			do
			{
				if(s_oscillator[i].phase < 0x8000){
					value = -32 + (s_oscillator[i].phase >> 9);
					break;
				}
				value = 31 -  ((s_oscillator[i].phase - 0x8000) >> 9);
			}while(0);

			break;
		default:
			break;
		}

		accumulated_value += value * s_oscillator[i].volume;
		s_oscillator[i].phase += s_phase_table[s_oscillator[i].note];
	}

	s_time_tick += s_delta_tick;
	return 128 + (accumulated_value >> 8);
}

