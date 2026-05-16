#ifndef _TUNEMANAGER_H_
#define _TUNEMANAGER_H_

#include <QObject>
#include <QMutex>

class MidSong;
class TuneManagerPrivate;

class MidiMessageProvider
 {
 public:
	virtual MidSong *GetMidSongPointer(void) const = 0;
	virtual ~MidiMessageProvider(void){};
 };


class TuneManager : public QObject
{
	Q_OBJECT
public:
	enum SamplingSize
	{
		SamplingSize8Bit			= 8,
		SamplingSize16Bit			= 16,

		SamplingSizeMax				= 255,
	}; Q_ENUM(SamplingSize)

	explicit TuneManager(bool const is_stereo = true,
						 int const sampling_rate = 16000, int const sampling_size = SamplingSize8Bit,
						 QObject * parent = nullptr);
	~TuneManager(void);
	void SetMidiMessageProvider(MidiMessageProvider *p_midi_message_provider);
public:
	int GetNumberOfChannels(void);
	int GetSamplingRate(void);
	int GetSamplingSize(void);
	int GetCurrentChannelInstrument(int const channel_index);

public slots:
	void RequestWave(int const size);
public:
	signals:
	void WaveDelivered(QByteArray const &wave_bytearray);

public:
	enum WaveformType
	{
		WaveformSquareDutyCycle50	= 0,
		WaveformSquareDutyCycle25,
		WaveformSquareDutyCycle12_5,
		WaveformSquareDutyCycle75,
		WaveformTriangle,
		WaveformSaw,
		WaveformNoise,
	}; Q_ENUM(WaveformType)

	enum EnvelopeCurveType
	{
		EnvelopeCurveLinear			= 0,
		EnvelopeCurveExponential,
		EnvelopeCurveGaussian,
		EnvelopeCurveFermi,
	}; Q_ENUM(EnvelopeCurveType)

	int SetMelodicChannelTimbre(int8_t const channel_index,
							  int8_t const waveform = WaveformTriangle,
							  int8_t const envelope_attack_curve = EnvelopeCurveLinear, float const envelope_attack_duration_in_seconds = 0.02,
							  int8_t const envelope_decay_curve = EnvelopeCurveFermi, float const envelope_decay_duration_in_seconds = 0.01,
							  uint8_t const envelope_note_on_sustain_level = 96,
							  int8_t const envelope_release_curve = EnvelopeCurveExponential, float const envelope_release_duration_in_seconds = 0.03,
							  uint8_t const envelope_damper_sustain_level = 24,
							  int8_t const envelope_damper_sustain_curve = EnvelopeCurveLinear,
							  float const envelope_damper_sustain_duration_in_seconds = 8.0);

	void SetChannelOutputEnabled(int const channel_index, bool is_enabled);

public slots:
	int SendMidiMessage(uint32_t const midi_message);

public:
	QByteArray TakeWave(int const size);
	bool IsTuneEnding(void);

	float GetCurrentElapsedTimeInSeconds(void);
	int GetCurrentTick(void);
	double GetCurrentTempo(void);
	double GetPlayingEffectiveTempo(void);
	void SetPlayingSpeedRatio(double const playing_speed_raio);
	void SetPitchShift(int const pitch_shift_in_semitones);

	int SetStartTimeInSeconds(float const target_start_time_in_seconds);

	QList<QPair<int, int>> GetChannelInstrumentPairList(void);
private:
	signals:
	void GenerateWaveRequested(int const size);
private slots:
	void HandleGenerateWaveRequested(int const size);
private:
	QByteArray FetchWave(int const size);
	void SubmitWaveGeneration(int const size, bool const is_synchronized = true);
private:
	TuneManagerPrivate * const m_p_private;
};

#endif // _TUNEMANAGER_H_
