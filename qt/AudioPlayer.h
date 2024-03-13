#ifndef _AUDIOPLAYER_H_
#define _AUDIOPLAYER_H_

#include <QObject>
#include <QAudioOutput>
#include <QThread>

#include <QIODevice>
#include <QMutex>

#include "TuneManager.h"


class AudioPlayer : public QObject
{
	Q_OBJECT
public:
	enum CHANNEL_COUNTS
	{
		CHANNEL_COUNTS_1		= 1,
		CHANNEL_COUNTS_2		= 2,

		CHANNEL_COUNTS_MAX		= 255,
	}; Q_ENUM(CHANNEL_COUNTS)

	AudioPlayer(TuneManager *p_tune_manager, QObject *parent = nullptr);
	~AudioPlayer()  Q_DECL_OVERRIDE;

	void Play(bool const is_blocking = false);
	void Stop(void);

	QAudio::State GetState(void);
private slots:
	void HandleAudioNotify(void);
	void HandleAudioStateChanged(QAudio::State state);

private :
	void InitializeAudioResources(int const channel_counts, int const sampling_rate, int const sampling_size,
								  int fetching_wave_interval_in_milliseconds);
	void AppendWave(QByteArray wave_bytearray);
	void CleanAudioResources();
private:
	QAudioOutput * m_p_audio_output;
	QIODevice *m_p_audio_io_device;
	QMutex m_accessing_io_device_mutex;
	TuneManager *m_p_tune_manager;
};

#endif // _AUDIOPLAYER_H_
