#include <QBuffer>
#include <QEventLoop>

#include <QDebug>

#include "AudioPlayer.h"

class AudioIODevice: public QIODevice
{
public :
	AudioIODevice(void){ }

protected :
	qint64 readData(char *data, qint64 maxlen) Q_DECL_OVERRIDE
	{
		int read_size = maxlen;
		if(m_audio_data_bytearray.size() < read_size)
			read_size = m_audio_data_bytearray.size();

		memcpy(data, m_audio_data_bytearray.data(), read_size);
		m_audio_data_bytearray.remove(0, read_size);
		return read_size;
	}

	qint64 writeData(const char *data, qint64 len) Q_DECL_OVERRIDE
	{
		m_audio_data_bytearray.append(QByteArray(data, len));
		return len;
	}

private:
		QByteArray m_audio_data_bytearray;
};

/**********************************************************************************/

AudioPlayer::AudioPlayer(TuneManager *p_tune_manager, QObject *parent)
	: QObject(parent),
	m_p_audio_output(nullptr),
	m_p_audio_io_device(nullptr),
	m_p_tune_manager(p_tune_manager)
{

}

/**********************************************************************************/

AudioPlayer::~AudioPlayer(void)
{
	AudioPlayer::Stop();
	AudioPlayer::CleanAudioResources();
}

/**********************************************************************************/

void AudioPlayer::CleanAudioResources(void)
{
	if(nullptr != m_p_audio_output){
		m_p_audio_output->stop();
		m_p_audio_output->reset();
		delete m_p_audio_output;
	}
	m_p_audio_output = nullptr;

	if(nullptr != m_p_audio_io_device){
		delete m_p_audio_io_device;
	}
	m_p_audio_io_device = nullptr;
}

/**********************************************************************************/

void AudioPlayer::InitializeAudioResources(int const channel_counts, int const sampling_rate, int const sampling_size,
										   int const fetching_wave_interval_in_milliseconds)
{
	CleanAudioResources();
	QAudioFormat format;
	format.setChannelCount((int)channel_counts);
	format.setSampleRate(sampling_rate);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);

	do
	{
		if(16 == sampling_size){
			format.setSampleSize(16);
			format.setSampleType(QAudioFormat::SignedInt);
			break;
		}
		format.setSampleSize(8);
		format.setSampleType(QAudioFormat::UnSignedInt);
	} while(0);

	QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
	qDebug() << info.supportedSampleRates();
	if (!info.isFormatSupported(format)) {
		qWarning()<<"raw audio format not supported by backend, cannot play audio.";
	}
	qDebug() << info.deviceName();
	qDebug() << format;


	if(nullptr != m_p_audio_output){
		delete m_p_audio_output;
		m_p_audio_output = nullptr;
	}
	m_p_audio_output = new QAudioOutput(info, format);
	QObject::connect(m_p_audio_output, &QAudioOutput::notify, this, &AudioPlayer::HandleAudioNotify);
	QObject::connect(m_p_audio_output, &QAudioOutput::stateChanged, this, &AudioPlayer::HandleAudioStateChanged);
	m_p_audio_output->setVolume(1.00);

	int audio_buffer_size = 2.0 * fetching_wave_interval_in_milliseconds
			* format.sampleRate() * format.channelCount() * format.sampleSize()/8/1000;
	m_p_audio_output->setNotifyInterval(fetching_wave_interval_in_milliseconds);

	m_p_audio_output->setBufferSize(audio_buffer_size);
	qDebug() <<" m_p_audio_output->bufferSize() = " << m_p_audio_output->bufferSize();

	m_p_audio_io_device = new AudioIODevice();
	m_p_audio_io_device->open(QIODevice::ReadWrite);
}

/**********************************************************************************/

void AudioPlayer::Play(bool const is_blocking)
{
	m_p_tune_manager->InitializeTune();

	InitializeAudioResources(m_p_tune_manager->GetNumberOfChannels(),
							 m_p_tune_manager->GetSamplingRate(), m_p_tune_manager->GetSamplingSize(), 100);
	AudioPlayer::AppendWave(m_p_tune_manager->FetchWave(m_p_audio_output->bufferSize()));
	m_p_audio_output->start(m_p_audio_io_device);

	do
	{
		if(false == is_blocking){
			break;
		}

		QEventLoop loop;
		QObject::connect(m_p_audio_output, SIGNAL(stateChanged(QAudio::State)), &loop, SLOT(quit()));
		do {
			loop.exec();
		} while(m_p_audio_output->state() == QAudio::ActiveState);
	}while(0);

}

/**********************************************************************************/

void AudioPlayer::AppendWave(QByteArray audio_bytearray)
{
	QMutexLocker lock(&m_accessing_io_device_mutex);
	if(nullptr == m_p_audio_io_device){
		return ;
	}
	m_p_audio_io_device->write(audio_bytearray);
}

/**********************************************************************************/

void AudioPlayer::Stop(void)
{
	QMutexLocker lock(&m_accessing_io_device_mutex);
	if(nullptr != m_p_audio_output){
		m_p_audio_output->stop();
	}
	AudioPlayer::CleanAudioResources();
}

/**********************************************************************************/

void AudioPlayer::HandleAudioNotify(void)
{
	//qDebug() << Q_FUNC_INFO  << "elapsed " <<
	//			m_p_audio_output->elapsedUSecs()/1000.0/1000.0
	//		 << "seconds";

	int remain_audio_buffer_size = m_p_audio_output->bytesFree();
	if(0 == remain_audio_buffer_size){
		return ;
	}

	QByteArray fetched_bytearray
			= m_p_tune_manager->FetchWave(remain_audio_buffer_size);
	AudioPlayer::AppendWave(fetched_bytearray);
}

/**********************************************************************************/

void AudioPlayer::HandleAudioStateChanged(QAudio::State state)
{
	qDebug() << Q_FUNC_INFO << state;
	switch (state)
	{
	case QAudio::ActiveState:
		break;
	case QAudio::SuspendedState:
		break;
	case QAudio::IdleState :
			// Finished playing (no more data)
			//m_p_audio_output->stop();
		break;

	case QAudio::StoppedState:
		// Stopped for other reasons
		if (m_p_audio_output->error() != QAudio::NoError) {
			// Error handling
		}
		break;

	case QAudio::InterruptedState:
		break;
	default:
		// ... other cases as appropriate
		break;
	}
}
