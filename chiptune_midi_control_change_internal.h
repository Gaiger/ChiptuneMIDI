#ifndef _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_
#define _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_

#include <stdint.h>

int process_control_change_message(uint32_t const tick, int8_t const voice, int8_t const number, int8_t const value);

void set_cc_expression(uint32_t const tick, int8_t const voice, int8_t const value);

#endif // _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_
