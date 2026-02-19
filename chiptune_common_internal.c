#include <math.h>
#include "chiptune_common_internal.h"

#ifndef _FIXED_MAX_OSCILLATOR_AND_EVENT_NUMBER
#include <stdlib.h>

/**********************************************************************************/

void* chiptune_malloc(size_t const size){ return malloc(size);}

/**********************************************************************************/

void chiptune_free(void * const ptr){ free(ptr);}

#endif

/**********************************************************************************/

uint16_t const calculate_phase_increment_from_pitch(float const pitch_in_semitones)
{
	// TO DO : too many float variable

	/*
	 * freq = 440 * 2**((note - 69)/12)
	*/
	float frequency = 440.0f * powf(2.0f, (pitch_in_semitones - 69.0f)/12.0f);
	frequency = roundf(frequency * 100.0f)/100.0f;

	/*
	 * sampling_rate/frequency = samples_per_cycle  = (UINT16_MAX + 1)/delta_phase
	*/
	uint16_t phase_increment = (uint16_t)((UINT16_MAX + 1) * frequency / get_sampling_rate());

	return phase_increment;
}
