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
	explicit TuneManager(int sampliing_rate = 16000, QObject *parent = nullptr);
	~TuneManager(void);

	int SetMidiFile(QString midi_file_name_string);

	int GetSamplingRate(void);
	QByteArray FetchWave(int const length);

public:
	signals:
	void TuneEnded(void);
	void WaveFetched(QByteArray wave_bytearray);

private:
	signals:
	void GenerateWaveRequested(int length);
private slots:
	void HandleGenerateWaveRequested(int length);
private:
	void GenerateWave(int length, bool is_synchronized = true);
private:
	TuneManagerPrivate *m_p_private;
	QMutex m_mutex;
};

#endif // TUNEMANAGER_H
