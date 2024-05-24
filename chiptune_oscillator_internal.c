#include <math.h>
#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"
#include "chiptune_channel_controller_internal.h"
#include "chiptune_oscillator_internal.h"

static int16_t s_pitch_shift_in_semitones = 0;

void set_pitch_shift(int8_t pitch_shift_in_semitones)
{
	s_pitch_shift_in_semitones = (int16_t)pitch_shift_in_semitones;
}

/**********************************************************************************/

uint16_t const calculate_oscillator_delta_phase(int8_t const voice,
												int16_t const note, float const pitch_chorus_bend_in_semitones)
{
	// TO DO : too many float variable
	channel_controller_t const * const p_channel_controller = get_channel_controller_pointer_from_index(voice);

	float corrected_note = (float)(note + s_pitch_shift_in_semitones) + p_channel_controller->tuning_in_semitones
			+ p_channel_controller->pitch_wheel_bend_in_semitones + pitch_chorus_bend_in_semitones;
	/*
	 * freq = 440 * 2**((note - 69)/12)
	*/
	float frequency = 440.0f * powf(2.0f, (corrected_note - 69.0f)/12.0f);
	frequency = roundf(frequency * 100.0f + 0.5f)/100.0f;
	/*
	 * sampling_rate/frequency = samples_per_cycle  = (UINT16_MAX + 1)/phase
	*/
	uint16_t delta_phase = (uint16_t)((UINT16_MAX + 1) * frequency / get_sampling_rate());
	return delta_phase;
}

/**********************************************************************************/

//xor-shift pesudo random https://en.wikipedia.org/wiki/Xorshift
static uint32_t s_chorus_random_seed = 20240129;

static uint16_t obtain_chorus_random(void)
{
	s_chorus_random_seed ^= s_chorus_random_seed << 13;
	s_chorus_random_seed ^= s_chorus_random_seed >> 17;
	s_chorus_random_seed ^= s_chorus_random_seed << 5;
	return (uint16_t)(s_chorus_random_seed);
}

/**********************************************************************************/
#define DIVIDE_BY_2(VALUE)							((VALUE) >> 1)
#define RAMDON_RANGE_TO_PLUS_MINUS_ONE(VALUE)	\
												(((DIVIDE_BY_2(UINT16_MAX) + 1) - (VALUE))/(float)(DIVIDE_BY_2(UINT16_MAX) + 1))

float const obtain_oscillator_pitch_chorus_bend_in_semitones(int8_t const chorus,
															float const max_pitch_chorus_bend_in_semitones)
{
	if(0 == chorus){
		return 0.0;
	}

	uint16_t random = obtain_chorus_random();
	float pitch_chorus_bend_in_semitones;
	pitch_chorus_bend_in_semitones = RAMDON_RANGE_TO_PLUS_MINUS_ONE(random) * chorus/(float)INT8_MAX;
	pitch_chorus_bend_in_semitones *= max_pitch_chorus_bend_in_semitones;
	//CHIPTUNE_PRINTF(cDeveloping, "pitch_chorus_bend_in_semitones = %3.2f\r\n", pitch_chorus_bend_in_semitones);
	return pitch_chorus_bend_in_semitones;
}

/**********************************************************************************/

int setup_envelope_state(oscillator_t *p_oscillator, uint8_t evelope_state){

	if(ENVELOPE_STATE_MAX <= evelope_state){
		CHIPTUNE_PRINTF(cDeveloping, "ERROR :: undefined state number = %u in %s\r\n",
						evelope_state, __func__);
		return -1;
	}

	p_oscillator->envelope_state = evelope_state;
	p_oscillator->envelope_table_index = 0;
	p_oscillator->envelope_same_index_count = 0;
	return 0;
}
