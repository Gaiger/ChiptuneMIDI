#include <QDebug>

#if QT_VERSION_CHECK(6, 0, 0) > QT_VERSION
#include "AudioPlayerPrivateQt5.h"
#endif

#if QT_VERSION_CHECK(6, 0, 0) <= QT_VERSION
#include "AudioPlayerPrivateQt6.h"
#endif

#include "AudioPlayer.h"

/**********************************************************************************/

AudioPlayer::AudioPlayer(TuneManager *p_tune_manager, int fetching_wave_interval_in_milliseconds, QObject *parent)
	: QObject(parent),
    m_p_private(nullptr)
{
    m_p_private = new AudioPlayerPrivate(p_tune_manager, fetching_wave_interval_in_milliseconds, parent);
    QObject::connect(m_p_private, &AudioPlayerPrivate::StateChanged,
                     this, &AudioPlayer::StateChanged,
                     Qt::DirectConnection);
}

/**********************************************************************************/

AudioPlayer::~AudioPlayer(void)
{
    delete m_p_private;
    m_p_private = nullptr;
}

/**********************************************************************************/

void AudioPlayer::Play(void)
{
    m_p_private->Play();
}

/**********************************************************************************/

void AudioPlayer::Stop(void)
{
    m_p_private->Stop();
}

/**********************************************************************************/

void AudioPlayer::Pause(void)
{
    m_p_private->Pause();
}

/**********************************************************************************/

AudioPlayer::PlaybackState AudioPlayer::GetState(void)
{
    return m_p_private->GetState();
}
