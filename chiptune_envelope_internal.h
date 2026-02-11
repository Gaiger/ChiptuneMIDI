#ifndef _CHIPTUNE_ENVELOPE_INTERNAL_H_
#define _CHIPTUNE_ENVELOPE_INTERNAL_H_

#include "chiptune_oscillator_internal.h"


uint16_t calculate_sustain_amplitude(uint16_t const loudness,
								   normalized_midi_level_t const envelope_sustain_level);

enum EnvelopeState
{
	EnvelopeStateAttack = 0,
	EnvelopeStateDecay,
	EnvelopeStateNoteOnSustain,
	EnvelopeStateDamperSustain,
	EnvelopeStateDamperEntryRelease,
	EnvelopeStateFreeRelease,
	EnvelopeStateMax,
};

int switch_melodic_envelope_state(oscillator_t * const p_oscillator, uint8_t const evelope_state);

void update_melodic_envelope(oscillator_t * const p_oscillator);
void update_percussion_envelope(oscillator_t * const p_oscillator);

#endif //_CHIPTUNE_ENVELOPE_INTERNAL_H_
