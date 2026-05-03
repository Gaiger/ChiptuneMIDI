//NOLINTBEGIN(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_oscillator_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_event_internal.h"
#include "chiptune_envelope_internal.h"

#include "chiptune_midi_note_internal.h"
#include "chiptune_midi_control_change_internal.h"
#include "chiptune_midi_effect_internal.h"

#include "chiptune.h"

#ifdef _USE_SAMPLE_INDEX_AS_TIMEBASE
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

#ifdef _USE_SAMPLE_INDEX_AS_TIMEBASE
static uint32_t s_current_sample_index = 0;
static chiptune_float s_tick_to_sample_index_ratio = \
		(chiptune_float)(DEFAULT_SAMPLING_RATE * 1.0/(MIDI_DEFAULT_TEMPO * DEFAULT_PLAYING_SPEED_RATIO/60.0)/MIDI_DEFAULT_RESOLUTION);
static chiptune_float s_sample_index_to_tick_ratio = \
		(chiptune_float)(MIDI_DEFAULT_TEMPO * DEFAULT_PLAYING_SPEED_RATIO * MIDI_DEFAULT_RESOLUTION/60.0)/DEFAULT_SAMPLING_RATE;
static uint32_t s_current_tick = 0;

#define UPDATE_SAMPLES_TO_TICK_RATIO()				\
													do { \
														s_tick_to_sample_index_ratio \
															= (chiptune_float)(s_sampling_rate * (60.0/s_tempo)/s_playing_speed_ratio/s_resolution); \
														s_sample_index_to_tick_ratio \
															= (chiptune_float)((s_tempo/60.0) * s_playing_speed_ratio * s_resolution/s_sampling_rate); \
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

//#define TICK_TO_SAMPLE_INDEX(TICK)					((uint32_t)(s_tick_to_sample_index_ratio * (chiptune_float)(TICK) + 0.5))

#define RESET_CURRENT_TICK()						\
													do { \
														s_current_sample_index = 0; \
														s_current_tick = 0; \
													} while(0)

#define CURRENT_TICK()								(s_current_tick)

#define ADVANCE_CURRENT_TICK()			\
													do { \
														s_current_sample_index += 1; \
														s_current_tick = (uint32_t)((s_current_sample_index * s_sample_index_to_tick_ratio) + 0.5); \
													} while(0)

#define UPDATE_CURRENT_TICK(TICK)					do { \
														s_current_sample_index = (uint32_t)((TICK) * s_tick_to_sample_index_ratio + 0.5); \
														s_current_tick = (uint32_t)((s_current_sample_index * s_sample_index_to_tick_ratio) + 0.5); \
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

#define ADVANCE_CURRENT_TICK()			\
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

uint32_t get_sampling_rate(void) { return s_sampling_rate; }

/**********************************************************************************/

uint32_t get_resolution(void) { return s_resolution; }

/**********************************************************************************/

static float get_playing_speed_ratio(void) {return s_playing_speed_ratio; }

/**********************************************************************************/

float get_playing_tempo(void)
{
	return s_tempo * s_playing_speed_ratio;
}

/**********************************************************************************/

static inline bool is_stereo() { return s_is_stereo;}

/**********************************************************************************/

static inline bool is_processing_left_channel() { return s_is_processing_left_channel; }

/**********************************************************************************/

static inline void swap_processing_channel() { s_is_processing_left_channel = !s_is_processing_left_channel; }

/**********************************************************************************/

static int process_program_change_message(uint32_t const tick, int8_t const voice, midi_value_t const number)
{
	do {
		if(CHANNEL_CONTROLLER_INSTRUMENT_NOT_SPECIFIED != get_channel_controller_pointer_from_index(voice)->instrument){
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
			update_oscillator_phase_increment(p_oscillator);
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

		uint8_t const type = u.data_as_bytes[0] & 0xF0;
		int8_t const voice = u.data_as_bytes[0] & 0x0F;
		switch(type)
		{
		case MIDI_MESSAGE_NOTE_OFF:
		case MIDI_MESSAGE_NOTE_ON:
			process_note_message(tick, (MIDI_MESSAGE_NOTE_OFF == type) ? false : true,
				voice, (midi_value_t)SEVEN_BITS_VALID(u.data_as_bytes[1]),
								 (midi_value_t)SEVEN_BITS_VALID(u.data_as_bytes[2]));
		 break;
		case MIDI_MESSAGE_KEY_PRESSURE:
			CHIPTUNE_PRINTF(cMidiControlChange,
							"tick = %u, MIDI_MESSAGE_KEY_PRESSURE :: note = %d, amount = %d %s\r\n",
							tick, voice, SEVEN_BITS_VALID(u.data_as_bytes[1]), "(NOT IMPLEMENTED YET)");
			break;
		case MIDI_MESSAGE_CONTROL_CHANGE:
			process_control_change_message(tick, voice, (midi_value_t)SEVEN_BITS_VALID(u.data_as_bytes[1]),
										   (midi_value_t)SEVEN_BITS_VALID(u.data_as_bytes[2]));
			break;
		case MIDI_MESSAGE_PROGRAM_CHANGE:
			process_program_change_message(tick, voice, (midi_value_t)SEVEN_BITS_VALID(u.data_as_bytes[1]));
			break;
		case MIDI_MESSAGE_CHANNEL_PRESSURE:
			process_channel_pressure_message(tick, voice, (midi_value_t)SEVEN_BITS_VALID(u.data_as_bytes[1]));
			break;
		case MIDI_MESSAGE_PITCH_WHEEL:
#define COMBINE_AS_PITCH_WHEEL_14BITS(BYTE1, BYTE2)	\
	(SEVEN_BITS_VALID(BYTE2) << 7) | SEVEN_BITS_VALID(BYTE1))
			process_pitch_wheel_message(tick, voice,
										(int16_t)COMBINE_AS_PITCH_WHEEL_14BITS(u.data_as_bytes[1], u.data_as_bytes[2]);
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

static int fetch_midi_tick_message(uint32_t const message_index, struct _tick_message *p_tick_message)
{
	uint32_t tick;
	uint32_t message;
	int ret = s_handler_get_midi_message(message_index, &tick, &message);
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

static int free_note_off_but_damper_pedal_on_oscillators(uint32_t const tick)
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
			put_event(EventTypeFree, oscillator_index, tick);
		} while(0);
		oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
	}

	return ret;
}

/**********************************************************************************/

static int free_remaining_oscillators(uint32_t const tick)
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
				put_event(EventTypeFree, oscillator_index, tick);
			} while(0);
			oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
		}
	} while(0);

	return ret;
}

/**********************************************************************************/

int process_ending(uint32_t const tick)
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

#define NORMALIZE_VIBRATO_PHASE_INCREMENT(VALUE)	DIVIDE_BY_128(DIVIDE_BY_128(((int32_t)(VALUE))))

#define VIBRATO_PHASE_INCREMENT(MODULATION_WHEEL, MAX_VIBRATO_PHASE_INCREMENT, VIBRATO_TABLE_VALUE) \
	NORMALIZE_VIBRATO_PHASE_INCREMENT( \
		(MAX_VIBRATO_PHASE_INCREMENT) * ((MODULATION_WHEEL) * (VIBRATO_TABLE_VALUE)) )

void perform_vibrato(oscillator_t * const p_oscillator)
{
	do {
		if(true == is_stereo()
				&& false == is_processing_left_channel()){
			break;
		}

		if(true == IS_PERCUSSION_OSCILLATOR(p_oscillator)){
			break;
		}

		channel_controller_t *p_channel_controller = get_channel_controller_pointer_from_index(p_oscillator->voice);
		normalized_midi_level_t const modulation_wheel = p_channel_controller->modulation_wheel;
		if(0 >= modulation_wheel){
			break;
		}

		p_oscillator->current_phase +=
				VIBRATO_PHASE_INCREMENT(modulation_wheel, p_oscillator->max_vibrato_phase_increment,
										p_channel_controller->p_vibrato_phase_table[p_oscillator->vibrato_table_index]);
		p_oscillator->vibrato_same_index_count += 1;
		if(p_channel_controller->vibrato_same_index_number == p_oscillator->vibrato_same_index_count){
			p_oscillator->vibrato_same_index_count = 0;
			p_oscillator->vibrato_table_index = REMAINDER_OF_DIVIDE_BY_CHANNEL_CONTROLLER_LOOKUP_TABLE_LENGTH(
						p_oscillator->vibrato_table_index + 1);
		}
	} while(0);
}

/**********************************************************************************/

void perform_melodic_envelope(oscillator_t * const p_oscillator)
{
	do {
		if(true == is_stereo()
				&& false == is_processing_left_channel()){
			break;
		}
		if(true == IS_PERCUSSION_OSCILLATOR(p_oscillator)){
			break;
		}

		update_melodic_envelope(p_oscillator);
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
		if(false == (true == IS_PERCUSSION_OSCILLATOR(p_oscillator))){
			break;
		}

		update_percussion_envelope(p_oscillator);
	}while(0);
}

/**********************************************************************************/

static inline int16_t obtain_sine_wave(uint16_t const phase)
{
#define DIVIDE_BY_UINT16_MAX_PLUS_ONE(VALUE)		(((int32_t)(VALUE)) >> 16)
	return s_sine_table[DIVIDE_BY_UINT16_MAX_PLUS_ONE(phase * SINE_TABLE_LENGTH)];
}

/**********************************************************************************/

static uint16_t s_noise_random_seed = 1;

static uint16_t generate_lfsr15_random(void)
{
	uint8_t feedback;
	/*hardware chiptune project version :*/
	feedback=((s_noise_random_seed >> 13) & 1)^((s_noise_random_seed >> 14) & 1 );
	s_noise_random_seed = (s_noise_random_seed<<1) + feedback;
	s_noise_random_seed &= 0x7fff;
	/*
	 * NES version :
	 * feedback = ((s_noise_random_seed >> 0) & 1) ^ ((s_noise_random_seed >> 1) & 1);
	 * s_noise_random_seed  = (s_noise_random_seed >> 1) + (feedback << 14) ;
	 */
	return s_noise_random_seed;
}

static uint16_t generate_waveform_noise_value(void)
{
	return MULTIPLY_BY_2(generate_lfsr15_random()) - INT16_MAX_PLUS_1;
}

/**********************************************************************************/

#define SINE_WAVE(PHASE)							(obtain_sine_wave(PHASE))

void update_mono_wave_amplitude(oscillator_t * const p_oscillator)
{
	do
	{
		if(true == is_stereo()
				&& false == is_processing_left_channel()){
			break;
		}

		channel_controller_t const *p_channel_controller
				= get_channel_controller_pointer_from_index(p_oscillator->voice);

		int8_t waveform = p_channel_controller->waveform;
		uint16_t critical_phase = p_channel_controller->duty_cycle_critical_phase;
		if(true == IS_PERCUSSION_OSCILLATOR(p_oscillator)){
			percussion_t const * const p_percussion = get_percussion_pointer_from_key(p_oscillator->note);
			waveform = p_percussion->waveform[p_oscillator->percussion_waveform_segment_index];
			critical_phase = INT16_MAX_PLUS_1;
		}
		uint16_t current_phase_advanced_by_quarter_cycle
				= p_oscillator->current_phase + DIVIDE_BY_2(INT16_MAX_PLUS_1);

		int16_t wave = 0;
		switch(waveform)
		{
		case WaveformSquare:
			wave = (critical_phase > p_oscillator->current_phase)
					? INT16_MAX : -INT16_MAX_PLUS_1;
			break;
		case WaveformTriangle:
			do {
				if(INT16_MAX_PLUS_1 > current_phase_advanced_by_quarter_cycle){
					wave = -INT16_MAX_PLUS_1 + MULTIPLY_BY_2(current_phase_advanced_by_quarter_cycle);
					break;
				}
				wave = INT16_MAX - MULTIPLY_BY_2(current_phase_advanced_by_quarter_cycle - INT16_MAX_PLUS_1);
			} while(0);
			break;
		case WaveformSaw:
			wave = -INT16_MAX_PLUS_1 + current_phase_advanced_by_quarter_cycle;
			break;
		case WaveformSine:
			wave = SINE_WAVE(p_oscillator->current_phase);
			break;
		case WaveformNoise:
			wave = (int16_t)generate_waveform_noise_value();
			break;
		default:
			CHIPTUNE_PRINTF(cDeveloping, "ERROR :: waveform = %d in %s\r\n ",
							waveform, __func__);
			wave = 0;
			break;
		}
		p_oscillator->current_phase += p_oscillator->base_phase_increment;

		p_oscillator->mono_wave_amplitude = wave * (int32_t)p_oscillator->amplitude;
	} while(0);
}

/**********************************************************************************/

#define PANNED_WAVE_AMPLITUDE(MONO_WAVE_AMPLITUDE, PANNING_WEIGHT) \
	DIVIDE_BY_128((int64_t)(MONO_WAVE_AMPLITUDE) * (PANNING_WEIGHT))

int32_t generate_panned_wave_amplitude(oscillator_t const * const p_oscillator)
{
	int32_t pannel_wave_amplitude = p_oscillator->mono_wave_amplitude;
	do {
		if(false == is_stereo()){
			break;
		}

		channel_controller_t const * const p_channel_controller
				= get_channel_controller_pointer_from_index(p_oscillator->voice);
		bool is_pan_not_centered = (MIDI_SEVEN_BITS_CENTER_VALUE != p_channel_controller->pan);
		uint8_t panning_weight = ONE_TO_ZERO(p_channel_controller->pan + (uint8_t)is_pan_not_centered);
		if(true == is_processing_left_channel()){
			panning_weight = 2 * MIDI_SEVEN_BITS_CENTER_VALUE - panning_weight;
		}
		pannel_wave_amplitude
				= PANNED_WAVE_AMPLITUDE(p_oscillator->mono_wave_amplitude, panning_weight);
	} while(0);

	return pannel_wave_amplitude;
}

/**********************************************************************************/
#define MIX_WAVE_AMPLITUDE_DIVISOR					(256)

static int32_t chiptune_fetch_32bit_wave(void)
{
	if(-1 == process_timely_midi_message_and_event()){
		s_is_tune_ending = true;
	}

	int32_t accumulated_wave_amplitude = 0;
	int16_t oscillator_index = get_occupied_oscillator_head_index();
	int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
	for(int16_t k = 0; k < occupied_oscillator_number; k++){
		oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
		do {
			if(false == get_channel_controller_pointer_from_index(p_oscillator->voice)->is_output_enabled){
				break;
			}
			if(false == IS_ACTIVATED(p_oscillator->state_bits)){
				break;
			}

			perform_vibrato(p_oscillator);
			perform_melodic_envelope(p_oscillator);
			perform_percussion(p_oscillator);

			update_mono_wave_amplitude(p_oscillator);
			int32_t panned_wave_amplitude = generate_panned_wave_amplitude(p_oscillator);
			int32_t const mix_wave_amplitude = panned_wave_amplitude/MIX_WAVE_AMPLITUDE_DIVISOR;
#ifdef _DEBUG
			int64_t const accumulated_wave_amplitude_64bit =
				(int64_t)accumulated_wave_amplitude + mix_wave_amplitude;
			if(INT32_MAX < accumulated_wave_amplitude_64bit  || INT32_MIN > accumulated_wave_amplitude_64bit){
				CHIPTUNE_PRINTF(cDeveloping, "mix wave amplitude overflow\r\n");
			}
#endif
			accumulated_wave_amplitude += mix_wave_amplitude;
		} while(0);
		oscillator_index = get_occupied_oscillator_next_index(oscillator_index);
	}

	if(false == is_stereo()
			|| false == is_processing_left_channel()){
		ADVANCE_CURRENT_TICK();
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

static void chase_midi_messages(uint32_t const end_midi_message_index,
								bool is_channel_has_note_array[MIDI_MAX_CHANNEL_NUMBER])
{
	if(NULL != is_channel_has_note_array){
		memset(&is_channel_has_note_array[0], false, sizeof(bool) * MIDI_MAX_CHANNEL_NUMBER);
	}
	clear_all_oscillators_and_events();
	for(int8_t voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		reset_channel_controller_to_midi_defaults(voice);
	}
	RESET_STATIC_INDEX_MESSAGE_TICK_VARIABLES();
	if(0 == end_midi_message_index){
		return ;
	}
	SET_CHIPTUNE_PRINTF_ENABLED(false);
	int16_t max_occupied_oscillator_number = 0;

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
					put_event(EventTypeDeactivate, oscillator_index, CURRENT_TICK());
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

			if(max_occupied_oscillator_number < get_occupied_oscillator_number()){
				max_occupied_oscillator_number = get_occupied_oscillator_number();
			}

			int16_t oscillator_index = get_occupied_oscillator_head_index();
			int16_t const occupied_oscillator_number = get_occupied_oscillator_number();
			for(int16_t i = 0; i < occupied_oscillator_number; i++,
				oscillator_index = get_occupied_oscillator_next_index(oscillator_index)){
				oscillator_t * const p_oscillator = get_oscillator_pointer_from_index(oscillator_index);
				if(NULL != is_channel_has_note_array){
					is_channel_has_note_array[p_oscillator->voice] = true;
				}
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
		CHIPTUNE_PRINTF(cDeveloping, "max_occupied_oscillator_number = %d\r\n",
						max_occupied_oscillator_number);
	}

	SET_CHIPTUNE_PRINTF_ENABLED(true);
}

/**********************************************************************************/
uint32_t number_of_roundup_to_power2(uint32_t const value)
{
	uint32_t v = value; // compute the next highest power of 2 of 32-bit v

	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

#define DEFAULT_AMPLITUDE_NORMALIZATION_DIVISOR	(1)
static int32_t s_amplitude_normalization_divisor = DEFAULT_AMPLITUDE_NORMALIZATION_DIVISOR;
#define AMPLITUDE_NORMALIZATION_DIVISOR()			(s_amplitude_normalization_divisor)

#ifdef _USE_SCALED_RECIPROCAL_MULTIPLIER_FOR_WAVE_AMPLITUDE_NORMALIZATION
#define _DEBUG_SCALED_RECIPROCAL_MULTIPLIER_WAVE_AMPLITUDE_NORMALIZATION
/*
 * wave_amplitude / divisor
 * ~= wave_amplitude * reciprocal_multiplier / (2 ** shift)
 *
 * reciprocal_multiplier = (int)(2 ** shift / divisor + 0.5)
 *
 * The error is:
 * wave_amplitude * (2 ** shift / divisor - reciprocal_multiplier) / (2 ** shift)
 *
 * Since rounding keeps:
 * abs(2 ** shift / divisor - reciprocal_multiplier) <= 0.5
 *
 * The worst-case error is:
 * wave_amplitude * 0.5 / (2 ** shift)
 *
 * Since `wave_amplitude` is already reduced to int32_t here:
 * wave_amplitude_max = 2 ** 31
 *
 * The final worst-case error is:
 * 2 ** 31 * 0.5 / (2 ** shift)
 * = 2 ** (30 - shift)
 *
 * shift = 30 keeps the worst-case normalization error within 1.
 */
#define AMPLITUDE_NORMALIZATION_RECIPROCAL_MULTIPLIER_SHIFT_BIT_NUMBER (30)

/**********************************************************************************/
static inline int32_t calculate_amplitude_normalization_reciprocal_multiplier(
	int32_t const amplitude_normalization_divisor)
{
	int32_t const scale = (int32_t)1 << AMPLITUDE_NORMALIZATION_RECIPROCAL_MULTIPLIER_SHIFT_BIT_NUMBER;
	return (int32_t)((scale + (amplitude_normalization_divisor / 2)) / amplitude_normalization_divisor);
}

static int32_t s_amplitude_normalization_reciprocal_multiplier =
	(((int64_t)1 << AMPLITUDE_NORMALIZATION_RECIPROCAL_MULTIPLIER_SHIFT_BIT_NUMBER)
		+ (DEFAULT_AMPLITUDE_NORMALIZATION_DIVISOR / 2)) / DEFAULT_AMPLITUDE_NORMALIZATION_DIVISOR;
#define RESET_AMPLITUDE_NORMALIZATION_DIVISOR()	\
													do { \
														s_amplitude_normalization_divisor = DEFAULT_AMPLITUDE_NORMALIZATION_DIVISOR; \
														s_amplitude_normalization_reciprocal_multiplier \
															= calculate_amplitude_normalization_reciprocal_multiplier(DEFAULT_AMPLITUDE_NORMALIZATION_DIVISOR); \
													}while(0)
#define AMPLITUDE_NORMALIZATION_RECIPROCAL_MULTIPLIER()	(s_amplitude_normalization_reciprocal_multiplier)

#define UPDATE_AMPLITUDE_NORMALIZATION_RECIPROCAL_MULTIPLIER(AMPLITUDE_NORMALIZATION_DIVISOR) \
													do { \
														s_amplitude_normalization_reciprocal_multiplier \
															= calculate_amplitude_normalization_reciprocal_multiplier(AMPLITUDE_NORMALIZATION_DIVISOR); \
													}while(0)
#define UPDATE_AMPLITUDE_NORMALIZATION_DIVISOR(AMPLITUDE_NORMALIZATION_DIVISOR)	\
													do { \
														int32_t const amplitude_normalization_divisor_to_update = (AMPLITUDE_NORMALIZATION_DIVISOR); \
														s_amplitude_normalization_divisor = amplitude_normalization_divisor_to_update; \
														UPDATE_AMPLITUDE_NORMALIZATION_RECIPROCAL_MULTIPLIER(amplitude_normalization_divisor_to_update); \
													}while(0)
#define WAVE_AMPLITUDE_NORMALIZATION_BY_DIVISION(WAVE_AMPLITUDE) \
													((int32_t)((WAVE_AMPLITUDE)/(int32_t)s_amplitude_normalization_divisor))

/**********************************************************************************/
static inline int64_t right_shift_divide_toward_zero(int64_t const value)
{
	uint64_t const divisor_minus_one = ((uint64_t)1 << AMPLITUDE_NORMALIZATION_RECIPROCAL_MULTIPLIER_SHIFT_BIT_NUMBER) - 1;
	uint64_t const toward_zero_bias = ((uint64_t)value >> 63) * divisor_minus_one;
	return (value + (int64_t)toward_zero_bias) >> AMPLITUDE_NORMALIZATION_RECIPROCAL_MULTIPLIER_SHIFT_BIT_NUMBER;
}

/**********************************************************************************/
static inline int32_t normalize_wave_amplitude_by_scaled_reciprocal_multiplier(
	int64_t const wave_amplitude,
	int32_t const amplitude_normalization_reciprocal_multiplier)
{
	int64_t const product = wave_amplitude
		* amplitude_normalization_reciprocal_multiplier;
	int32_t const normalized_wave_amplitude = (int32_t)right_shift_divide_toward_zero(product);

#ifdef _DEBUG_SCALED_RECIPROCAL_MULTIPLIER_WAVE_AMPLITUDE_NORMALIZATION
	int32_t const division_normalized_wave_amplitude =
		WAVE_AMPLITUDE_NORMALIZATION_BY_DIVISION(wave_amplitude);
	int32_t const calculated_amplitude_normalization_reciprocal_multiplier =
		calculate_amplitude_normalization_reciprocal_multiplier(AMPLITUDE_NORMALIZATION_DIVISOR());
	int64_t const calculated_product = wave_amplitude
		* calculated_amplitude_normalization_reciprocal_multiplier;
	int32_t const calculated_reciprocal_normalized_wave_amplitude =
		(int32_t)right_shift_divide_toward_zero(calculated_product);
	if(1 < abs(division_normalized_wave_amplitude - normalized_wave_amplitude)){
		CHIPTUNE_PRINTF(
			cDeveloping,
			"WAVE_AMPLITUDE_NORMALIZATION differs: division=%d normalized=%d reciprocal=%d calculated_reciprocal=%d gain=%d multiplier=%lld calculated_multiplier=%lld\r\n",
			division_normalized_wave_amplitude,
			normalized_wave_amplitude,
			normalized_wave_amplitude,
			calculated_reciprocal_normalized_wave_amplitude,
			AMPLITUDE_NORMALIZATION_DIVISOR(),
			(long long)amplitude_normalization_reciprocal_multiplier,
			(long long)calculated_amplitude_normalization_reciprocal_multiplier);
	}
#endif
	return normalized_wave_amplitude;
}

#define NORMALIZE_WAVE_AMPLITUDE(WAVE_AMPLITUDE) \
													(normalize_wave_amplitude_by_scaled_reciprocal_multiplier((WAVE_AMPLITUDE), AMPLITUDE_NORMALIZATION_RECIPROCAL_MULTIPLIER()))
#else
#define RESET_AMPLITUDE_NORMALIZATION_DIVISOR()	\
													do { \
														s_amplitude_normalization_divisor = DEFAULT_AMPLITUDE_NORMALIZATION_DIVISOR; \
													}while(0)

#define UPDATE_AMPLITUDE_NORMALIZATION_DIVISOR(AMPLITUDE_NORMALIZATION_DIVISOR)	\
													do { \
														s_amplitude_normalization_divisor = (AMPLITUDE_NORMALIZATION_DIVISOR); \
													}while(0)
#define NORMALIZE_WAVE_AMPLITUDE(WAVE_AMPLITUDE)		((int32_t)((WAVE_AMPLITUDE)/(int32_t)s_amplitude_normalization_divisor))
#endif

#define AMPLITUDE_NORMALIZATION_DIVISOR_INCREMENT	(1)
/*
 * Recovery step in dB:
 *   20 * log10(divisor / (divisor - decrement))
 *
 * Per-window loudness change guideline:
 *   < 0.25 dB  usually imperceptible
 *   0.25-0.5 dB subtle, mostly smooth
 *   0.5-1 dB   may be audible on sustained/simple tones
 *   > 1 dB     likely pumping or level jump
 *   > 3 dB     obvious level change
 *
 * Typical divisor is around 40-120. decrement = 2 keeps recovery
 * mostly around 0.15-0.45 dB/window in that range.
 */
#define AMPLITUDE_NORMALIZATION_DIVISOR_DECREMENT	(2)
#define REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_OUTPUT_AMPLITUDE_THRESHOLD	((INT16_MAX * 3) / 4)
static uint32_t s_reduce_amplitude_normalization_divisor_sample_count = 0;
#define RESET_REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_SAMPLE_COUNT() \
													do { \
														s_reduce_amplitude_normalization_divisor_sample_count = 0; \
													}while(0)
#define ADVANCE_REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_SAMPLE_COUNT() \
													do { \
														s_reduce_amplitude_normalization_divisor_sample_count += 1; \
													}while(0)
#define IS_TO_REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR() \
													(((AMPLITUDE_NORMALIZATION_DIVISOR() > DEFAULT_AMPLITUDE_NORMALIZATION_DIVISOR) \
														&& (REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_SAMPLE_NUMBER() <= s_reduce_amplitude_normalization_divisor_sample_count)) \
														? true : false)
#define IS_OUTPUT_AMPLITUDE_BELOW_THRESHOLD(WAVE_AMPLITUDE) \
													((REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_OUTPUT_AMPLITUDE_THRESHOLD > abs(WAVE_AMPLITUDE)) \
														? true : false)

static uint32_t s_reduce_amplitude_normalization_divisor_sample_number = (UINT16_MAX + 1);
#define SET_REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_SAMPLE_NUMBER(SAMPLE_NUMBER) \
													do { \
														s_reduce_amplitude_normalization_divisor_sample_number =\
															number_of_roundup_to_power2(SAMPLE_NUMBER); \
													}while(0)
#define REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_SAMPLE_NUMBER() \
													((uint32_t const)s_reduce_amplitude_normalization_divisor_sample_number)

/**********************************************************************************/

void chiptune_initialize(bool const is_stereo, uint32_t const sampling_rate,
						 chiptune_get_midi_message_callback_t const get_midi_message_callback)
{
	s_is_stereo = is_stereo;
	s_is_processing_left_channel = true;
	s_handler_get_midi_message = get_midi_message_callback;
	UPDATE_SAMPLING_RATE(sampling_rate);
	UPDATE_RESOLUTION(MIDI_DEFAULT_RESOLUTION);
	UPDATE_TEMPO(MIDI_DEFAULT_TEMPO);
	SET_REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_SAMPLE_NUMBER(sampling_rate);
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

	bool is_channel_has_note_array[MIDI_MAX_CHANNEL_NUMBER];
	reset_all_channels_to_defaults();
	chase_midi_messages(TO_LAST_MESSAGE_INDEX, &is_channel_has_note_array[0]);
	for(int8_t voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		s_ending_instrument_array[voice] = get_channel_controller_pointer_from_index(voice)->instrument;
		if(CHIPTUNE_INSTRUMENT_NOT_SPECIFIED == s_ending_instrument_array[voice]){
			if(false == is_channel_has_note_array[voice]){
				s_ending_instrument_array[voice] = CHIPTUNE_INSTRUMENT_UNUSED_CHANNEL;
			}
		}
	}

	clear_all_oscillators_and_events();
	reset_all_channels_to_defaults();

	RESET_STATIC_INDEX_MESSAGE_TICK_VARIABLES();
	RESET_AMPLITUDE_NORMALIZATION_DIVISOR();
	RESET_REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_SAMPLE_COUNT();
}

/**********************************************************************************/

void chiptune_set_current_message_index(uint32_t const message_index)
{
	chase_midi_messages(message_index, NULL);
	RESET_AMPLITUDE_NORMALIZATION_DIVISOR();
	RESET_REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_SAMPLE_COUNT();
}

/**********************************************************************************/

void chiptune_set_tempo(float const tempo)
{
	CHIPTUNE_PRINTF(cMidiControlChange, "tick = %d, set tempo as %3.1f\r\n", CURRENT_TICK(), tempo);
	adjust_event_triggering_tick_by_playing_tempo(CURRENT_TICK(), tempo * get_playing_speed_ratio());
	UPDATE_TEMPO(tempo);
	update_effect_tick();
	synchronize_channel_controllers_to_playing_tempo();
}

/**********************************************************************************/

float chiptune_get_tempo(void)
{
	return s_tempo;
}

/**********************************************************************************/

void chiptune_set_playing_speed_ratio(float const playing_speed_ratio)
{
	adjust_event_triggering_tick_by_playing_tempo(CURRENT_TICK(), chiptune_get_tempo() * playing_speed_ratio);
	UPDATE_PLAYING_SPEED_RATIO(playing_speed_ratio);
	update_effect_tick();
	synchronize_channel_controllers_to_playing_tempo();
}

/**********************************************************************************/

float chiptune_get_playing_effective_tempo(void)
{
	return get_playing_tempo();
}

/**********************************************************************************/

void chiptune_get_ending_instruments(int8_t instrument_array[MIDI_MAX_CHANNEL_NUMBER])
{
	memcpy(&instrument_array[0], &s_ending_instrument_array[0], MIDI_MAX_CHANNEL_NUMBER * sizeof(int8_t));
}

/**********************************************************************************/
#ifdef _DEBUG
#define _REPORT_AMPLITUDE_NORMALIZAION
#endif

int16_t chiptune_fetch_16bit_wave(void)
{
	int32_t const wave_32bit = chiptune_fetch_32bit_wave();
	int32_t const original_amplitude_normalization_divisor = AMPLITUDE_NORMALIZATION_DIVISOR();
	while(1)
	{
		int32_t output_wave = NORMALIZE_WAVE_AMPLITUDE(wave_32bit);
		do {
			if(INT16_MAX < output_wave){
				break;
			}
			if(-INT16_MAX_PLUS_1 > output_wave){
				break;
			}

			do
			{

				if(AMPLITUDE_NORMALIZATION_DIVISOR() != original_amplitude_normalization_divisor){
#ifdef _REPORT_AMPLITUDE_NORMALIZAION
					CHIPTUNE_PRINTF(cDeveloping, "raise AMPLITUDE_NORMALIZATION_DIVISOR to %d\r\n",
									AMPLITUDE_NORMALIZATION_DIVISOR());
#endif
					break;
				}

				if(true == IS_OUTPUT_AMPLITUDE_BELOW_THRESHOLD(output_wave)){
					if(true == IS_TO_REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR()){
						int32_t updated_amplitude_normalization_divisor
								= AMPLITUDE_NORMALIZATION_DIVISOR() - AMPLITUDE_NORMALIZATION_DIVISOR_DECREMENT;
						if(DEFAULT_AMPLITUDE_NORMALIZATION_DIVISOR > updated_amplitude_normalization_divisor){
							updated_amplitude_normalization_divisor = DEFAULT_AMPLITUDE_NORMALIZATION_DIVISOR;
						}

						UPDATE_AMPLITUDE_NORMALIZATION_DIVISOR(updated_amplitude_normalization_divisor);
						RESET_REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_SAMPLE_COUNT();
#ifdef _REPORT_AMPLITUDE_NORMALIZAION
						float const recovery_step_in_db = 20.0f
								* log10f((float)original_amplitude_normalization_divisor
										/ (float)updated_amplitude_normalization_divisor);
						CHIPTUNE_PRINTF(cDeveloping,
										"reduce AMPLITUDE_NORMALIZATION_DIVISOR to %d (%+1.2f dB)\r\n",
										AMPLITUDE_NORMALIZATION_DIVISOR(), recovery_step_in_db);
#endif
						break;
					}

					if(false == is_stereo()
							|| false == is_processing_left_channel()){
						ADVANCE_REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_SAMPLE_COUNT();
					}
					break;
				}
				RESET_REDUCE_AMPLITUDE_NORMALIZATION_DIVISOR_SAMPLE_COUNT();
			}while(0);
			return (int16_t)output_wave;
		}while(0);

		UPDATE_AMPLITUDE_NORMALIZATION_DIVISOR(AMPLITUDE_NORMALIZATION_DIVISOR() + AMPLITUDE_NORMALIZATION_DIVISOR_INCREMENT);
	} while(1);

	return 0;
}

/**********************************************************************************/
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
	if( 0 > channel_index || MIDI_MAX_CHANNEL_NUMBER <= channel_index){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: channel_index = %d is not acceptable for %s\r\n",
						channel_index, __func__);
		return ;
	}

	get_channel_controller_pointer_from_index(channel_index)->is_output_enabled = is_enabled;
}

/**********************************************************************************/

int chiptune_set_melodic_channel_timbre(int8_t const channel_index, int8_t const waveform,
									  int8_t const envelope_attack_curve, float const envelope_attack_duration_in_seconds,
									  int8_t const envelope_decay_curve, float const envelope_decay_duration_in_seconds,
									  uint8_t const envelope_note_on_sustain_level,
									  int8_t const envelope_release_curve, float const envelope_release_duration_in_seconds,
									  uint8_t const envelope_damper_sustain_level,
									  int8_t const envelope_damper_sustain_curve,
									  float const envelope_damper_sustain_duration_in_seconds)
{
	if( 0 > channel_index  || channel_index >= MIDI_MAX_CHANNEL_NUMBER){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: channel_index = %d is not acceptable for %s\r\n",
						channel_index, __func__);
		return -1;
	}
	if(MIDI_PERCUSSION_CHANNEL == channel_index){
		CHIPTUNE_PRINTF(cDeveloping, "WARNING :: channel_index = %d is MIDI_PERCUSSION_CHANNEL %s\r\n",
						MIDI_PERCUSSION_CHANNEL , __func__);
		return 1;
	}

	int8_t channel_controller_waveform = WaveformSquare;
	uint16_t dutycycle_critical_phase = WaveformDutyCycle50;
	switch(waveform)
	{
	case ChiptuneWaveformSquareDutyCycle50:
		dutycycle_critical_phase = WaveformDutyCycle50;
		break;
	case ChiptuneWaveformSquareDutyCycle25:
		dutycycle_critical_phase = WaveformDutyCycle25;
		break;
	case ChiptuneWaveformSquareDutyCycle12_5:
		dutycycle_critical_phase = WaveformDutyCycle12_5;
		break;
	case ChiptuneWaveformSquareDutyCycle75:
		dutycycle_critical_phase = WaveformDutyCycle75;
		break;
	case ChiptuneWaveformTriangle:
		channel_controller_waveform = WaveformTriangle;
		break;
	case ChiptuneWaveformSaw:
		channel_controller_waveform = WaveformSaw;
		break;
	default:
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: waveform = %d is not acceptable for %s\r\n",
						waveform, __func__);
		return -2;
	}

	return set_melodic_channel_timbre(channel_index, channel_controller_waveform, dutycycle_critical_phase,
									  envelope_attack_curve, envelope_attack_duration_in_seconds,
									  envelope_decay_curve, envelope_decay_duration_in_seconds,
									  envelope_note_on_sustain_level,
									  envelope_release_curve, envelope_release_duration_in_seconds,
									  envelope_damper_sustain_level,
									  envelope_damper_sustain_curve,
									  envelope_damper_sustain_duration_in_seconds);
}

/**********************************************************************************/

void chiptune_set_pitch_shift_in_semitones(int8_t const pitch_shift_in_semitones)
{
	set_pitch_shift_in_semitones((int16_t)pitch_shift_in_semitones);
}
/**********************************************************************************/

int8_t chiptune_get_pitch_shift_in_semitones(void)
{
	return (int8_t)get_pitch_shift_in_semitones();
}

// NOLINTEND(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
