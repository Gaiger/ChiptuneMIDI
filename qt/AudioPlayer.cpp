#include <QDebug>
#include <QThread>

#include "AudioPlayerPrivate.h"
#include "AudioPlayer.h"

/**********************************************************************************/

AudioPlayer::AudioPlayer(int const number_of_channels, int const sampling_rate, int const sampling_size,
						 int const fetching_wave_interval_in_milliseconds, QObject *parent)
	: QObject(parent),
	m_p_private(new AudioPlayerPrivate(number_of_channels, sampling_rate, sampling_size,
									   fetching_wave_interval_in_milliseconds, this))
{
	do
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		if( false != QMetaType::fromName("AudioPlayer::PlaybackState").isValid()){
			break;
		}
#else
		if( QMetaType::UnknownType != QMetaType::type("AudioPlayer::PlaybackState")){
			break;
		}
#endif
		qRegisterMetaType<AudioPlayer::PlaybackState>("AudioPlayer::PlaybackState");
	} while(0);

	QObject::connect(m_p_private, &AudioPlayerPrivate::DataRequested,
					 this, &AudioPlayer::DataRequested, Qt::DirectConnection);
	QObject::connect(m_p_private, &AudioPlayerPrivate::StateChanged,
					 this, &AudioPlayer::StateChanged, Qt::DirectConnection);
}

/**********************************************************************************/

AudioPlayer::~AudioPlayer(void)
{
	delete m_p_private;
}

/**********************************************************************************/

void AudioPlayer::FeedData(QByteArray const &data)
{
	m_p_private->FeedData(data);
}

void AudioPlayer::Prime(void)
{
	m_p_private->Prime();
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
