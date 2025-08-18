#ifndef _AUDIOPLAYERPRIVATE_H_
#define _AUDIOPLAYERPRIVATE_H_

#include <QMutex>
#include <QTimer>
#include <QObject>
#include <QAudio>
#include <QIODevice>

#include "AudioPlayer.h" //PlaybackState

class AudioPlayerOutput;

class AudioPlayerPrivate : public QObject
{
	Q_OBJECT
public:
	explicit AudioPlayerPrivate(int const number_of_channels, int const sampling_rate, int const sampling_size,
								int const fetching_wave_interval_in_milliseconds = 100,
								QObject *parent = nullptr);
	~AudioPlayerPrivate()  Q_DECL_OVERRIDE;

	void Play(void);
	void Stop(void);
	void Pause(void);

	AudioPlayer::PlaybackState GetState(void);
	void FeedData(const QByteArray& data);
public:
	signals:
	void DataRequested(int size);
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
	void ClearOutMidiFileAudioResources();
	void OrganizeConnection(void);

private:
	int const m_number_of_channels;
	int const m_sampling_rate;
	int const m_sampling_size;
	int const m_fetching_wave_interval_in_milliseconds;

	QIODevice *m_p_audio_io_device;
	QTimer *m_p_refill_timer;

	Qt::ConnectionType m_connection_type;
	QMutex m_mutex;

private:
	AudioPlayerOutput *m_p_audio_player_output;
};

#endif // _AUDIOPLAYERPRIVATE_H_
