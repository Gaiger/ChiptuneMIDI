#ifndef _AUDIOPLAYER_H_
#define _AUDIOPLAYER_H_

#include <QObject>

class AudioPlayerPrivate;

class AudioPlayer : public QObject
{
	Q_OBJECT
public:
	explicit AudioPlayer(int const number_of_channels, int const sampling_rate, int const sampling_size,
						 int const fetching_wave_interval_in_milliseconds = 100,
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
public slots:
	void FeedData(const QByteArray& data);
public:
	signals:
	void DataRequested(int size);
	void StateChanged(AudioPlayer::PlaybackState state);

private:
	AudioPlayerPrivate *m_p_private;
};

#endif // _AUDIOPLAYER_H_
