#ifndef _CHIPTUNEMIDIOUT_H_
#define _CHIPTUNEMIDIOUT_H_

#include <stdint.h>

class ChiptuneMidiOutPrivate;

class ChiptuneMidiOut
{
public:
	explicit ChiptuneMidiOut(bool const is_stereo = true,
							 int const sampling_rate = 44100,
							 int const sampling_size = 16,
							 int const audio_player_buffer_in_milliseconds = 30);
	~ChiptuneMidiOut(void);

	void Start(void);
	void Stop(void);
	int SendMidiMessage(uint32_t const midi_message);
	void SetPitchShift(int const pitch_shift_in_semitones);

	enum WaveformType
	{
		WaveformSquareDutyCycle50	= 0,
		WaveformSquareDutyCycle25,
		WaveformSquareDutyCycle12_5,
		WaveformSquareDutyCycle75,
		WaveformTriangle,
		WaveformSaw,
		WaveformNoise,
	};

	enum EnvelopeCurveType
	{
		EnvelopeCurveLinear			= 0,
		EnvelopeCurveExponential,
		EnvelopeCurveGaussian,
		EnvelopeCurveFermi,
	};

	int SetMelodicChannelTimbre(int8_t const channel_index,
								int8_t const waveform = WaveformTriangle,
								int8_t const envelope_attack_curve = EnvelopeCurveLinear,
								float const envelope_attack_duration_in_seconds = 0.02f,
								int8_t const envelope_decay_curve = EnvelopeCurveFermi,
								float const envelope_decay_duration_in_seconds = 0.01f,
								uint8_t const envelope_note_on_sustain_level = 96,
								int8_t const envelope_release_curve = EnvelopeCurveExponential,
								float const envelope_release_duration_in_seconds = 0.03f,
								uint8_t const envelope_damper_sustain_level = 24,
								int8_t const envelope_damper_sustain_curve = EnvelopeCurveLinear,
								float const envelope_damper_sustain_duration_in_seconds = 8.0f);

private:
	ChiptuneMidiOutPrivate * const m_p_private;
};

#endif // _CHIPTUNEMIDIOUT_H_
