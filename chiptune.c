#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include<stdarg.h>
#include <stdio.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
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



struct _channel_controller s_channel_controller[MAX_VOICE_NUMBER];

struct _oscillator s_oscillator[MAX_OSCILLATOR_NUMBER];


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
		s_channel_controller[voice].waveform = WAVEFORM_NOISE;
	}while(0);

	switch(s_channel_controller[voice].waveform)
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
											 uint16_t const pitch_bend_range_in_semitones, uint16_t const pitch_wheel,
											 float *p_pitch_bend_in_semitone)
{
	// TO DO : too many float variable
	float pitch_bend_in_semitone = (((int16_t)pitch_wheel - MIDI_PITCH_WHEEL_CENTER)/(float)MIDI_PITCH_WHEEL_CENTER) * DIVIDE_BY_2(pitch_bend_range_in_semitones);
	float corrected_note = (float)((int16_t)note + (int16_t)tuning_in_semitones) + pitch_bend_in_semitone;
	/*
	 * freq = 440 * 2**((note - 69)/12)
	*/
	float frequency = 440.0f * powf(2.0f, (corrected_note - 69.0f)/12.0f);
	frequency = roundf(frequency * 100.0f + 0.5f)/100.0f;
	/*
	 * sampling_rate/frequency = samples_per_cycle  = (UINT16_MAX + 1)/phase
	*/
	uint16_t delta_phase = (uint16_t)((UINT16_MAX + 1) * frequency / s_sampling_rate);
	*p_pitch_bend_in_semitone = pitch_bend_in_semitone;
	return delta_phase;
}

/**********************************************************************************/

#define NULL_MESSAGE							(0)
#define NULL_TICK								(UINT32_MAX)

#define MAX_ILLUSION_TICK_MESSAGE_NUMBER		(MAX_OSCILLATOR_NUMBER)

struct _tick_message
{
	uint32_t tick;
	uint32_t message;
} s_illusion_tick_message[MAX_ILLUSION_TICK_MESSAGE_NUMBER];


#define DIVIDE_BY_8(VALUE)							((VALUE) >> 3)
#define REDUCE_VOOLUME_AS_DAMPING_PEDAL_ON_BUT_NOTE_RELEASED(VALUE)	DIVIDE_BY_8(VALUE)

static int process_note_message(uint32_t const tick, bool const is_note_on,
						 uint8_t const voice, uint8_t const note, uint8_t const velocity, bool is_illusion_enabed,
								bool is_illusion)
{
	float pitch_bend_in_semitone;

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

			RESET_STATE_BITES(s_oscillator[ii].state_bits);
			SET_NOTE_ON(s_oscillator[ii].state_bits);
			s_oscillator[ii].voice = voice;
			s_oscillator[ii].note = note;
			s_oscillator[ii].delta_phase = calculate_delta_phase(s_oscillator[ii].note, s_channel_controller[voice].tuning_in_semitones,
															   s_channel_controller[voice].pitch_bend_range_in_semitones,
															   s_channel_controller[voice].pitch_wheel, &pitch_bend_in_semitone);
			s_oscillator[ii].current_phase = 0;
			s_oscillator[ii].volume = (uint16_t)velocity * (uint16_t)s_channel_controller[voice].playing_volume;
			s_oscillator[ii].waveform = s_channel_controller[voice].waveform;
			s_oscillator[ii].duty_cycle_critical_phase = s_channel_controller[voice].duty_cycle_critical_phase;

			s_oscillator[ii].delta_vibration_phase = calculate_delta_phase(s_oscillator[ii].note + 1, s_channel_controller[voice].tuning_in_semitones,
																	   s_channel_controller[voice].pitch_bend_range_in_semitones,
																	   s_channel_controller[voice].pitch_wheel, &pitch_bend_in_semitone) - s_oscillator[ii].delta_phase;
			s_oscillator[ii].vibration_table_index = 0;
			s_oscillator[ii].vibration_same_index_count = 0;

			if(false == is_illusion_enabed){
				break;
			}
			if(0 == s_channel_controller[voice].chorus){
				break;
			}
			s_oscillator[ii].volume >>= 1;
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
			if(true == IS_DAMPER_PEDAL_ON(s_oscillator[ii].state_bits))
			{
				SET_NOTE_OFF(s_oscillator[ii].state_bits);
				s_oscillator[ii].volume
						= REDUCE_VOOLUME_AS_DAMPING_PEDAL_ON_BUT_NOTE_RELEASED(s_oscillator[ii].volume);
				break;
			}
			s_oscillator[ii].voice = UNUSED_OSCILLATOR;
		}while(0);
	}while(0);



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
	CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, %s :: voice = %u, note = %u, velocity = %u",
					tick, is_note_on ? "MIDI_MESSAGE_NOTE_ON" : "MIDI_MESSAGE_NOTE_OFF" , voice, note, velocity);
	do
	{
		if(false == is_note_on){
			break;
		}
		if(0.0f == pitch_bend_in_semitone){
			break;
		}
		CHIPTUNE_PRINTF(cNoteOperation, ", pitch bend = %+3.2f", pitch_bend_in_semitone);
	}while(0);

	if(true == is_illusion){
		CHIPTUNE_PRINTF(cNoteOperation, " (chorus)");
	}
	CHIPTUNE_PRINTF(cNoteOperation, "\r\n");
#endif
	return 0;
}

/**********************************************************************************/

static void process_pitch_wheel_message(uint32_t const tick, uint8_t const voice, uint16_t const value)
{
	CHIPTUNE_PRINTF(cNoteOperation, "tick = %u, MIDI_MESSAGE_PITCH_WHEEL :: voice = %u, value = %u\r\n",
					tick, voice, value);
	s_channel_controller[voice].pitch_wheel = value;
	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		if(voice != s_oscillator[i].voice){
			continue;
		}
		float pitch_bend_in_semitone;
		s_oscillator[i].delta_phase = calculate_delta_phase(s_oscillator[i].note, s_channel_controller[voice].tuning_in_semitones,
														   s_channel_controller[voice].pitch_bend_range_in_semitones,
														   s_channel_controller[voice].pitch_wheel, &pitch_bend_in_semitone);

		CHIPTUNE_PRINTF(cNoteOperation, "---- voice = %u, note = %u, pitch bend = %+3.2f\r\n",voice, s_oscillator[i].note, pitch_bend_in_semitone);
	}
}

/**********************************************************************************/

static void process_midi_message(uint32_t const tick, uint32_t const message, bool is_illusion_enabed, bool is_illusion)
{
	union {
		uint32_t data_as_uint32;
		unsigned char data_as_bytes[4];
	} u;

	u.data_as_uint32 = message;

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
	{
		process_note_message(tick, (MIDI_MESSAGE_NOTE_OFF == type) ? false : true,
							 voice, u.data_as_bytes[1], u.data_as_bytes[2], is_illusion_enabed, is_illusion);
		if(false == is_illusion_enabed){
			break;
		}

		if(false == is_illusion){
			break;
		}

		int kk;
		for(kk = 0; kk < MAX_ILLUSION_TICK_MESSAGE_NUMBER; kk++){
			if(NULL_TICK == s_illusion_tick_message[kk].tick
					&& NULL_MESSAGE == s_illusion_tick_message[kk].message){
				break;
			}
		}

		if(MAX_ILLUSION_TICK_MESSAGE_NUMBER == kk){
			CHIPTUNE_PRINTF(cDeveloping, "ERROR::all illusion_tick_message are used\r\n");
			break;
		}
#define CHORUS_DELAY_TIME_IN_MS						(0.030)
		s_illusion_tick_message[kk].tick = tick + (uint32_t)(CHORUS_DELAY_TIME_IN_MS * s_tempo * s_resolution/ (60.0));
		s_illusion_tick_message[kk].message = message;
	} break;
	case MIDI_MESSAGE_KEY_PRESSURE:
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE_CHANNEL_PRESSURE :: note = %u, amount = %u %s\r\n",
						tick, voice, u.data_as_bytes[1], "(NOT IMPLEMENTED YET)");
		break;
	case MIDI_MESSAGE_CONTROL_CHANGE:
		process_control_change_message(&s_channel_controller[0], &s_oscillator[0],
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
		CHIPTUNE_PRINTF(cMidiSetup, "tick = %u, MIDI_MESSAGE code = %u :: voice = %u, byte 1 = %u, byte 2 = %u %s\r\n",
						tick, type, voice, u.data_as_bytes[1], u.data_as_bytes[2], "(NOT IMPLEMENTED YET)");
		break;
	}
}

/**********************************************************************************/

struct _tick_message s_fetched_tick_message;
uint32_t s_midi_messge_index = 0;
uint32_t s_total_message_number = 0;

#ifdef _INCREMENTAL_SAMPLE_INDEX
#define	IS_AFTER_CURRENT_TIME(TICK)					((TICK_TO_SAMPLE_INDEX(TICK) > s_current_sample_index) ? true : false)
#else
#define	IS_AFTER_CURRENT_TIME(TICK)					(((TICK) > s_current_tick) ? true : false)
#endif
/**********************************************************************************/

static int process_timely_midi_message(void)
{
	uint32_t tick;
	uint32_t message;

	for(int k = 0; k < MAX_ILLUSION_TICK_MESSAGE_NUMBER; k++){
		if(NULL_TICK == s_illusion_tick_message[k].tick
				&& NULL_MESSAGE == s_illusion_tick_message[k].message){
			continue ;
		}

		if(true == IS_AFTER_CURRENT_TIME(s_illusion_tick_message[k].tick)){
			continue ;
		}
		process_midi_message(s_illusion_tick_message[k].tick, s_illusion_tick_message[k].message, true, true);
		s_illusion_tick_message[k].tick = NULL_TICK;
		s_illusion_tick_message[k].message = NULL_MESSAGE;
	}

	int ii = 0;
	if(false == (NULL_TICK == s_fetched_tick_message.tick && NULL_MESSAGE == s_fetched_tick_message.message)){
		if(true == IS_AFTER_CURRENT_TIME(s_fetched_tick_message.tick)){
			return 0;
		}
		process_midi_message(s_fetched_tick_message.tick, s_fetched_tick_message.message, true, false);
		s_fetched_tick_message.tick = NULL_TICK;
		s_fetched_tick_message.message = NULL_MESSAGE;
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
			s_fetched_tick_message.tick = tick;
			s_fetched_tick_message.message = message;
			break;
		}

		process_midi_message(tick, message, true, false);
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
	SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(false);

	uint32_t previous_tick;
	uint32_t message;
	uint32_t midi_messge_index = 0;
	uint32_t max_amplitude = 0;

	s_handler_get_midi_message(midi_messge_index, &message, &previous_tick);
	midi_messge_index += 1;
	process_midi_message(previous_tick, message, false, false);

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

		process_midi_message(tick, message, false, false);

	}

	SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(true);
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
	s_fetched_tick_message.tick = NULL_TICK;
	s_fetched_tick_message.message = NULL_MESSAGE;

	for(int i = 0; i < MAX_ILLUSION_TICK_MESSAGE_NUMBER; i++){
		s_illusion_tick_message[i].tick = NULL_TICK;
		s_illusion_tick_message[i].message = NULL_MESSAGE;
	}

	for(int i = 0; i < MAX_VOICE_NUMBER; i++){
		reset_channel_controller(&s_channel_controller[i]);
	}

	for(int i = 0; i < MAX_OSCILLATOR_NUMBER; i++){
		s_oscillator[i].voice = UNUSED_OSCILLATOR;
	}

	s_sampling_rate = sampling_rate;
	s_resolution = resolution;
	s_total_message_number = total_message_number;
	for(int i = 0; i < VIBRATION_PHASE_TABLE_LENGTH; i++){
		s_vibration_phase_table[i] = (int8_t)(INT8_MAX *  sinf( 2.0f * (float)M_PI * i / (float)VIBRATION_PHASE_TABLE_LENGTH));
	}
	s_vibration_same_index_count_number = (uint32_t)(s_sampling_rate/(VIBRATION_PHASE_TABLE_LENGTH * (float)VIBRATION_FREQUENCY));

	UPDATE_TIME_BASE_UNIT();
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

static inline bool is_all_oscillators_unused(void)
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
		do
		{
			if(0 ==  s_channel_controller[s_oscillator[i].voice].modulation_wheel){
				break;
			}
#define DIVIDE_BY_128(VALUE)						((VALUE) >> 7)
#define NORMALIZE_VIBRAION_DELTA_PHASE_AMPLITUDE(VALUE)	\
													DIVIDE_BY_128(DIVIDE_BY_128(VALUE))
#define REGULATE_MODULATION_WHEEL(VALUE)			(VALUE + 1)

			uint16_t delta_vibration_phase = s_oscillator[i].delta_vibration_phase;

			uint32_t vibration_amplitude = REGULATE_MODULATION_WHEEL(s_channel_controller[s_oscillator[i].voice].modulation_wheel)
					* s_vibration_phase_table[s_oscillator[i].vibration_table_index];

			delta_vibration_phase = NORMALIZE_VIBRAION_DELTA_PHASE_AMPLITUDE(vibration_amplitude * delta_vibration_phase);
			s_oscillator[i].current_phase += delta_vibration_phase;

			s_oscillator[i].vibration_same_index_count += 1;
			if(s_vibration_same_index_count_number == s_oscillator[i].vibration_same_index_count){
				s_oscillator[i].vibration_same_index_count = 0;
				s_oscillator[i].vibration_table_index = (s_oscillator[i].vibration_table_index  + 1) % VIBRATION_PHASE_TABLE_LENGTH;
			}
		}while(0);

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
		case WAVEFORM_NOISE:
			break;
		default:
			break;
		}
		accumulated_value += (value * s_oscillator[i].volume);

		s_oscillator[i].current_phase += s_oscillator[i].delta_phase;
	}

	INCREMENT_TIME_BASE();
#ifdef _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING
	increase_time_base_for_fast_to_ending();
#endif

	int32_t out_value = NORMALIZE_AMPLITUDE(accumulated_value);
	do
	{
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
