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

	void Prime(void);
	void Play(void);
	void Stop(void);
	void Pause(void);

	enum PlaybackState
	{
		PlaybackStateIdle				= 0,
		PlaybackStateStopped			= 1,
		PlaybackStatePlaying			= 2,
		PlaybackStatePaused				= 3,

		PlaybackStateMax				= 255,
	}; Q_ENUM(PlaybackState)

	PlaybackState GetState(void);
public slots:
	void FeedData(QByteArray const &data);
public:
	signals:
	void DataRequested(int const size);
	void StateChanged(AudioPlayer::PlaybackState const state);

private:
	AudioPlayerPrivate * const m_p_private;
};

#endif // _AUDIOPLAYER_H_
