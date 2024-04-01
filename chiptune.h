#ifndef _CHIPTUNE_H_
#define _CHIPTUNE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

void chiptune_set_midi_message_callback(
		int(*handler_get_midi_message)(uint32_t const index, uint32_t * const p_tick, uint32_t * const p_message) );

#define CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER			(16)
#define CHIPTUNE_INSTRUMENT_NOT_SPECIFIED			(-1)
#define CHIPTUNE_INSTRUMENT_UNUSED_CHANNEL			(-2)

void chiptune_initialize(bool const is_stereo, uint32_t const sampling_rate, uint32_t const resolution,
						 int8_t channel_instrument_array[CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER]);

void chiptune_set_tempo(float const tempo);

enum CHIPTUNE_WAVEFORM_TYPE
{
	CHIPTUNE_WAVEFORM_SQUARE_DUDYCYCLE_50 = 0,
	CHIPTUNE_WAVEFORM_SQUARE_DUDYCYCLE_25,
	CHIPTUNE_WAVEFORM_SQUARE_DUDYCYCLE_125,
	CHIPTUNE_WAVEFORM_SQUARE_DUDYCYCLE_75,
	CHIPTUNE_WAVEFORM_TRIANGLE,
	CHIPTUNE_WAVEFORM_SAW,
	CHIPTUNE_WAVEFORM_NOISE,
};

enum CHIPTUNE_ENVELOPE_CURVE_TYPE
{
	CHIPTUNE_ENVELOPE_CURVE_LINEAR = 0,
	CHIPTUNE_ENVELOPE_CURVE_EXPONENTIAL,
	CHIPTUNE_ENVELOPE_CURVE_GAUSSIAN,
	CHIPTUNE_ENVELOPE_CURVE_FERMI,
};

int chiptune_set_pitch_channel_timbre(int8_t const channel_index, int8_t const waveform,
									  int8_t const envelope_attack_curve, float const envelope_attack_duration_in_seconds,
									  int8_t const envelope_decay_curve, float const envelope_decay_duration_in_seconds,
									  uint8_t const envelope_sustain_level,
									  int8_t const envelope_release_curve, float const envelope_release_duration_in_seconds,
									  uint8_t const envelope_damper_on_but_note_off_sustain_level,
									  int8_t const envelope_damper_on_but_note_off_sustain_curve,
									  float const envelope_damper_on_but_note_off_sustain_duration_in_seconds);

uint8_t chiptune_fetch_8bit_wave(void);
int16_t chiptune_fetch_16bit_wave(void);
bool chiptune_is_tune_ending(void);

void chiptune_move_toward(uint32_t const index);
uint32_t chiptune_get_current_tick(void);
void chiptune_set_channel_output_enabled(int8_t const channel_index, bool const is_enabled);

int32_t chiptune_get_amplitude_gain(void);
void chiptune_set_amplitude_gain(int32_t amplitude_gain);

#ifdef __cplusplus
}
#endif

#endif /* _CHIPTUNE_H_ */
