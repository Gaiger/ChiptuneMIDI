#ifndef _CHIPTUNE_OSCILLATOR_INTERNAL_H_
#define _CHIPTUNE_OSCILLATOR_INTERNAL_H_

#include <stdint.h>

enum EnvelopeState
{
	ENVELOPE_STATE_ATTACK,
	ENVELOPE_STATE_DECAY,
	ENVELOPE_STATE_SUSTAIN,
	ENVELOPE_STATE_RELEASE,
	ENVELOPE_STATE_MAX,
};

#define MAX_ASSOCIATE_OSCILLATOR_NUMBER				(6)

typedef struct _oscillator
{
	uint8_t		state_bits;
	uint8_t		envelope_state;

	int8_t		voice;

	int8_t		note;
	uint16_t	delta_phase;
	uint16_t	current_phase;

	int16_t		loudness;
	int16_t		amplitude;

union{
	struct {
		int16_t		associate_oscillators[MAX_ASSOCIATE_OSCILLATOR_NUMBER];

		uint16_t	envelope_table_index;
		uint16_t	envelope_same_index_count;
		int16_t		release_reference_amplitude;
		int16_t		attack_decay_reference_amplitude;

		uint16_t	max_delta_vibrato_phase;
		uint16_t	vibrato_table_index;
		uint16_t	vibrato_same_index_count;

		float		pitch_chorus_bend_in_semitones;
	};
	struct {
		int8_t		percussion_waveform_index;
		uint32_t	percussion_duration_sample_count;
		uint16_t	percussion_table_index;
		uint16_t	percussion_same_index_count;
	};
};
} oscillator_t;

#define UNOCCUPIED_OSCILLATOR							(-1)
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

#define STATE_REVERB_ASSOCIATE_BIT					(6)
#define SET_REVERB_ASSOCIATE(STATE_BITS)			( (STATE_BITS) |= ((0x01)<< STATE_REVERB_ASSOCIATE_BIT))
#define IS_REVERB_ASSOCIATE(STATE_BITS)				(((0x01 << STATE_REVERB_ASSOCIATE_BIT) & (STATE_BITS)) ? true : false)

#define STATE_CHORUS_ASSOCIATE_BIT					(7)
#define SET_CHORUS_ASSOCIATE(STATE_BITS)			( (STATE_BITS) |= ((0x01)<< STATE_CHORUS_ASSOCIATE_BIT))
#define IS_CHORUS_ASSOCIATE(STATE_BITS)				(((0x01 << STATE_CHORUS_ASSOCIATE_BIT) & (STATE_BITS)) ? true : false)


#define IS_NATIVE_OSCILLATOR(STATE_BITS)			((IS_REVERB_ASSOCIATE(STATE_BITS) || IS_CHORUS_ASSOCIATE(STATE_BITS)) ? false : true)

uint16_t const calculate_oscillator_delta_phase(int8_t const voice,
												int16_t const note, float const pitch_chorus_bend_in_semitones);

float const obtain_oscillator_pitch_chorus_bend_in_semitones(int8_t const chorus,
															float const max_pitch_chorus_bend_in_semitones);
void set_pitch_shift_in_semitones(int16_t pitch_shift_in_semitones);
int16_t get_pitch_shift_in_semitones(void);
int setup_envelope_state(oscillator_t *p_oscillator, uint8_t evelope_state);

int discard_oscillator(int16_t const index);
int mark_all_oscillators_and_links_unused(void);
int release_all_oscillators_and_links(void);

oscillator_t * const acquire_freed_oscillator(int16_t * const p_index);

int16_t const get_occupied_oscillator_number(void);
int16_t const get_occupied_oscillator_head_index();
int16_t const get_occupied_oscillator_next_index(int16_t const index);
oscillator_t * const get_oscillator_pointer_from_index(int16_t const index);

#endif // _CHIPTUNE_OSCILLATOR_INTERNAL_H_
