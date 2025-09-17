#ifndef _TUNEMANAGER_H_
#define _TUNEMANAGER_H_

#include <QObject>
#include <QMutex>

#include "QMidiFile.h"

class TuneManagerPrivate;

#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL			(9)
#define MIDI_MAX_CHANNEL_NUMBER						(16)

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

	explicit TuneManager(bool is_stereo = true,
						 int const sampling_rate = 16000, int const sampling_size = SamplingSize8Bit,
						 QObject * parent = nullptr);
	~TuneManager(void);

	int LoadMidiFile(QString const midi_file_name_string);
	void ClearOutMidiFile(void);
	QMidiFile* GetMidiFilePointer(void);
	bool IsFileLoaded(void);
	int GetNumberOfChannels(void);
	int GetSamplingRate(void);
	int GetSamplingSize(void);
	int GetAmplitudeGain(void);
	void SetAmplitudeGain(int amplitude_gain);

	QByteArray FetchWave(int const size);
	bool IsTuneEnding(void);

	float GetMidiFileDurationInSeconds(void);
	float GetCurrentElapsedTimeInSeconds(void);
	int GetCurrentTick(void);
	double GetTempo(void);
	double GetPlayingEffectiveTempo(void);
	void SetPlayingSpeedRatio(double playing_speed_raio);
	void SetPitchShift(int pitch_shift_in_semitones);

	int SetStartTimeInSeconds(float target_start_time_in_seconds);

	QList<QPair<int, int>> GetChannelInstrumentPairList(void);
	void SetChannelOutputEnabled(int index, bool is_enabled);
	enum WaveformType
	{
		WAVEFORM_SQUARE_DUDYCYCLE_50 = 0,
		WAVEFORM_SQUARE_DUDYCYCLE_25,
		WAVEFORM_SQUARE_DUDYCYCLE_125,
		WAVEFORM_SQUARE_DUDYCYCLE_75,
		WAVEFORM_TRIANGLE,
		WAVEFORM_SAW,
		WAVEFORM_NOISE,
	}; Q_ENUM(WaveformType)

	enum EnvelopeCurveType
	{
		ENVELOPE_CURVE_LINEAR = 0,
		ENVELOPE_CURVE_EXPONENTIAL,
		ENVELOPE_CURVE_GAUSSIAN,
		ENVELOPE_CURVE_FERMI,
	}; Q_ENUM(EnvelopeCurveType)

	int SetPitchChannelTimbre(int8_t const channel_index,
							  int8_t const waveform = WAVEFORM_TRIANGLE,
							  int8_t const envelope_attack_curve = ENVELOPE_CURVE_LINEAR, float const envelope_attack_duration_in_seconds = 0.02,
							  int8_t const envelope_decay_curve = ENVELOPE_CURVE_FERMI, float const envelope_decay_duration_in_seconds = 0.01,
							  uint8_t const envelope_sustain_level = 96,
							  int8_t const envelope_release_curve = ENVELOPE_CURVE_EXPONENTIAL, float const envelope_release_duration_in_seconds = 0.03,
							  uint8_t const envelope_damper_on_but_note_off_sustain_level = 24,
							  int8_t const envelope_damper_on_but_note_off_sustain_curve = ENVELOPE_CURVE_LINEAR,
							  float const envelope_damper_on_but_note_off_sustain_duration_in_seconds = 8.0);
public slots:
	void RequestWave(int const size);
public:
	signals:
	void WaveDelivered(QByteArray const &wave_bytearray);

private:
	signals:
	void GenerateWaveRequested(int const size);
private slots:
	void HandleGenerateWaveRequested(int const size);
private:
	void SubmitWaveGeneration(int const size, bool const is_synchronized = true);
private:
	TuneManagerPrivate *m_p_private;
};

#endif // _TUNEMANAGER_H_
