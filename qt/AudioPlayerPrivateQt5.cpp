#include <QtGlobal>
#if QT_VERSION_CHECK(6, 0, 0) >= QT_VERSION

#include <QBuffer>
#include <QThread>

#include <QIODevice>

#include <QDebug>

#include "AudioPlayerPrivateQt5.h"


class AudioIODevice: public QIODevice
{
public :
    AudioIODevice(void){ }

protected :
    qint64 readData(char *data, qint64 maxlen) Q_DECL_OVERRIDE
    {
        int read_size = maxlen;
        if(m_audio_data_bytearray.size() < read_size){
            read_size = m_audio_data_bytearray.size();
        }
        memcpy(data, m_audio_data_bytearray.data(), read_size);
        m_audio_data_bytearray.remove(0, read_size);
        return read_size;
    }

    qint64 writeData(const char *data, qint64 len) Q_DECL_OVERRIDE
    {
        m_audio_data_bytearray.append(QByteArray(data, len));
        return len;
    }

    qint64 bytesAvailable() const Q_DECL_OVERRIDE
    {
        return m_audio_data_bytearray.size() + QIODevice::bytesAvailable();
    }
private:
        QByteArray m_audio_data_bytearray;
};

/**********************************************************************************/

AudioPlayerPrivate::AudioPlayerPrivate(TuneManager *p_tune_manager, int fetching_wave_interval_in_milliseconds,
                                       QObject *parent)
    : QObject(parent),
    m_p_tune_manager(p_tune_manager),
    m_fetching_wave_interval_in_milliseconds(fetching_wave_interval_in_milliseconds),

    m_p_audio_output(nullptr), m_p_audio_io_device(nullptr),
    m_connection_type(Qt::AutoConnection)
{
    if( QMetaType::UnknownType == QMetaType::type("PlaybackState")){
            qRegisterMetaType<TuneManager::SamplingSize>("PlaybackState");
    }
}

/**********************************************************************************/

AudioPlayerPrivate::~AudioPlayerPrivate(void)
{
    AudioPlayerPrivate::Stop();
}

/**********************************************************************************/

void AudioPlayerPrivate::ClearOutMidiFileAudioResources(void)
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

void AudioPlayerPrivate::InitializeAudioResources(int const number_of_channels, int const sampling_rate, int const sampling_size,
                                           int const fetching_wave_interval_in_milliseconds)
{
    ClearOutMidiFileAudioResources();
    QAudioFormat format;
    format.setChannelCount((int)number_of_channels);
    format.setSampleRate(sampling_rate);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);

    do {
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
    QObject::connect(m_p_audio_output, &QAudioOutput::notify, this, &AudioPlayerPrivate::HandleAudioNotify);
    QObject::connect(m_p_audio_output, &QAudioOutput::stateChanged, this, &AudioPlayerPrivate::HandleAudioStateChanged);
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

void AudioPlayerPrivate::HandlePlayRequested(void)
{
    //QMutexLocker lock(&m_accessing_io_device_mutex);
    do {
        if(nullptr != m_p_audio_output){
            if(QAudio::ActiveState == m_p_audio_output->state()){
                qDebug() << Q_FUNC_INFO << "Playing, ingore";
                return ;
            }

            if(QAudio::SuspendedState == m_p_audio_output->state())
            {
                qDebug() << Q_FUNC_INFO << "Resume play";
                m_p_audio_output->resume();
                return ;
            }
        }

        qDebug() << Q_FUNC_INFO << "Start Play";

        InitializeAudioResources(m_p_tune_manager->GetNumberOfChannels(),
                                 m_p_tune_manager->GetSamplingRate(), m_p_tune_manager->GetSamplingSize(),
                                 m_fetching_wave_interval_in_milliseconds);
        AudioPlayerPrivate::AppendWave(m_p_tune_manager->FetchWave(m_p_audio_output->bufferSize()));
        m_p_audio_output->start(m_p_audio_io_device);
    } while(0);
}

/**********************************************************************************/

void AudioPlayerPrivate::OrganizeConnection(void)
{
    bool is_to_reconnect = false;
    do{
        if( QObject::thread() == QThread::currentThread()){
            if(Qt::DirectConnection != m_connection_type){
                m_connection_type = Qt::DirectConnection;
                is_to_reconnect = true;
            }
            break;
        }

        if(Qt::BlockingQueuedConnection != m_connection_type){
            m_connection_type = Qt::BlockingQueuedConnection;
            is_to_reconnect = true;
        }
    }while(0);

    if(true == is_to_reconnect){
        QObject::disconnect(this, &AudioPlayerPrivate::PlayRequested,
                            this, &AudioPlayerPrivate::HandlePlayRequested);
        QObject::connect(this, &AudioPlayerPrivate::PlayRequested,
                         this, &AudioPlayerPrivate::HandlePlayRequested, m_connection_type);

        QObject::disconnect(this, &AudioPlayerPrivate::StopRequested,
                            this, &AudioPlayerPrivate::HandleStopRequested);
        QObject::connect(this, &AudioPlayerPrivate::StopRequested,
                         this, &AudioPlayerPrivate::HandleStopRequested, m_connection_type);

        QObject::disconnect(this, &AudioPlayerPrivate::PauseRequested,
                            this, &AudioPlayerPrivate::HandlePauseRequested);
        QObject::connect(this, &AudioPlayerPrivate::PauseRequested,
                         this, &AudioPlayerPrivate::HandlePauseRequested, m_connection_type);
    }
}

/**********************************************************************************/

void AudioPlayerPrivate::HandleStopRequested(void)
{
    //QMutexLocker lock(&m_accessing_io_device_mutex);
    AudioPlayerPrivate::ClearOutMidiFileAudioResources();
}
/**********************************************************************************/

void AudioPlayerPrivate::HandlePauseRequested(void)
{
    //QMutexLocker lock(&m_accessing_io_device_mutex);
    if(nullptr != m_p_audio_output){
        m_p_audio_output->suspend();
    }
}

/**********************************************************************************/

void AudioPlayerPrivate::Play(void)
{
    QMutexLocker locker(&m_mutex);
    OrganizeConnection();
    emit PlayRequested();
}

/**********************************************************************************/

void AudioPlayerPrivate::Stop(void)
{
    QMutexLocker locker(&m_mutex);
    OrganizeConnection();
    emit StopRequested();
}

/**********************************************************************************/

void AudioPlayerPrivate::Pause(void)
{
    QMutexLocker locker(&m_mutex);
    OrganizeConnection();
    emit PauseRequested();
}

/**********************************************************************************/

void AudioPlayerPrivate::AppendWave(QByteArray audio_bytearray)
{
    if(nullptr == m_p_audio_io_device){
        return ;
    }
    m_p_audio_io_device->write(audio_bytearray);
    //qDebug() <<Q_FUNC_INFO <<"QObject::thread() = " << QObject::thread();
}

/**********************************************************************************/

void AudioPlayerPrivate::HandleAudioNotify(void)
{
    //QMutexLocker lock(&m_accessing_io_device_mutex);
    //qDebug() << Q_FUNC_INFO  << "elapsed " <<
    //qDebug() <<Q_FUNC_INFO <<"QThread::currentThread() = " << QThread::currentThread();
    //qDebug() << Q_FUNC_INFO  << "elapsed " <<
    //			m_p_audio_output->elapsedUSecs()/1000.0/1000.0
    //		 << "seconds";
    if(QAudio::ActiveState != m_p_audio_output->state()){
        return ;
    }

    int remain_audio_buffer_size = m_p_audio_output->bytesFree();
    if(0 == remain_audio_buffer_size){
        return ;
    }

    QByteArray fetched_bytearray
            = m_p_tune_manager->FetchWave(remain_audio_buffer_size);
    AudioPlayerPrivate::AppendWave(fetched_bytearray);
}

/**********************************************************************************/

void AudioPlayerPrivate::HandleAudioStateChanged(QAudio::State state)
{
    //qDebug() <<Q_FUNC_INFO <<"QThread::currentThread() = " << QThread::currentThread();
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
    emit StateChanged(GetState());
}

/**********************************************************************************/

AudioPlayer::PlaybackState AudioPlayerPrivate::GetState(void)
{
    if(0 == m_p_audio_output){
        return AudioPlayer::PlaybackStateStateStopped;
    }

    AudioPlayer::PlaybackState state = AudioPlayer::PlaybackStateStateStopped;
    switch(m_p_audio_output->state()){
    case QAudio::ActiveState:
    case QAudio::InterruptedState:
        state = AudioPlayer::PlaybackStateStatePlaying;
        break;
    case QAudio::SuspendedState:
        state = AudioPlayer::PlaybackStateStatePaused;
        break;
    case QAudio::IdleState:
        state = AudioPlayer::PlaybackStateStateIdle;
    default:
        break;
    }
    return state;
}

#endif //QT_VERSION_CHECK
