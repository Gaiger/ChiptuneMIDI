#include <string.h>
#include <math.h>

#include<stdarg.h>
#include <stdio.h>

#include "chiptune.h"

#define _PRINT_MIDI_DEVELOPING
#define _PRINT_MIDI_SETUP
#define _PRINT_MOTE_OPERATION

enum
{
	cDeveloping		= 0,
	cMidiSetup		= 1,
	cNoteOperation	= 2,
} PrintType;

void chiptune_printf(int const print_type, const char* fmt, ...)
{
	bool is_print_out = false;

#ifdef _PRINT_MIDI_DEVELOPING
	if(cDeveloping == print_type){
		is_print_out = true;
		//fprintf(stdout, "cDeveloping:: ");
	}
#endif

#ifdef _PRINT_MIDI_SETUP
	if(cMidiSetup == print_type){
		is_print_out = true;
		//fprintf(stdout, "cMidiSetup:: ");
	}
#endif

#ifdef _PRINT_MOTE_OPERATION
	if(cNoteOperation == print_type){
		is_print_out = true;
		//fprintf(stdout, "cNoteOperation:: ");
	}
#endif

	if (false == is_print_out){
			return;
	}

	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
}

#define CHIPTUNE_PRINTF(PRINT_TYPE, FMT, ...)				do { \
													chiptune_printf(PRINT_TYPE, FMT, ##__VA_ARGS__); \
												}while(0)

#if(0)
#define CHIPTUNE_PRINTF(PRINT_TYPE, FMT, ...)				do { \
													(void)0; \
												}while(0)
#endif

typedef double chiptune_float;

#define DEFAULT_TEMPO								(120.0)
#define DEFAULT_SAMPLING_RATE						(16000)
#define DEFAULT_RESOLUTION							(960)

uint16_t s_phase_table[MIDI_FREQUENCY_TABLE_SIZE] = {0};

static chiptune_float s_tempo = DEFAULT_TEMPO;
static uint32_t s_sampling_rate = DEFAULT_SAMPLING_RATE;

static uint32_t s_resolution = DEFAULT_RESOLUTION;
static chiptune_float s_time_tick = 0.0f;
static chiptune_float s_delta_tick = (double)(DEFAULT_SAMPLING_RATE * 60.0/DEFAULT_TEMPO/DEFAULT_RESOLUTION);

#define UPDATE_DELTA_TICK()				do{			\
											s_delta_tick = (chiptune_float)(s_tempo * s_resolution / (chiptune_float)s_sampling_rate/60.0); \
										} while(0)

static int(*s_handler_get_next_midi_message)(uint32_t * const p_message, uint32_t * const p_tick) = NULL;

static bool s_is_tune_ending = false;

/**********************************************************************************/

void chiptune_set_midi_message_callback( int(*handler_get_next_midi_message)(uint32_t * const p_message, uint32_t * const p_tick) )
{
	s_handler_get_next_midi_message = handler_get_next_midi_message;
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
		CHIPTUNE_PRINTF(cMidiSetup, "MIDI_CC_DATA_ENTRY_MSB :: voice = %u, value = %u\r\n", voice, value);
		break;
	case MIDI_CC_VOLUME:
		do
		{
			if(0 == number){
				for(int i = 0; i < MAX_TRACK_NUMBER; i++){
					s_track_info[i].volume = 2 * value;
				}
				break;
			}
			s_track_info[voice].volume = 2 * value;
		}while(0);
		CHIPTUNE_PRINTF(cMidiSetup, "%s::MIDI_CC_VOLUME :: voice = %u, value = %u\r\n", __FUNCTION__, voice, value);
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
		CHIPTUNE_PRINTF(cMidiSetup, "%s::MIDI_CC_PAN :: voice = %u, value = %u\r\n", __FUNCTION__, voice, value);
		break;
	case MIDI_CC_EXPRESSION:
	case MIDI_CC_DATA_ENTRY_LSB:
	case MIDI_CC_EFFECT_1_DEPTH:
	case MIDI_CC_EFFECT_2_DEPTH:
	case MIDI_CC_EFFECT_3_DEPTH:
	case MIDI_CC_EFFECT_4_DEPTH:
	case MIDI_CC_EFFECT_5_DEPTH:
	default:
		CHIPTUNE_PRINTF(cMidiSetup, "%s :: %u :: voice = %u, value = %u\r\n", __FUNCTION__, number,
						voice, value);
		break;
	}

	return 0;
}

/**********************************************************************************/

static int setup_program_change_into_track_info(uint8_t const voice, uint8_t const number)
{
	CHIPTUNE_PRINTF(cMidiSetup, "%s, %voice = %u, number = %u\r\n", __FUNCTION__, voice, number);

	do
	{
		if(0 == voice){
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
#if(1)
	if(818880 < s_time_tick){
		CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, voice = %u, note = %u, is_note_on = %u, velocity = %u\r\n",
						tick, voice, note, is_note_on, velocity);
	}
#endif
	do
	{
		int ii = 0;
		if(true == is_note_on){
			for(ii = 0; ii < MAX_OSCILLATOR_NUMBER; ii++){
				 if(UNSED_OSCILLATOR == s_oscillator[ii].track_index){
					 break;
				 }
			}

			if(MAX_OSCILLATOR_NUMBER== ii){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR::all oscillator are used\r\n");
				return -1;
			}
			s_oscillator[ii].track_index = voice;
			s_oscillator[ii].waveform = s_track_info[voice].waveform;
			s_oscillator[ii].duty = s_track_info[voice].duty;
			s_oscillator[ii].volume = (2 * velocity * (uint16_t)s_track_info[voice].volume) >> 8;
			s_oscillator[ii].note = note;
			s_oscillator[ii].phase = 0;
			s_oscillator[ii].start_tick = tick;
			s_oscillator[ii].end_tick = UINT32_MAX;
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

		if(MAX_OSCILLATOR_NUMBER == ii){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR::no corresponding note for off ::  track = %u,  note = %u, tick = %u\r\n",
							voice, note, tick);
			return -2;
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
		process_note_message(tick, (type == MIDI_MESSAGE_NOTE_OFF) ? false : true,
							 voice, u.data_as_bytes[1], u.data_as_bytes[2]);
		break;
	case MIDI_MESSAGE_KEY_PRESSURE:
		printf("MIDI_MESSAGE_KEY_PRESSURE\r\n");
		printf("voice = %u, ", voice);
		printf("note = %u, amount = %u\r\n",  u.data_as_bytes[1], u.data_as_bytes[2]);
		break;
	case MIDI_MESSAGE_CONTROL_CHANGE:
		setup_control_change_into_track_info(voice, u.data_as_bytes[1], u.data_as_bytes[2]);
		break;
	case MIDI_MESSAGE_PROGRAM_CHANGE:
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

uint32_t s_fetched_message = UINT32_MAX;
uint32_t s_fetched_tick = UINT32_MAX;

inline static int process_timely_midi_message(void)
{
	uint32_t message;
	uint32_t tick;

	int ii = 0;
	bool is_note_message;
	if(!(UINT32_MAX == s_fetched_message && UINT32_MAX == s_fetched_tick)){
		if(s_fetched_tick > s_time_tick + s_delta_tick){
			return 0;
		}
		process_midi_message(s_fetched_message, s_fetched_tick, &is_note_message);
		ii += 1;
	}

	bool is_no_more_message = false;
	while(1)
	{
		if(0 != s_handler_get_next_midi_message(&message, &tick)){
			is_no_more_message = true;
			break;
		}

		if(tick > s_time_tick + s_delta_tick){
			break;
		}

		process_midi_message(message, tick, &is_note_message);
		ii += 1;
	}

#if(1)
	if(818880 == tick){
		UPDATE_DELTA_TICK();
	}
#endif

	s_fetched_message = message;
	s_fetched_tick = tick;
	if(true == is_no_more_message){
		s_fetched_message = UINT32_MAX;
		s_fetched_tick = UINT32_MAX;
	}

	if(0== ii){
		return -1;
	}

	return ii;
}

/**********************************************************************************/

void chiptune_initialize(uint32_t const sampling_rate)
{
	s_is_tune_ending = false;
	s_time_tick = 0.0;
	s_sampling_rate = sampling_rate;
	s_fetched_message = UINT32_MAX;
	s_fetched_tick = UINT32_MAX;
	UPDATE_DELTA_TICK();

	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		s_oscillator[i].track_index = UNSED_OSCILLATOR;
	}

	for(int i = 0; i < MIDI_FREQUENCY_TABLE_SIZE; i++){
		/*
		 * freq = 440 * 2**((n-69)/12)
		*/
		double frequency = 440.0 * pow(2.0, (float)(i- 69)/12.0);
		frequency = round(frequency * 100.0 + 0.5)/100.0;
		/*
		 * sampling_rate/frequency = samples_per_cycle  = (UINT16_MAX = + 1)/phase
		*/
		s_phase_table[i] = (uint16_t)((UINT16_MAX + 1) * frequency / sampling_rate);
	}

	process_timely_midi_message();
	return ;
}

/**********************************************************************************/

void chiptune_set_tempo(float const tempo)
{
	s_tempo = (chiptune_float)tempo;
	UPDATE_DELTA_TICK();
}

/**********************************************************************************/

void chiptune_set_resolution(uint32_t const resolution)
{
	s_resolution = resolution;
	UPDATE_DELTA_TICK();
#if(1)
	s_delta_tick *= 100;
#endif
}

/**********************************************************************************/

uint8_t chiptune_fetch_wave(void)
{
	if(-1 == process_timely_midi_message()){
		if(true == is_all_oscillators_unused()){
			s_is_tune_ending = true;
		}
	}

	int16_t accumulated_value = 0;
	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		if(UNSED_OSCILLATOR == s_oscillator[i].track_index){
			continue;
		}

		if(s_time_tick > s_oscillator[i].end_tick){
			s_oscillator[i].track_index = UNSED_OSCILLATOR;
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
		case WAVEFORM_SAW:
			value = -32 + (s_oscillator[i].phase >> 10);
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

/**********************************************************************************/

bool chiptune_is_tune_ending(void)
{
	return s_is_tune_ending;
}
