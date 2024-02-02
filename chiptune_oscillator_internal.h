#ifndef _CHIPTUNE_OSCILLATOR_INTERNAL_H_
#define _CHIPTUNE_OSCILLATOR_INTERNAL_H_

#include <stdint.h>

enum EnvelopeType
{
	ENVELOPE_ATTACK,
	ENVELOPE_DECAY,
	ENVELOPE_SUSTAIN,
	ENVELOPE_RELEASE,
};

typedef struct _oscillator
{
	uint8_t		state_bits;
	uint8_t		envelope_state;

	int8_t		voice;

	int8_t		note;
	uint16_t	delta_phase;
	uint16_t	current_phase;

	int16_t		loudness;

	float		pitch_chorus_bend_in_semitone;

	uint16_t	delta_vibrato_phase;
	uint16_t	vibrato_table_index;
	uint16_t	vibrato_same_index_count;

	int16_t		amplitude;
	uint16_t	envelope_table_index;
	uint16_t	envelope_same_index_count;

	int16_t		release_reference_amplitude;

	int16_t		native_oscillator;
} oscillator_t;

#define UNUSED_OSCILLATOR							(-1)
#define RESET_STATE_BITES(STATE_BITES)				((STATE_BITES) = 0)

#define STATE_ACTIVATED_BIT							(0)
#define SET_ACTIVATED_ON(STATE_BITS)				( (STATE_BITS) |= (0x01 << STATE_ACTIVATED_BIT) )
#define SET_ACTIVATED_OFF(STATE_BITS)				( (STATE_BITS) &= (~(0x01 << STATE_ACTIVATED_BIT)) )
#define IS_ACTIVATED(STATE_BITS)					(((0x01 << STATE_ACTIVATED_BIT) & (STATE_BITS) ) ? true : false)

#define STATE_NOTE_BIT								(1)
#define SET_NOTE_ON(STATE_BITS)						( (STATE_BITS) |= (0x01 << STATE_NOTE_BIT) )
#define SET_NOTE_OFF(STATE_BITS)					( (STATE_BITS) &= (~(0x01 << STATE_NOTE_BIT)) )
#define IS_NOTE_ON(STATE_BITS)						(((0x01 << STATE_NOTE_BIT) & (STATE_BITS) ) ? true : false)

#define STATE_CHORUS_OSCILLATOR_BIT					(2)
#define SET_CHORUS_OSCILLATOR(STATE_BITS)			( (STATE_BITS) |= ((0x01)<< STATE_CHORUS_OSCILLATOR_BIT))
//#define RESET_CHORUS_OSCILLATOR(STATE_BITS)			( (STATE_BITS) &= (~((0x01)<< STATE_CHORUS_OSCILLATOR_BIT)))
#define IS_CHORUS_OSCILLATOR(STATE_BITS)			(((0x01 << STATE_CHORUS_OSCILLATOR_BIT) & (STATE_BITS)) ? true : false)

void reset_all_oscillators(void);

oscillator_t * const acquire_oscillator(int16_t * const p_index);
int discard_oscillator(int16_t const index);

int16_t const get_occupied_oscillator_number(void);
int16_t get_head_occupied_oscillator_index();
int16_t get_next_occupied_oscillator_index(int16_t const index);
oscillator_t * const get_oscillator_pointer_from_index(int16_t const index);

#endif // _CHIPTUNE_OSCILLATOR_INTERNAL_H_
