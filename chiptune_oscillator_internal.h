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
	int16_t		amplitude;

union{
	struct {
		uint16_t	envelope_table_index;
		uint16_t	envelope_same_index_count;
		int16_t		release_reference_amplitude;

		uint16_t	max_delta_vibrato_phase;
		uint16_t	vibrato_table_index;
		uint16_t	vibrato_same_index_count;

		float		pitch_chorus_bend_in_semitone;
	};
	struct {
		int8_t		percussion_waveform_index;
		uint32_t	percussion_duration_sample_count;
		uint16_t	percussion_table_index;
		uint16_t	percussion_same_index_count;
	};
};
	int16_t			native_oscillator;
	int16_t			chorus_asscociate_oscillators[3];
	int16_t			reverb_asscociate_oscillators[3];
} oscillator_t;

#define UNUSED_OSCILLATOR							(-1)
#define RESET_STATE_BITES(STATE_BITES)				((STATE_BITES) = 0)

#define STATE_ACTIVATED_BIT							(0)
#define SET_ACTIVATED(STATE_BITS)					( (STATE_BITS) |= (0x01 << STATE_ACTIVATED_BIT) )
#define SET_DEACTIVATED(STATE_BITS)					( (STATE_BITS) &= (~(0x01 << STATE_ACTIVATED_BIT)) )
#define IS_ACTIVATED(STATE_BITS)					(((0x01 << STATE_ACTIVATED_BIT) & (STATE_BITS) ) ? true : false)

#define STATE_RESTING_BIT							(1)
#define SET_RESTING(STATE_BITS)						( (STATE_BITS) |= (0x01 << STATE_RESTING_BIT) )
#define IS_RESTING(STATE_BITS)						(((0x01 << STATE_RESTING_BIT) & (STATE_BITS) ) ? true : false)

#define STATE_FREEING_BIT							(2)
#define SET_FREEING(STATE_BITS)						( (STATE_BITS) |= ((0x01)<< STATE_FREEING_BIT))
#define IS_FREEING(STATE_BITS)						(((0x01 << STATE_FREEING_BIT) & (STATE_BITS)) ? true : false)

#define STATE_NOTE_BIT								(3)
#define SET_NOTE_ON(STATE_BITS)						( (STATE_BITS) |= (0x01 << STATE_NOTE_BIT) )
#define SET_NOTE_OFF(STATE_BITS)					( (STATE_BITS) &= (~(0x01 << STATE_NOTE_BIT)) )
#define IS_NOTE_ON(STATE_BITS)						(((0x01 << STATE_NOTE_BIT) & (STATE_BITS) ) ? true : false)

#define STATE_CHORUS_ASSOCIATE_BIT					(4)
#define SET_CHORUS_ASSOCIATE(STATE_BITS)			( (STATE_BITS) |= ((0x01)<< STATE_CHORUS_ASSOCIATE_BIT))
#define IS_CHORUS_ASSOCIATE(STATE_BITS)				(((0x01 << STATE_CHORUS_ASSOCIATE_BIT) & (STATE_BITS)) ? true : false)

#define STATE_REVERB_ASSOCIATE_BIT					(5)
#define SET_REVERB_ASSOCIATE(STATE_BITS)			( (STATE_BITS) |= ((0x01)<< STATE_REVERB_ASSOCIATE_BIT))
#define IS_REVERB_ASSOCIATE(STATE_BITS)				(((0x01 << STATE_REVERB_ASSOCIATE_BIT) & (STATE_BITS)) ? true : false)


uint16_t const calculate_oscillator_delta_phase(int16_t const note, int8_t tuning_in_semitones,
							   int8_t const pitch_wheel_bend_range_in_semitones, int16_t const pitch_wheel,
							   float const pitch_chorus_bend_in_semitones, float * const p_pitch_wheel_bend_in_semitone);

float const obtain_oscillator_pitch_chorus_bend_in_semitone(int8_t const chorus,
															float const max_pitch_chorus_bend_in_semitones);
#endif // _CHIPTUNE_OSCILLATOR_INTERNAL_H_
