#ifndef _AUDIOPLAYERPRIVATEQT5_H_
#define _AUDIOPLAYERPRIVATEQT5_H_

#include <QtGlobal>
#if QT_VERSION_CHECK(6, 0, 0) > QT_VERSION

#include <QObject>
#include <QAudioOutput>
#include <QTimer>

#include "TuneManager.h"
#include "AudioPlayer.h" //PlaybackState

class AudioPlayerPrivate : public QObject
{
    Q_OBJECT
public:
    AudioPlayerPrivate(TuneManager *p_tune_manager, int fetching_wave_interval_in_milliseconds = 100,
                QObject *parent = nullptr);
    ~AudioPlayerPrivate()  Q_DECL_OVERRIDE;

    void Play(void);
    void Stop(void);
    void Pause(void);

    AudioPlayer::PlaybackState GetState(void);
public:
    signals:
    void StateChanged(AudioPlayer::PlaybackState state);

private:
    signals:
    void PlayRequested(void);
    void StopRequested(void);
    void PauseRequested(void);

private slots:
    void HandlePlayRequested(void);
    void HandleStopRequested(void);
    void HandlePauseRequested(void);

private slots:
    void HandleRefillTimerTimeout(void);
    void HandleAudioStateChanged(QAudio::State state);

public:
    void InitializeAudioResources(int const number_of_channels, int const sampling_rate, int const sampling_size,
                                  int fetching_wave_interval_in_milliseconds);
    void AppendDataToAudioIODevice(QByteArray wave_bytearray);
    void ClearOutMidiFileAudioResources();

    void OrganizeConnection(void);

private:
    TuneManager *m_p_tune_manager;
    int m_fetching_wave_interval_in_milliseconds;

    QAudioOutput * m_p_audio_output;
    QIODevice *m_p_audio_io_device;
    QTimer *m_p_refill_timer;

    Qt::ConnectionType m_connection_type;
    //QMutex m_accessing_io_device_mutex;
    QMutex m_mutex;
};

#endif //QT_VERSION_CHECK
#endif
