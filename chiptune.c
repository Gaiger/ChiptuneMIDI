#include <string.h>
#include <math.h>

#include<stdarg.h>
#include <stdio.h>

#include "chiptune.h"

//#define _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING

//#define _INCREMENTAL_SAMPLE_INDEX
//#define _RIGHT_SHIFT_FOR_NORMALIZING_AMPLITUDE

static bool s_enable_print_out = true;

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

#define CHIPTUNE_PRINTF(PRINT_TYPE, FMT, ...)		\
													do { \
														if(false == s_enable_print_out){ \
															break;\
														} \
														chiptune_printf(PRINT_TYPE, FMT, ##__VA_ARGS__); \
													}while(0)

#if(0)
#define CHIPTUNE_PRINTF(PRINT_TYPE, FMT, ...)				do { \
													(void)0; \
												}while(0)
#endif

#define SET_CHIPTUNE_PRINTF_ENABLED(IS_ENABLED)		\
													do { \
														s_enable_print_out = (IS_ENABLED); \
													} while(0)

#ifdef _INCREMENTAL_SAMPLE_INDEX
typedef float chiptune_float;
#else
typedef double chiptune_float;
#endif

#define DEFAULT_TEMPO								(120.0)
#define DEFAULT_SAMPLING_RATE						(16000)
#define DEFAULT_RESOLUTION							(960)

uint16_t s_phase_table[MIDI_FREQUENCY_TABLE_SIZE] = {0};

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

#define UPDATE_TIME_BASE_UNIT()						UPDATE_SAMPLES_TO_TICK_RATIO()

#define TICK_TO_SAMPLE_INDEX(TICK)					((uint32_t)(s_tick_to_sample_index_ratio * (chiptune_float)(TICK) + 0.5 ))
#else
static chiptune_float s_current_tick = 0.0;
static chiptune_float s_delta_tick_per_sample = (DEFAULT_RESOLUTION / ( (chiptune_float)DEFAULT_SAMPLING_RATE/(DEFAULT_TEMPO /60.0) ) );

#define	UPDATE_DELTA_TICK_PER_SAMPLE()				\
													do { \
														s_delta_tick_per_sample = ( s_resolution * s_tempo / (chiptune_float)s_sampling_rate/ 60.0 ); \
													} while(0)

#define UPDATE_TIME_BASE_UNIT()						UPDATE_DELTA_TICK_PER_SAMPLE()
#endif

static int(*s_handler_get_midi_message)(uint32_t index,uint32_t * const p_tick, uint32_t * const p_message) = NULL;

static bool s_is_tune_ending = false;

/**********************************************************************************/

void chiptune_set_midi_message_callback( int(*handler_get_midi_message)(uint32_t index, uint32_t * const p_tick, uint32_t * const p_message) )
{
	s_handler_get_midi_message = handler_get_midi_message;
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

enum
{
	DUTY_CYLCE_125_CRITICAL_PHASE	= (UINT16_MAX + 1) >> 3,
	DUTY_CYLCE_25_CRITICAL_PHASE	= (UINT16_MAX + 1) >> 2,
	DUTY_CYLCE_50_CRITICAL_PHASE	= (UINT16_MAX + 1) >> 1,
	DUTY_CYLCE_75_CRITICAL_PHASE	= (UINT16_MAX + 1) - ((UINT16_MAX + 1) >> 2),
};

#define MAX_VOICE_NUMBER							(16)

struct _voice_info
{
	uint8_t		max_volume;
	uint8_t		playing_volume;
	uint8_t		pan;
	bool		is_damping_pedal_on;
	uint8_t		waveform;
	uint16_t	duty_cycle_critical_phase;
	uint16_t	pitch_bend_in_semitones;
	uint16_t	registered_parameter_number;
	uint16_t	registered_parameter_value;
}s_voice_info[MAX_VOICE_NUMBER];

#define MAX_OSCILLATOR_NUMBER						(MAX_VOICE_NUMBER * 2)

struct _oscillator
{
	int8_t		voice;
	uint8_t		note;
	uint16_t	volume;
	uint8_t		waveform;
	uint16_t	duty_cycle_critical_phase;
	uint16_t	current_phase;
} s_oscillator[MAX_OSCILLATOR_NUMBER];

#define UNUSED_OSCILLATOR							(-1)


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

#define MIDI_CC_DAMPER_PEDAL						(64)

#define MIDI_CC_EFFECT_1_DEPTH						(91)
#define MIDI_CC_EFFECT_2_DEPTH						(92)
#define MIDI_CC_EFFECT_3_DEPTH						(93)
#define MIDI_CC_EFFECT_4_DEPTH						(94)
#define MIDI_CC_EFFECT_5_DEPTH						(95)

#define MIDI_CC_NON_NRPN_LSB						(98)
#define MIDI_CC_NON_NRPN_MSB						(99)

//https://zh.wikipedia.org/zh-tw/General_MIDI
#define MIDI_CC_RPN_LSB								(100)
#define MIDI_CC_RPN_MSB								(101)


inline static void set_voice_registered_parameter(uint32_t const tick, uint8_t const voice)
{
	(void)tick;
#define MIDI_CC_RPN_PITCH_BEND_SENSITIVY			(0)
#define MIDI_CC_RPN_CHANNEL_FINE_TUNING				(1)
#define MIDI_CC_RPN_CHANNEL_COARSE_TUNING			(2)
#define MIDI_CC_RPN_TURNING_PROGRAM_CHANGE			(3)
#define MIDI_CC_RPN_TURNING_BANK_SELECT				(4)
#define MIDI_CC_RPN_MODULATION_DEPTH_RANGE			(5)
#define MIDI_CC_RPN_NULL							((127 << 8) + 127)
	switch(s_voice_info[voice].registered_parameter_number)
	{
	case MIDI_CC_RPN_PITCH_BEND_SENSITIVY:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_PITCH_BEND_SENSITIVY :: voice = %u, semitones = %u\r\n",
						voice, s_voice_info[voice].registered_parameter_value >> 8);
		s_voice_info[voice].pitch_bend_in_semitones = s_voice_info[voice].registered_parameter_value >> 8;
		if(0 != (s_voice_info[voice].registered_parameter_value & 0xFF)){
			CHIPTUNE_PRINTF(cMidiSetup, "----  MIDI_CC_RPN_PITCH_BEND_SENSITIVY :: voice = %u, cents = %u (%s)\r\n",
						voice, s_voice_info[voice].registered_parameter_number & 0xFF, "(NOT IMPLEMENTED YET)");
		}
		break;
	case MIDI_CC_RPN_CHANNEL_FINE_TUNING:
		CHIPTUNE_PRINTF(cMidiSetup, "----  MIDI_CC_RPN_CHANNEL_FINE_TUNING(%u) :: voice = %u, value = %u %s\r\n",
						voice, s_voice_info[voice].registered_parameter_number, s_voice_info[voice].registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_CHANNEL_COARSE_TUNING:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_CHANNEL_COARSE_TUNING(%u) :: voice = %u, value = %u %s\r\n",
						voice, s_voice_info[voice].registered_parameter_number, s_voice_info[voice].registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_TURNING_PROGRAM_CHANGE:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_TURNING_PROGRAM_CHANGE(%u) :: voice = %u, value = %u, value = %u %s\r\n",
						voice, s_voice_info[voice].registered_parameter_number, s_voice_info[voice].registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_TURNING_BANK_SELECT:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_TURNING_BANK_SELECT(%u) :: voice = %u, value = %u %s\r\n",
						voice, s_voice_info[voice].registered_parameter_number, s_voice_info[voice].registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_MODULATION_DEPTH_RANGE:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_MODULATION_DEPTH_RANGE(%u) :: voice = %u %s\r\n",
						voice, s_voice_info[voice].registered_parameter_number, s_voice_info[voice].registered_parameter_value,
						"(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_NULL:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN_NULL :: voice = %u\r\n", voice);
		s_voice_info[voice].registered_parameter_value = 0;
		break;
	default:
		CHIPTUNE_PRINTF(cMidiSetup, "---- MIDI_CC_RPN code = %d :: voice = %u, value = %u \r\n",
						s_voice_info[voice].registered_parameter_number, voice, s_voice_info[voice].registered_parameter_value);
		s_voice_info[voice].registered_parameter_value = 0;
	}
}

inline static void set_voice_info_volume(uint32_t const tick, uint8_t const voice, uint8_t const value)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_VOLUME :: voice = %u, value = %u\r\n", tick, voice, value);
	s_voice_info[voice].max_volume = value;
}

/**********************************************************************************/

inline static void set_voice_info_expression(uint32_t const tick, uint8_t const voice, uint8_t const value)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EXPRESSION :: voice = %u, value = %u\r\n", tick, voice, value);
	s_voice_info[voice].playing_volume = (value * s_voice_info[voice].max_volume)/INT8_MAX;
}

/**********************************************************************************/

inline static void set_voice_info_damping_pedal(uint32_t const tick, uint8_t const voice, uint8_t const value)
{
	bool is_damping_pedal_on = (value > 63) ?  true : false;
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_DAMPER_PEDAL :: voice = %u, is_damping_pedal_on = %u\r\n",
					tick, voice, is_damping_pedal_on);

	s_voice_info[voice].is_damping_pedal_on = is_damping_pedal_on;
	do{
		if(true == is_damping_pedal_on){
			break;
		}

		for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
			if( voice == s_oscillator[i].voice){
				s_oscillator[i].voice = UNUSED_OSCILLATOR;
			}
		}
	}while(0);
}

/**********************************************************************************/

static int setup_control_change_into_voice_info(uint32_t const tick, uint8_t const voice, uint8_t const number, uint8_t const value)
{
	switch(number){
	case MIDI_CC_DATA_ENTRY_MSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_DATA_ENTRY_MSB :: voice = %u, value = %u\r\n",
						tick, voice, value);
		s_voice_info[voice].registered_parameter_value
				= ((value & 0xFF) << 8) | s_voice_info[voice].registered_parameter_value;
		break;
	case MIDI_CC_VOLUME:
		set_voice_info_volume(tick, voice, value);
		break;
	case MIDI_CC_PAN:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_PAN(%u) :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		s_voice_info[voice].pan = value;
		break;
	case MIDI_CC_EXPRESSION:
		set_voice_info_expression(tick, voice, value);
		break;
	case MIDI_CC_DATA_ENTRY_LSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_DATA_ENTRY_LSB :: voice = %u, value = %u\r\n",
						tick, voice, value);
		s_voice_info[voice].registered_parameter_value
				= s_voice_info[voice].registered_parameter_value | ((value & 0xFF) << 0);

		set_voice_registered_parameter(tick, voice);
		break;
	case MIDI_CC_DAMPER_PEDAL:
		set_voice_info_damping_pedal(tick, voice, value);
		break;
	case MIDI_CC_EFFECT_1_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EFFECT_1_DEPTH(%u) :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_EFFECT_2_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EFFECT_2_DEPTH(%u) :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_EFFECT_3_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EFFECT_3_DEPTH(%u) :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_EFFECT_4_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "%s :: MIDI_CC_EFFECT_4_DEPTH(%u) :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_EFFECT_5_DEPTH:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_EFFECT_5_DEPTH(%u) :: voice = %u, value = %u %s\r\n",
						tick, number, voice, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_NON_NRPN_LSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_NON_NRPN_LSB(%u) :: voice = %u, value = %u %s\r\n",
						tick, voice, number, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_NON_NRPN_MSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_NON_NRPN_MSB(%u) :: voice = %u, value = %u %s\r\n",
						tick, voice, number, value, "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_CC_RPN_LSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_RPN_LSB :: voice = %u, value = %u\r\n",
						tick, voice, value);
		s_voice_info[voice].registered_parameter_number
				= s_voice_info[voice].registered_parameter_number | ((value & 0xFF) << 0);
		break;
	case MIDI_CC_RPN_MSB:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC_RPN_MSB :: voice = %u, value = %u\r\n",
						tick, voice, value);
		s_voice_info[voice].registered_parameter_number
				= ((value & 0xFF) << 8) | s_voice_info[voice].registered_parameter_number;
		break;
	default:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_CC code = %u :: voice = %u, value = %u\r\n",
						tick, number, voice, value);
		break;
	}

	return 0;
}

/**********************************************************************************/

static void setup_program_change_into_voice_info(uint32_t const tick, uint8_t const voice, uint8_t const number)
{
	CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_PROGRAM_CHANGE :: ", tick);
	switch(number)
	{
#define MIDI_INSTRUMENT_OVERDRIVE_GUITAR			(29)
	case MIDI_INSTRUMENT_OVERDRIVE_GUITAR:
		CHIPTUNE_PRINTF(cMidiSetup, "%voice = %u instrument = %u, as WAVEFORM_SQUARE with duty = 50%%\r\n", voice, number);
		s_voice_info[voice].waveform = WAVEFORM_SQUARE;
		s_voice_info[voice].duty_cycle_critical_phase = DUTY_CYLCE_50_CRITICAL_PHASE;
		break;
#define MIDI_INSTRUMENT_DISTORTION_GUITAR			(30)
	case MIDI_INSTRUMENT_DISTORTION_GUITAR:
		s_voice_info[voice].waveform = WAVEFORM_SQUARE;
		s_voice_info[voice].duty_cycle_critical_phase = DUTY_CYLCE_25_CRITICAL_PHASE;
		CHIPTUNE_PRINTF(cMidiSetup, "%voice = %u instrument = %u, as WAVEFORM_SQUARE with duty = 25%%\r\n", voice, number);
		break;
#define MIDI_INSTRUMENT_ACOUSTIC_BASS				(32)
#define MIDI_INSTRUMENT_ELECTRIC_BASS_FINGER		(33)
#define MIDI_INSTRUMENT_ELECTRIC_BASS_PICK			(34)
#define MIDI_INSTRUMENT_FRETLESS_BASS				(35)
#define MIDI_INSTRUMENT_SLAP_BASS_1					(36)
#define MIDI_INSTRUMENT_SLAP_BASS_2					(37)
#define MIDI_INSTRUMENT_SYNTH_BASS_1				(38)
#define MIDI_INSTRUMENT_SYNTH_BASS_2				(39)
	case MIDI_INSTRUMENT_ACOUSTIC_BASS:
	case MIDI_INSTRUMENT_ELECTRIC_BASS_FINGER:
	case MIDI_INSTRUMENT_ELECTRIC_BASS_PICK:
	case MIDI_INSTRUMENT_FRETLESS_BASS:
	case MIDI_INSTRUMENT_SLAP_BASS_1:
	case MIDI_INSTRUMENT_SLAP_BASS_2:
	case MIDI_INSTRUMENT_SYNTH_BASS_1:
	case MIDI_INSTRUMENT_SYNTH_BASS_2:
#define MIDI_INSTRUMENT_STRING_ENSEMBLE_1			(48)
#define MIDI_INSTRUMENT_STRING_ENSEMBLE_2			(49) //slow string
	case MIDI_INSTRUMENT_STRING_ENSEMBLE_1:
	case MIDI_INSTRUMENT_STRING_ENSEMBLE_2:
	default:
		CHIPTUNE_PRINTF(cMidiSetup, "%voice = %u instrument = %u, as WAVEFORM_TRIANGLE\r\n", voice, number);
		s_voice_info[voice].waveform = WAVEFORM_TRIANGLE;
		break;
	}

}

/**********************************************************************************/

static bool is_all_oscillators_unused(void)
{
	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		if(UNUSED_OSCILLATOR == s_oscillator[i].voice){
			continue;
		}
		return false;
	}

	return true;
}

/**********************************************************************************/

#define DIVIDE_BY_8(VALUE)							((VALUE) >> 3)
#define REDUCE_VOOLUME_AS_DAMPING_PEDAL_ON_BUT_NOTE_RELEASED(VALUE)	DIVIDE_BY_8(VALUE)

static int process_note_message(uint32_t const tick, bool const is_note_on,
						 uint8_t const voice, uint8_t const note, uint8_t const velocity)
{
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
#else
	CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %u, note = %u, velocity = %u\r\n",
					tick, is_note_on ? "MIDI_MESSAGE_NOTE_ON" : "MIDI_MESSAGE_NOTE_OFF" , voice, note, velocity);
#endif

	do
	{
		int ii = 0;
		if(true == is_note_on){
			for(ii = 0; ii < MAX_OSCILLATOR_NUMBER; ii++){
				 if(UNUSED_OSCILLATOR == s_oscillator[ii].voice){
					 break;
				 }
			}

			if(MAX_OSCILLATOR_NUMBER == ii){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR::all oscillator are used\r\n");
				return -1;
			}
			s_oscillator[ii].voice = voice;
			s_oscillator[ii].waveform = s_voice_info[voice].waveform;
			s_oscillator[ii].duty_cycle_critical_phase = s_voice_info[voice].duty_cycle_critical_phase;
			s_oscillator[ii].volume = (uint32_t)velocity * (uint32_t)s_voice_info[voice].playing_volume;
			s_oscillator[ii].note = note;
			s_oscillator[ii].current_phase = 0;
			break;
		}

		for(ii = 0; ii < MAX_OSCILLATOR_NUMBER; ii++){
			if(voice == s_oscillator[ii].voice){
				if(note == s_oscillator[ii].note){
					break;
				}
			}
		}

		if(MAX_OSCILLATOR_NUMBER == ii){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR::no corresponding note for off :: tick = %u, voice = %u,  note = %u\r\n",
							tick, voice, note);
			return -2;
		}

		do
		{
			if(s_voice_info[s_oscillator[ii].voice].is_damping_pedal_on){
				s_oscillator[ii].volume
						= REDUCE_VOOLUME_AS_DAMPING_PEDAL_ON_BUT_NOTE_RELEASED(s_oscillator[ii].volume);
				break;
			}
			s_oscillator[ii].voice = UNUSED_OSCILLATOR;
		}while(0);
	}while(0);

	return 0;
}

/**********************************************************************************/

static void process_midi_message(uint32_t const tick, uint32_t const message)
{
	union {
		uint32_t data_as_uint32;
		unsigned char data_as_bytes[4];
	} u;

	u.data_as_uint32 = message;

	uint8_t type =  u.data_as_bytes[0] & 0xF0;
	uint8_t voice = u.data_as_bytes[0] & 0x0F;

	switch(type)
	{
	case MIDI_MESSAGE_NOTE_OFF:
	case MIDI_MESSAGE_NOTE_ON:
		process_note_message(tick, (type == MIDI_MESSAGE_NOTE_OFF) ? false : true,
							 voice, u.data_as_bytes[1], u.data_as_bytes[2]);
		break;
	case MIDI_MESSAGE_KEY_PRESSURE:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_CHANNEL_PRESSURE :: note = %u, amount = %u %s\r\n",
						tick, voice, u.data_as_bytes[1], "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_MESSAGE_CONTROL_CHANGE:
		setup_control_change_into_voice_info(tick, voice, u.data_as_bytes[1], u.data_as_bytes[2]);
		break;
	case MIDI_MESSAGE_PROGRAM_CHANGE:
		setup_program_change_into_voice_info(tick, voice, u.data_as_bytes[1]);
		break;
	case MIDI_MESSAGE_CHANNEL_PRESSURE:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_CHANNEL_PRESSURE :: voice = %u, amount = %u %s\r\n",
						tick, voice, u.data_as_bytes[1], "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_MESSAGE_PITCH_WHEEL:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_PITCH_WHEEL :: voice = %u, value = %u %s\r\n",
						tick, voice,  (u.data_as_bytes[2] << 7) | u.data_as_bytes[1], "(NOT IMPLEMENTED YET)");
		break;
	default:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE UKNOWN :: %u, voice = %u,byte 1 = %u, byte 2 = %u \r\n",
						tick, type, voice, u.data_as_bytes[1], u.data_as_bytes[2]);
		break;
	}
}

/**********************************************************************************/

#define NO_FETCHED_MESSAGE							(UINT32_MAX)
#define NO_FETCHED_TICK								(UINT32_MAX)
uint32_t s_fetched_tick = NO_FETCHED_TICK;
uint32_t s_fetched_message = NO_FETCHED_MESSAGE;
uint32_t s_midi_messge_index = 0;
uint32_t s_total_message_number = 0;

#ifdef _INCREMENTAL_SAMPLE_INDEX
#define	IS_AFTER_CURRENT_TIME(TICK)					((TICK_TO_SAMPLE_INDEX(TICK) > s_current_sample_index) ? true : false)
#else
#define	IS_AFTER_CURRENT_TIME(TICK)					(((TICK) > s_current_tick) ? true : false)
#endif
/**********************************************************************************/

inline static int process_timely_midi_message(void)
{
	uint32_t tick;
	uint32_t message;

	int ii = 0;
	if(false == (NO_FETCHED_TICK == s_fetched_tick && NO_FETCHED_MESSAGE == s_fetched_message)){
		if(true == IS_AFTER_CURRENT_TIME(s_fetched_tick)){
			return 0;
		}
		process_midi_message(s_fetched_tick, s_fetched_message);
		s_fetched_message = NO_FETCHED_MESSAGE;
		s_fetched_tick = NO_FETCHED_TICK;
		ii += 1;
	}

	while(1)
	{
		if(s_total_message_number <= s_midi_messge_index){
			break;
		}

		int ret = s_handler_get_midi_message(s_midi_messge_index, &tick, &message);
		if(0 != ret){
			break;
		}
		s_midi_messge_index += 1;

		if(true == IS_AFTER_CURRENT_TIME(tick)){
			s_fetched_message = message;
			s_fetched_tick = tick;
			break;
		}

		process_midi_message(tick, message);
		ii += 1;
	}

	if(0 == ii){
		return -1;
	}

	return ii;
}

/**********************************************************************************/

static uint32_t get_max_simultaneous_amplitude(void)
{
	SET_CHIPTUNE_PRINTF_ENABLED(false);

	uint32_t previous_tick;
	uint32_t message;
	uint32_t midi_messge_index = 0;
	uint32_t max_amplitude = 0;

	s_handler_get_midi_message(midi_messge_index, &message, &previous_tick);
	midi_messge_index += 1;
	process_midi_message(previous_tick, message);

	while(midi_messge_index < s_total_message_number){
		uint32_t tick;
		int ret = s_handler_get_midi_message(midi_messge_index, &tick, &message);
		if(0 != ret){
			break;
		}
		midi_messge_index += 1;

		do
		{
			if(previous_tick == tick){
				break;
			}
			previous_tick = tick;

			uint32_t sum_amplitude = 0;
			for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
				if(UNUSED_OSCILLATOR == s_oscillator[i].voice){
					continue;
				}
				sum_amplitude += s_oscillator[i].volume;
			}
			if(sum_amplitude > max_amplitude){
				max_amplitude = sum_amplitude;
			}
		}while(0);

		process_midi_message(tick, message);
	}

	SET_CHIPTUNE_PRINTF_ENABLED(true);
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

void chiptune_initialize(uint32_t const sampling_rate, uint32_t const resolution, uint32_t const total_message_number)
{
#ifdef _INCREMENTAL_SAMPLE_INDEX
	s_current_sample_index = 0;
#else
	s_current_tick = 0;
#endif
	s_is_tune_ending = false;
	s_midi_messge_index = 0;
	s_fetched_message = NO_FETCHED_MESSAGE;
	s_fetched_tick = NO_FETCHED_TICK;
	for(int i = 0; i< MAX_VOICE_NUMBER; i++){
		s_voice_info[i].max_volume = (INT8_MAX + 1)/2;
		s_voice_info[i].playing_volume = (s_voice_info[i].max_volume * INT8_MAX)/INT8_MAX;
		s_voice_info[i].pan = 64;
		s_voice_info[i].is_damping_pedal_on = false;
		s_voice_info[i].waveform = WAVEFORM_TRIANGLE;
	}
	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		s_oscillator[i].voice = UNUSED_OSCILLATOR;
	}

	s_sampling_rate = sampling_rate;
	s_resolution = resolution;
	s_total_message_number = total_message_number;
	UPDATE_TIME_BASE_UNIT();
	for(int i = 0; i < MIDI_FREQUENCY_TABLE_SIZE; i++){
		/*
		 * freq = 440 * 2**((n-69)/12)
		*/
		double frequency = 440.0 * pow(2.0, (float)(i - 69)/12.0);
		frequency = round(frequency * 100.0 + 0.5)/100.0;
		/*
		 * sampling_rate/frequency = samples_per_cycle  = (UINT16_MAX = + 1)/phase
		*/
		s_phase_table[i] = (uint16_t)((UINT16_MAX + 1) * frequency / sampling_rate);
	}

	UPDATE_AMPLITUDE_NORMALIZER();
	process_timely_midi_message();
	return ;
}

/**********************************************************************************/

#ifdef _INCREMENTAL_SAMPLE_INDEX
#define CORRECT_TIME_BASE()							do { \
														s_current_sample_index = (uint32_t)(s_current_sample_index * s_tempo/tempo); \
													} while(0)
#else
#define CORRECT_TIME_BASE()							do { \
														(void)0; \
													} while(0)
#endif

void chiptune_set_tempo(float const tempo)
{
	CHIPTUNE_PRINTF(cMidiSetup, "%s :: tempo = %3.1f\r\n", __FUNCTION__,tempo);
	CORRECT_TIME_BASE();
	s_tempo = (chiptune_float)tempo;
	UPDATE_TIME_BASE_UNIT();
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

int16_t chiptune_fetch_16bit_wave(void)
{
	if(-1 == process_timely_midi_message()){
		if(true == is_all_oscillators_unused()){
			s_is_tune_ending = true;
		}
	}

	int64_t accumulated_value = 0;
	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		if(UNUSED_OSCILLATOR == s_oscillator[i].voice){
			continue;
		}

		int16_t value = 0;
		switch(s_oscillator[i].waveform)
		{
		case WAVEFORM_SQUARE:
			value = (s_oscillator[i].current_phase > s_oscillator[i].duty_cycle_critical_phase) ? -INT16_MAX_PLUS_1 : INT16_MAX;
			break;
		case WAVEFORM_TRIANGLE:
			do
			{
				if(s_oscillator[i].current_phase < INT16_MAX_PLUS_1){
					value = -INT16_MAX_PLUS_1 + MULTIPLY_BY_2(s_oscillator[i].current_phase);
					break;
				}
				value = INT16_MAX - MULTIPLY_BY_2(s_oscillator[i].current_phase - INT16_MAX_PLUS_1);
			}while(0);
			break;
		case WAVEFORM_SAW:
			value =  -INT16_MAX_PLUS_1 + s_oscillator[i].current_phase;
			break;
		default:
			break;
		}
		accumulated_value += (value * s_oscillator[i].volume);

		s_oscillator[i].current_phase += s_phase_table[s_oscillator[i].note];
	}

	INCREMENT_TIME_BASE();
#ifdef _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING
	increase_time_base_for_fast_to_ending();
#endif

	int32_t out_value = NORMALIZE_AMPLITUDE(accumulated_value);
	do
	{
		if(INT16_MAX < out_value ){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: out_value = %d, greater than UINT8_MAX\r\n",
							out_value);
			break;
		}

		if(-(INT16_MAX + 1) > out_value ){
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
#if(0)

uint8_t chiptune_fetch_8bit_wave(void)
{
	if(-1 == process_timely_midi_message()){
		if(true == is_all_oscillators_unused()){
			s_is_tune_ending = true;
		}
	}

	int32_t accumulated_value = 0;
	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		if(UNUSED_OSCILLATOR == s_oscillator[i].voice){
			continue;
		}

		int32_t value = 0;
		switch(s_oscillator[i].waveform)
		{
		case WAVEFORM_SQUARE:
			value = (s_oscillator[i].current_phase > s_oscillator[i].duty_cycle_critical_phase) ? -INT8_MAX_PLUS_1 : INT8_MAX;
			break;
		case WAVEFORM_TRIANGLE:
			do
			{
				if(s_oscillator[i].current_phase < INT16_MAX_PLUS_1){
					value = -INT8_MAX_PLUS_1 + REDUCE_INT16_PRECISION_TO_INT8(MULTIPLY_BY_2(s_oscillator[i].current_phase));
					break;
				}
				value = INT8_MAX - REDUCE_INT16_PRECISION_TO_INT8(MULTIPLY_BY_2((s_oscillator[i].current_phase - INT16_MAX_PLUS_1)));
			}while(0);
			break;
		case WAVEFORM_SAW:
			value = -INT8_MAX_PLUS_1 + REDUCE_INT16_PRECISION_TO_INT8(s_oscillator[i].current_phase);
			break;
		default:
			break;
		}
		accumulated_value += (value * s_oscillator[i].volume);

		s_oscillator[i].current_phase += s_phase_table[s_oscillator[i].note];
	}

	INCREMENT_TIME_BASE();
#ifdef _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING
	increase_time_base_for_fast_to_ending();
#endif

	int32_t out_value = NORMALIZE_AMPLITUDE(accumulated_value) + (INT8_MAX + 1);
	do
	{
		if(out_value > 0){
			if(UINT8_MAX < out_value ){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: out_value = %d, greater than UINT8_MAX\r\n",
								out_value);
				break;
			}
		}

		if(out_value < 0){
			if(0 > out_value){
				CHIPTUNE_PRINTF(cDeveloping, "ERROR :: out_value = %d, less than 0\r\n",
								out_value);
				break;
			}
		}
	}while(0);

	return (int8_t)out_value;
}
#else

uint8_t chiptune_fetch_8bit_wave(void)
{
	return (uint8_t)(REDUCE_INT16_PRECISION_TO_INT8(chiptune_fetch_16bit_wave()) + INT8_MAX_PLUS_1);
}
#endif

/**********************************************************************************/

bool chiptune_is_tune_ending(void)
{
	return s_is_tune_ending;
}
