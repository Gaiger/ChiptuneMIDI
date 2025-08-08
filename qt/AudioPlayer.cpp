#include <QDebug>
#include <QThread>
#include <QEventLoop>

#include "AudioPlayerPrivate.h"
#include "AudioPlayer.h"

/**********************************************************************************/

AudioPlayer::AudioPlayer(TuneManager *p_tune_manager, int fetching_wave_interval_in_milliseconds, QObject *parent)
	: QObject(parent),
    m_p_private(nullptr)
{
    m_p_private = new AudioPlayerPrivate(p_tune_manager, fetching_wave_interval_in_milliseconds, this);
    QObject::connect(m_p_private, &AudioPlayerPrivate::StateChanged,
                     this, &AudioPlayer::StateChanged,
                     Qt::DirectConnection);
}

/**********************************************************************************/

AudioPlayer::~AudioPlayer(void)
{
    do
    {
        if(QThread::currentThread() == m_p_private->thread()){
            delete m_p_private;
            break;
        }
        QEventLoop destroy_event_loop;
        QObject::connect(m_p_private, &QObject::destroyed, &destroy_event_loop, &QEventLoop::quit);
        m_p_private->deleteLater();
        destroy_event_loop.exec();
    }while(0);
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
