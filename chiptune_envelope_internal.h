#ifndef _CHIPTUNE_ENVELOPE_INTERNAL_H_
#define _CHIPTUNE_ENVELOPE_INTERNAL_H_

#include "chiptune_oscillator_internal.h"

int switch_melodic_envelope_state(oscillator_t * const p_oscillator, uint8_t const evelope_state);

void update_melodic_envelope(oscillator_t * const p_oscillator);
void update_percussion_envelope(oscillator_t * const p_oscillator);

#endif //_CHIPTUNE_ENVELOPE_INTERNAL_H_
