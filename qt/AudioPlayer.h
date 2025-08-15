#ifndef _AUDIOPLAYER_H_
#define _AUDIOPLAYER_H_
#include <QObject>
#include "TuneManager.h"

class AudioPlayerPrivate;

class AudioPlayer : public QObject
{
	Q_OBJECT
public:
	explicit AudioPlayer(TuneManager *p_tune_manager, int fetching_wave_interval_in_milliseconds = 100,
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
	AudioPlayerPrivate *m_p_private;

};

#endif // _AUDIOPLAYER_H_
