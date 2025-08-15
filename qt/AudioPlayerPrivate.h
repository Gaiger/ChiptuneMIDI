#ifndef _AUDIOPLAYERPRIVATE_H_
#define _AUDIOPLAYERPRIVATE_H_

#include <QObject>
#include <QTimer>
#include <QIODevice>
#include <QAudio>

#include "TuneManager.h"
#include "AudioPlayer.h" //PlaybackState

class AudioPlayerOutput;

class AudioPlayerPrivate : public QObject
{
	Q_OBJECT
public:
	explicit AudioPlayerPrivate(TuneManager *p_tune_manager, int fetching_wave_interval_in_milliseconds = 100,
				QObject *parent = nullptr);
	~AudioPlayerPrivate()  Q_DECL_OVERRIDE;

	void Play(void);
	void Stop(void);
	void Pause(void);

	AudioPlayer::PlaybackState GetState(void);
public:
	signals:
	void StateChanged(AudioPlayer::PlaybackState state);

private:
	signals:
	void PlayRequested(void);
	void StopRequested(void);
	void PauseRequested(void);

private slots:
	void HandlePlayRequested(void);
	void HandleStopRequested(void);
	void HandlePauseRequested(void);

private slots:
	void HandleRefillTimerTimeout(void);
	void HandleAudioStateChanged(QAudio::State state);

private:
	void InitializeAudioResources(void);
	void AppendDataToAudioIODevice(QByteArray wave_bytearray);
	void ClearOutMidiFileAudioResources();

	void OrganizeConnection(void);

private:
	TuneManager *m_p_tune_manager;
	int m_fetching_wave_interval_in_milliseconds;

	QIODevice *m_p_audio_io_device;
	QTimer *m_p_refill_timer;

	Qt::ConnectionType m_connection_type;
	QMutex m_mutex;

private:
	AudioPlayerOutput *m_p_audio_player_output;
};

#endif // _AUDIOPLAYERPRIVATE_H_
