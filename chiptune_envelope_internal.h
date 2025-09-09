#ifndef _CHIPTUNE_ENVELOPE_INTERNAL_H_
#define _CHIPTUNE_ENVELOPE_INTERNAL_H_

#include "chiptune_oscillator_internal.h"

void advance_pitch_amplitude(oscillator_t * const p_oscillator);
void advance_percussion_waveform_and_amplitude(oscillator_t * const p_oscillator);

#endif //_CHIPTUNE_ENVELOPE_INTERNAL_H_
