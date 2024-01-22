#ifndef _CHIPTUNE_COMMON_INTERNAL_H_
#define _CHIPTUNE_COMMON_INTERNAL_H_

#include <stdbool.h>
#include <stdint.h>
//#define _DEBUG_ANKOKU_BUTOUKAI_FAST_TO_ENDING

//#define _INCREMENTAL_SAMPLE_INDEX
//#define _RIGHT_SHIFT_FOR_NORMALIZING_AMPLITUDE


#define _PRINT_MIDI_DEVELOPING
#define _PRINT_MIDI_SETUP
#define _PRINT_MOTE_OPERATION


struct _voice_info
{
	int8_t		tuning_in_semitones;

	uint8_t		max_volume;
	uint8_t		playing_volume;
	uint8_t		pan;

	uint8_t		waveform;
	uint8_t		: 8;
	uint16_t	duty_cycle_critical_phase;

	uint16_t	pitch_bend_range_in_semitones;
	uint16_t	pitch_wheel;

	uint8_t		modulation_wheel;
	uint8_t		: 8;

	uint16_t	registered_parameter_number;
	uint16_t	registered_parameter_value;
};


struct _oscillator
{
	uint8_t		state_bits;
	uint8_t		: 8;

	int8_t		voice;

	uint8_t		note;
	uint16_t	delta_phase;
	uint16_t	current_phase;

	uint16_t	volume;

	uint8_t		waveform;
	uint8_t		: 8;
	uint16_t	duty_cycle_critical_phase;

	uint16_t	delta_vibration_phase;
	uint16_t	vibration_table_index;
	uint32_t	vibration_same_index_count;
};

#endif // _CHIPTUNE_COMMON_INTERNAL_H_
