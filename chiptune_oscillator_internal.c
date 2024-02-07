#include <math.h>
#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_channel_controller_internal.h"

#include "chiptune_oscillator_internal.h"


/**********************************************************************************/

#define DIVIDE_BY_2(VALUE)							((VALUE) >> 1)

uint16_t calculate_oscillator_delta_phase(int16_t const note, int8_t tuning_in_semitones,
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
	uint16_t delta_phase = (uint16_t)((UINT16_MAX + 1) * frequency / get_sampling_rate());
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

float obtain_oscillator_pitch_chorus_bend_in_semitone(int8_t const voice)
{
	channel_controller_t * const p_channel_controller
			= get_channel_controller_pointer_from_index(voice);
	int8_t const chorus = p_channel_controller->chorus;
	if(0 == chorus){
		return 0.0;
	}
	float const max_pitch_chorus_bend_in_semitones = p_channel_controller->max_pitch_chorus_bend_in_semitones;

	uint16_t random = chorus_ramdom();
	float pitch_chorus_bend_in_semitone;
	pitch_chorus_bend_in_semitone = RAMDON_RANGE_TO_PLUS_MINUS_ONE(random) * chorus/(float)INT8_MAX;
	pitch_chorus_bend_in_semitone *= max_pitch_chorus_bend_in_semitones;
	//CHIPTUNE_PRINTF(cDeveloping, "pitch_chorus_bend_in_semitone = %3.2f\r\n", pitch_chorus_bend_in_semitone);
	return pitch_chorus_bend_in_semitone;
}
