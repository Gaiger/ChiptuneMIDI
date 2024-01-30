#ifndef _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_
#define _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_

#include <stdint.h>


int process_control_change_message(struct _channel_controller * const p_channel_controller,
								   uint32_t const tick, uint8_t const voice, uint8_t const number, uint8_t const value);

void reset_channel_controller(struct _channel_controller * const p_channel_controller);
#endif // _CHIPTUNE_MIDI_CONTROL_CHANGE_INTERNAL_H_
