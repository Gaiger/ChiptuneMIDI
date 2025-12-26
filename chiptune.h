#ifndef _CHIPTUNE_H_
#define _CHIPTUNE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*chiptune_get_midi_message_callback_t)(
		uint32_t index, uint32_t *p_tick,uint32_t *p_message);


void chiptune_initialize(bool const is_stereo, uint32_t const sampling_rate,
						 chiptune_get_midi_message_callback_t get_midi_message_callback);
void chiptune_finalize(void);
void chiptune_prepare_song(uint32_t const resolution);

uint8_t chiptune_fetch_8bit_wave(void);
int16_t chiptune_fetch_16bit_wave(void);
bool chiptune_is_tune_ending(void);

void chiptune_set_tempo(float const tempo);
float chiptune_get_tempo(void);
void chiptune_set_playing_speed_ratio(float const playing_speed_ratio);
float chiptune_get_playing_effective_tempo(void);

#define CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER			(16)
#define CHIPTUNE_INSTRUMENT_NOT_SPECIFIED			(-1)
#define CHIPTUNE_INSTRUMENT_UNUSED_CHANNEL			(-2)
int chiptune_get_ending_instruments(int8_t instrument_array[CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER]);

void chiptune_set_current_message_index(uint32_t const index);
uint32_t chiptune_get_current_tick(void);

void chiptune_set_channel_output_enabled(int8_t const channel_index, bool const is_enabled);


void chiptune_set_pitch_shift_in_semitones(int8_t pitch_shift_in_semitones);
int8_t chiptune_get_pitch_shift_in_semitones(void);

enum ChiptuneWaveformType
{
	ChiptuneWaveformSquareDutyCycle50	= 0,
	ChiptuneWaveformSquareDutyCycle25,
	ChiptuneWaveformSquareDutyCycle12_5,
	ChiptuneWaveformSquareDutyCycle75,
	ChiptuneWaveformTriangle,
	ChiptuneWaveformSaw,
	ChiptuneWaveformNoise,
};

enum ChiptuneEnvelopeCurveType
{
	ChiptuneEnvelopeCurveLinear			= 0,
	ChiptuneEnvelopeCurveExponential,
	ChiptuneEnvelopeCurveGaussian,
	ChiptuneEnvelopeCurveFermi,
};

int chiptune_set_pitch_channel_timbre(int8_t const channel_index, int8_t const waveform,
									  int8_t const envelope_attack_curve, float const envelope_attack_duration_in_seconds,
									  int8_t const envelope_decay_curve, float const envelope_decay_duration_in_seconds,
									  uint8_t const envelope_note_on_sustain_level,
									  int8_t const envelope_release_curve, float const envelope_release_duration_in_seconds,
									  uint8_t const envelope_damper_on_but_note_off_sustain_level,
									  int8_t const envelope_damper_on_but_note_off_sustain_curve,
									  float const envelope_damper_on_but_note_off_sustain_duration_in_seconds);


int32_t chiptune_get_amplitude_gain(void);
void chiptune_set_amplitude_gain(int32_t amplitude_gain);
#ifdef __cplusplus
}
#endif

#endif /* _CHIPTUNE_H_ */
