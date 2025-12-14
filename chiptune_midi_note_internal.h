#ifndef _CHIPTUNE_MIDI_NOTE_INTERNAL_H_
#define _CHIPTUNE_MIDI_NOTE_INTERNAL_H_

#include <stdint.h>

int process_note_message(uint32_t const tick, bool const is_note_on,
						 int8_t const voice, midi_value_t const note, midi_value_t const velocity);

#endif // _CHIPTUNE_MIDI_NOTE_INTERNAL_H_
