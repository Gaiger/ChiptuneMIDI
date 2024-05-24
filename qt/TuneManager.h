#ifndef TUNEMANAGER_H
#define TUNEMANAGER_H

#include <QObject>
#include <QMutex>

#include "QMidiFile.h"

class TuneManagerPrivate;

#define MIDI_MAX_CHANNEL_NUMBER			(16)

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

	QByteArray FetchWave(int const length);
	bool IsTuneEnding(void);

	float GetMidiFileDurationInSeconds(void);
	float GetCurrentElapsedTimeInSeconds(void);
	int GetCurrentTick(void);
	double GetTempo(void);

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

public:
	signals:
	void WaveFetched(QByteArray const wave_bytearray);

private:
	signals:
	void GenerateWaveRequested(int const length);
private slots:
	void HandleGenerateWaveRequested(int const length);
private:
	void GenerateWave(int const length, bool const is_synchronized = true);
	int InitializeTune(void);
private:
	TuneManagerPrivate *m_p_private;
	QMutex m_mutex;
};

#endif // TUNEMANAGER_H
