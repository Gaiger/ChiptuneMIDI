#ifndef TUNEMANAGER_H
#define TUNEMANAGER_H

#include <QObject>
#include <QMutex>

#include "QMidiFile.h"

class TuneManagerPrivate;

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

	bool IsFileLoaded(void);
	int GetNumberOfChannels(void);
	int GetSamplingRate(void);
	int GetSamplingSize(void);

	QByteArray FetchWave(int const length);
	bool IsTuneEnding(void);

	float GetMidiFileDurationInSeconds(void);
	float GetCurrentElapsedTimeInSeconds(void);
	int SetStartTimeInSeconds(float target_start_time_in_seconds);

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
