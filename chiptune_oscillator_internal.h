#ifndef _CHIPTUNE_OSCILLATOR_INTERNAL_H_
#define _CHIPTUNE_OSCILLATOR_INTERNAL_H_

#include <stdint.h>
#include <stdbool.h>
#include "chiptune_common_internal.h"

enum MidiEffectType
{
	MidiEffectNone		= 0,
	MidiEffectReverb	= (0x01 << 0),
	//MidiEffectTremolo	= (0x01 << 1),
	MidiEffectChorus	= (0x01 << 2),
	MidiEffectDetune	= (0x01 << 3),
	//MidiEffectPhaser	= (0x01 << 4),
	MidiEffectAll
		= (MidiEffectReverb | MidiEffectChorus | MidiEffectDetune),
};

typedef struct _oscillator
{
	uint8_t			state_bits;

	int8_t			voice;
	midi_value_t	note;

	uint16_t		base_phase_increment;
	uint16_t		current_phase;

	uint16_t		loudness; // [0, INT16_MAX_PLUS_1]
	uint16_t		amplitude; // [0, INT16_MAX_PLUS_1]
union{
	struct {
		uint8_t			envelope_state;

		int8_t const *  p_envelope_table;
		uint16_t		envelope_table_index;
		uint16_t		envelope_same_index_count;
		uint16_t		delta_amplitude;
		uint16_t		shift_amplitude;

		uint16_t	envelope_reference_amplitude;
		//uint16_t	envelope_damper_start_amplitude;

		uint16_t	max_vibrato_phase_increment;
		uint16_t	vibrato_table_index;
		uint16_t	vibrato_same_index_count;

		float		pitch_detune_in_semitones;

		uint8_t		midi_effect_association;
		int16_t		midi_effect_aassociate_link_index;//internal

		int32_t		mono_wave_amplitude;
	};
	struct {
		int8_t		percussion_waveform_segment_index;
		uint32_t	percussion_waveform_segment_duration_sample_count;
		uint16_t	percussion_envelope_table_index;
		uint16_t	percussion_envelope_same_index_count;
		int16_t		percussion_phase_sweep_delta;
	};
};
} oscillator_t;

#define UNOCCUPIED_OSCILLATOR						(-1)
#define IS_PERCUSSION_OSCILLATOR(OSCILLATOR_POINTER) \
	((MIDI_PERCUSSION_CHANNEL == (OSCILLATOR_POINTER)->voice) ? true : false)

#define RESET_STATE_BITES(STATE_BITES)				((STATE_BITES) = 0)

#define STATE_ACTIVATED_BIT							(0)
#define SET_ACTIVATED(STATE_BITS)					( (STATE_BITS) |= (0x01 << STATE_ACTIVATED_BIT) )
#define SET_DEACTIVATED(STATE_BITS)					( (STATE_BITS) &= (~(0x01 << STATE_ACTIVATED_BIT)) )
#define IS_ACTIVATED(STATE_BITS)					(((0x01 << STATE_ACTIVATED_BIT) & (STATE_BITS) ) ? true : false)


#define STATE_RESTING_BIT							(1)
#define SET_RESTING(STATE_BITS)						( (STATE_BITS) |= (0x01 << STATE_RESTING_BIT) )
#define IS_RESTING(STATE_BITS)						(((0x01 << STATE_RESTING_BIT) & (STATE_BITS) ) ? true : false)

#define STATE_PREPARE_TO_REST_BIT					(2)
#define SET_PREPARE_TO_REST(STATE_BITS)				( (STATE_BITS) |= ((0x01) << STATE_PREPARE_TO_REST_BIT))
#define IS_PREPARE_TO_REST(STATE_BITS)				(((0x01 << STATE_PREPARE_TO_REST_BIT) & (STATE_BITS)) ? true : false)

#define IS_RESTING_OR_PREPARE_TO_REST(STATE_BITS)	((IS_RESTING(STATE_BITS) || IS_PREPARE_TO_REST(STATE_BITS)) ? true : false)

#define STATE_FREEING_BIT							(3)
#define SET_FREEING(STATE_BITS)						( (STATE_BITS) |= ((0x01) << STATE_FREEING_BIT))
#define IS_FREEING(STATE_BITS)						(((0x01 << STATE_FREEING_BIT) & (STATE_BITS)) ? true : false)

#define STATE_PREPARE_TO_FREE_BIT					(4)
#define SET_PREPARE_TO_FREE(STATE_BITS)				( (STATE_BITS) |= ((0x01) << STATE_PREPARE_TO_FREE_BIT))
#define IS_PREPARE_TO_FREE(STATE_BITS)				(((0x01 << STATE_PREPARE_TO_FREE_BIT) & (STATE_BITS)) ? true : false)

#define IS_FREEING_OR_PREPARE_TO_FREE(STATE_BITS)	((IS_FREEING(STATE_BITS) || IS_PREPARE_TO_FREE(STATE_BITS)) ? true : false)

#define STATE_NOTE_BIT								(5)
#define SET_NOTE_ON(STATE_BITS)						( (STATE_BITS) |= (0x01 << STATE_NOTE_BIT) )
#define SET_NOTE_OFF(STATE_BITS)					( (STATE_BITS) &= (~(0x01 << STATE_NOTE_BIT)) )
#define IS_NOTE_ON(STATE_BITS)						(((0x01 << STATE_NOTE_BIT) & (STATE_BITS) ) ? true : false)

#define IS_PRIMARY_OSCILLATOR(OSCILLATOR_POINTER) \
	(((MidiEffectNone == (OSCILLATOR_POINTER)->midi_effect_association) \
	|| true == IS_PERCUSSION_OSCILLATOR(OSCILLATOR_POINTER)) ? true : false)

#define SINGLE_EFFECT_ASSOCIATE_OSCILLATOR_NUMBER	(4 - 1)

void set_pitch_shift_in_semitones(int16_t pitch_shift_in_semitones);
int16_t get_pitch_shift_in_semitones(void);

int const update_oscillator_phase_increment(oscillator_t * const p_oscillator);

oscillator_t * const acquire_oscillator(int16_t * const p_index);
oscillator_t * const replicate_oscillator(int16_t const original_index, int16_t * const p_index);
int discard_oscillator(int16_t const index);

int clear_all_oscillators(void);
int destroy_all_oscillators(void);

int16_t const get_occupied_oscillator_number(void);
int16_t const get_occupied_oscillator_head_index();
int16_t const get_occupied_oscillator_next_index(int16_t const index);

oscillator_t * const get_oscillator_pointer_from_index(int16_t const index);

#define WITHOUT_EFFECT(MIDI_EFFECT)   (MidiEffectAll & (~(MIDI_EFFECT)))

int store_associate_oscillator_indexes(uint8_t const midi_effect_type, int16_t const index,
									  int16_t const * const p_associate_indexes);
int16_t count_all_subordinate_oscillators(uint8_t const midi_effect_type, int16_t const root_index);
int get_all_subordinate_oscillator_indexes(uint8_t const midi_effect_type, int16_t const root_index,
										   int16_t * const p_associate_indexes);

#endif // _CHIPTUNE_OSCILLATOR_INTERNAL_H_
