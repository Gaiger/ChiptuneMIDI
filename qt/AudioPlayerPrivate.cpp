#include <QBuffer>
#include <QThread>

#include <QIODevice>

#include <QDebug>

#include "AudioPlayerPrivate.h"


#if QT_VERSION_CHECK(6, 0, 0) <= QT_VERSION
#include <QAudioSink>
#include <QMediaDevices>

class AudioPlayerOutput
{
public:
    template<typename Receiver, typename PointerToMemberFunction>
    AudioPlayerOutput(int const number_of_channels, int const sampling_rate, int const sampling_size,
                      Receiver *p_receiver, PointerToMemberFunction stateChanged_slot_method,
                      QObject *parent)
        : m_p_audio_sink(nullptr){
        QAudioFormat format;
        format.setChannelCount((int)number_of_channels);
        format.setSampleRate(sampling_rate);

        do {
            if(16 == sampling_size){
                format.setSampleFormat(QAudioFormat::Int16);
                break;
            }
            format.setSampleFormat(QAudioFormat::UInt8);
        } while(0);

        QAudioDevice info(QMediaDevices::defaultAudioOutput());
        qDebug() << info.supportedSampleFormats();
        if (false == info.isFormatSupported(format)) {
            qWarning()<<"raw audio format not supported by backend, cannot play audio.";
        }
        qDebug() << info.description();
        qDebug() << format;

        if(nullptr != m_p_audio_sink){
            delete m_p_audio_sink;
            m_p_audio_sink = nullptr;
        }
        m_p_audio_sink = new QAudioSink(info, format, parent);
        QObject::connect(m_p_audio_sink, &QAudioSink::stateChanged,
                             p_receiver, stateChanged_slot_method);
    };

    ~AudioPlayerOutput(){
        if(nullptr != m_p_audio_sink){
            delete m_p_audio_sink;
        }
        m_p_audio_sink = nullptr;
    };

    void Start(QIODevice *device){
        m_p_audio_sink->start(device);
    }
    void Suspend(void){
        m_p_audio_sink->suspend();
    }
    void Resume(void){
        m_p_audio_sink->resume();
    }
    void Stop(void){
        m_p_audio_sink->stop();
    }
    void Reset(void){
        m_p_audio_sink->reset();
    }
    void SetBufferSize(qsizetype value){
        m_p_audio_sink->setBufferSize(value);
    }
    qsizetype BufferSize() const{
        return m_p_audio_sink->bufferSize();
    }
    void SetVolume(qreal volume){
        m_p_audio_sink->setVolume(volume);
    }
    QAudio::State State() const{
        return m_p_audio_sink->state();}
    int	BytesFree() const{
        return m_p_audio_sink->bytesFree();
    }
    QAudio::Error Error() const{
        return m_p_audio_sink->error();
    }
    QAudioSink* GetAudioOutputInstance(void){return m_p_audio_sink;}
private:
    QAudioSink * m_p_audio_sink;
};
#endif

#if QT_VERSION_CHECK(6, 0, 0) > QT_VERSION
#include <QAudioOutput>

class AudioPlayerOutput
{
public:
    template<typename Receiver, typename PointerToMemberFunction>
    AudioPlayerOutput(int const number_of_channels, int const sampling_rate, int const sampling_size,
                      Receiver *p_receiver, PointerToMemberFunction stateChanged_slot_method,
                      QObject *parent)
        : m_p_audio_output(nullptr){
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
        if (false == info.isFormatSupported(format)) {
            qWarning()<<"raw audio format not supported by backend, cannot play audio.";
        }
        qDebug() << info.deviceName();
        qDebug() << format;

        if(nullptr != m_p_audio_output){
            delete m_p_audio_output;
            m_p_audio_output = nullptr;
        }
        m_p_audio_output = new QAudioOutput(info, format, parent);
        QObject::connect(m_p_audio_output, &QAudioOutput::stateChanged,
                         p_receiver, stateChanged_slot_method);
    };

    ~AudioPlayerOutput(){
        if(nullptr != m_p_audio_output){
            delete m_p_audio_output;
        }
        m_p_audio_output = nullptr;
    };

    void Start(QIODevice *device){
        m_p_audio_output->start(device);
    }
    void Suspend(void){
        m_p_audio_output->suspend();
    }
    void Resume(void){
        m_p_audio_output->resume();
    }
    void Stop(void){
        m_p_audio_output->stop();
    }
    void Reset(void){
        m_p_audio_output->reset();
    }
    void SetBufferSize(qsizetype value){
        m_p_audio_output->setBufferSize(value);
    }
    qsizetype BufferSize() const{
        return m_p_audio_output->bufferSize();
    }
    void SetVolume(qreal volume){
        m_p_audio_output->setVolume(volume);
    }
    QAudio::State State() const{
        return m_p_audio_output->state();}
    int	BytesFree() const{
        return m_p_audio_output->bytesFree();
    }
    QAudio::Error Error() const{
        return m_p_audio_output->error();
    }
    QAudioOutput* GetAudioOutputInstance(void){return m_p_audio_output;}
private:
    QAudioOutput * m_p_audio_output;
};
#endif

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

    m_p_audio_io_device(nullptr),
    m_p_refill_timer(nullptr),
    m_connection_type(Qt::AutoConnection),
    m_p_audio_player_output(nullptr)
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
        qRegisterMetaType<TuneManager::SamplingSize>("PlaybackState");
    } while(0);
}

/**********************************************************************************/

AudioPlayerPrivate::~AudioPlayerPrivate(void)
{
    AudioPlayerPrivate::Stop();
}

/**********************************************************************************/

void AudioPlayerPrivate::ClearOutMidiFileAudioResources(void)
{
    if(nullptr != m_p_refill_timer){
        m_p_refill_timer->stop();
        delete m_p_refill_timer;
    }
    m_p_refill_timer = nullptr;

    if(nullptr != m_p_audio_player_output){
        m_p_audio_player_output->Stop();
        delete m_p_audio_player_output;
    }
    m_p_audio_player_output = nullptr;

    if(nullptr != m_p_audio_io_device){
        delete m_p_audio_io_device;
    }
    m_p_audio_io_device = nullptr;
}

/**********************************************************************************/

void AudioPlayerPrivate::InitializeAudioResources(void)
{
    ClearOutMidiFileAudioResources();

    int const number_of_channels = m_p_tune_manager->GetNumberOfChannels();
    int const sampling_rate = m_p_tune_manager->GetSamplingRate();
    int const sampling_size = m_p_tune_manager->GetSamplingSize();
    int const fetching_wave_interval_in_milliseconds = m_fetching_wave_interval_in_milliseconds;

    m_p_audio_player_output = new AudioPlayerOutput(number_of_channels, sampling_rate, sampling_size,
                                                    this, &AudioPlayerPrivate::HandleAudioStateChanged,
                                                    this);
    m_p_audio_player_output->SetVolume(1.00);

    int audio_buffer_size = 2.0 * fetching_wave_interval_in_milliseconds
            * number_of_channels * sampling_rate * sampling_size/8/1000;

    m_p_audio_player_output->SetBufferSize(audio_buffer_size);
    qDebug() <<" m_p_audio_player_output->BufferSize = " << m_p_audio_player_output->BufferSize();

    m_p_audio_io_device = new AudioIODevice();
    m_p_audio_io_device->open(QIODevice::ReadWrite);

    m_p_refill_timer = new QTimer(this);
    m_p_refill_timer->setInterval(fetching_wave_interval_in_milliseconds);
    QObject::connect(m_p_refill_timer, &QTimer::timeout, this, &AudioPlayerPrivate::HandleRefillTimerTimeout);
    m_p_refill_timer->start();
}

/**********************************************************************************/

void AudioPlayerPrivate::HandlePlayRequested(void)
{
    do {
        if(nullptr != m_p_audio_io_device){
            if(QAudio::ActiveState == m_p_audio_player_output->State()){
                qDebug() << Q_FUNC_INFO << "Playing, ingore";
                return ;
            }

            if(QAudio::SuspendedState == m_p_audio_player_output->State())
            {
                qDebug() << Q_FUNC_INFO << "Resume play";
                m_p_audio_player_output->Resume();
                return ;
            }
        }

        qDebug() << Q_FUNC_INFO << "Start Play";

        InitializeAudioResources();
        AudioPlayerPrivate::AppendDataToAudioIODevice(
                    m_p_tune_manager->FetchWave(m_p_audio_player_output->BufferSize()));
        m_p_audio_player_output->Start(m_p_audio_io_device);
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
    AudioPlayerPrivate::ClearOutMidiFileAudioResources();
}

/**********************************************************************************/

void AudioPlayerPrivate::HandlePauseRequested(void)
{
    if(nullptr != m_p_audio_player_output){
        m_p_audio_player_output->Suspend();
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

void AudioPlayerPrivate::AppendDataToAudioIODevice(QByteArray audio_bytearray)
{
    if(nullptr == m_p_audio_io_device){
        return ;
    }
    m_p_audio_io_device->write(audio_bytearray);
    //qDebug() <<Q_FUNC_INFO <<"QObject::thread() = " << QObject::thread();
}

/**********************************************************************************/

void AudioPlayerPrivate::HandleRefillTimerTimeout(void)
{
    do
    {
        if(QAudio::ActiveState != m_p_audio_player_output->State()){
            break;
        }

        int remain_audio_buffer_size = m_p_audio_player_output->BytesFree();
        if(0 == remain_audio_buffer_size){
            break;
        }

        QByteArray fetched_bytearray
                = m_p_tune_manager->FetchWave(remain_audio_buffer_size);
        AudioPlayerPrivate::AppendDataToAudioIODevice(fetched_bytearray);
    } while(0);
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
        if (QAudio::NoError != m_p_audio_player_output->Error() ) {
            // Error handling
        }
        break;
#if QT_VERSION_CHECK(6, 0, 0) > QT_VERSION
    case QAudio::InterruptedState:
        break;
#endif
    default:
        // ... other cases as appropriate
        break;
    }
    emit StateChanged(GetState());
}

/**********************************************************************************/

AudioPlayer::PlaybackState AudioPlayerPrivate::GetState(void)
{
    AudioPlayer::PlaybackState state = AudioPlayer::PlaybackStateStateStopped;
    do
    {
        if(nullptr == m_p_audio_player_output){
            break;
        }

        switch(m_p_audio_player_output->State()){
        case QAudio::ActiveState:
#if QT_VERSION_CHECK(6, 0, 0) > QT_VERSION
        case QAudio::InterruptedState:
            state = AudioPlayer::PlaybackStateStatePlaying;
            break;
#endif
        case QAudio::SuspendedState:
            state = AudioPlayer::PlaybackStateStatePaused;
            break;
        case QAudio::IdleState:
            state = AudioPlayer::PlaybackStateStateIdle;
        default:
            break;
        }
    }while (0);
    return state;
}
