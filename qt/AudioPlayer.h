#ifndef _AUDIOPLAYER_H_
#define _AUDIOPLAYER_H_

#include <QObject>
#include <QAudioOutput>
#include <QMutex>
#include "TuneManager.h"

class AudioPlayerPrivate;

class AudioPlayer : public QObject
{
	Q_OBJECT
public:
	AudioPlayer(TuneManager *p_tune_manager, int fetching_wave_interval_in_milliseconds = 100,
				QObject *parent = nullptr);
	~AudioPlayer()  Q_DECL_OVERRIDE;

	void Play(void);
	void Stop(void);
	void Pause(void);

	enum PlaybackState
	{
		PlaybackStateStateIdle				= 0,
		PlaybackStateStateStopped			= 1,
		PlaybackStateStatePlaying			= 2,
		PlaybackStateStatePaused			= 3,

		PlaybackStateStateMax				= 255,
	}; Q_ENUM(PlaybackState)

	PlaybackState GetState(void);
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
	void HandleAudioNotify(void);
	void HandleAudioStateChanged(QAudio::State state);

private :
	void InitializeAudioResources(int const number_of_channels, int const sampling_rate, int const sampling_size,
								  int fetching_wave_interval_in_milliseconds);
	void AppendWave(QByteArray wave_bytearray);
	void ClearOutMidiFileAudioResources();

	void OrganizeConnection(void);

private:
	TuneManager *m_p_tune_manager;
	int m_fetching_wave_interval_in_milliseconds;

	QAudioOutput * m_p_audio_output;
	QIODevice *m_p_audio_io_device;

	Qt::ConnectionType m_connection_type;
	//QMutex m_accessing_io_device_mutex;
	QMutex m_mutex;
};

#endif // _AUDIOPLAYER_H_
