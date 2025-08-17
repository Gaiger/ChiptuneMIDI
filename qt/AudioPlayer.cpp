#include <QDebug>
#include <QThread>
#include <QEventLoop>

#include "AudioPlayerPrivate.h"
#include "AudioPlayer.h"

/**********************************************************************************/

AudioPlayer::AudioPlayer(int const number_of_channels, int const sampling_rate, int const sampling_size,
						 int const fetching_wave_interval_in_milliseconds, QObject *parent)
	: QObject(parent),
	m_p_private(nullptr)
{
	do
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		if( false !=  QMetaType::fromName("PlaybackState").isValid()){
			break;
		}
#else
		if( QMetaType::UnknownType != QMetaType::type("PlaybackState")){
			break;
		}
#endif
		qRegisterMetaType<AudioPlayer::PlaybackState>("PlaybackState");
	} while(0);

	m_p_private = new AudioPlayerPrivate(number_of_channels, sampling_rate, sampling_size,
										 fetching_wave_interval_in_milliseconds, this);
	QObject::connect(m_p_private, &AudioPlayerPrivate::DataRequested,
					 this, &AudioPlayer::DataRequested, Qt::DirectConnection);
	QObject::connect(m_p_private, &AudioPlayerPrivate::StateChanged,
					 this, &AudioPlayer::StateChanged, Qt::DirectConnection);
}

/**********************************************************************************/

AudioPlayer::~AudioPlayer(void)
{
	do {
		if(QThread::currentThread() == m_p_private->QObject::thread()){
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

void AudioPlayer::FeedData(const QByteArray& data)
{
	m_p_private->FeedData(data);
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
