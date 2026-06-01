#include <functional>

#include <QDebug>
#include <QObject>
#include <QThread>
#include <QTimer>

#include "AudioPlayer.h"
#include "TuneManager.h"

#include "ChiptuneMidiOut.h"

/**********************************************************************************/

#define CHIPTUNE_MIDI_OUT_AUDIO_PLAYER_THREAD_STARTUP_DELAY_IN_MILLISECONDS	(50)

class ChiptuneMidiOutPrivate
{
public:
	QObject *m_p_context;
	TuneManager *m_p_tune_manager;
	AudioPlayer *m_p_audio_player;
	QThread *m_p_tune_manager_working_thread;
	QThread *m_p_audio_player_working_thread;
};

/**********************************************************************************/

QString ChiptuneMidiOut::GetChiptuneEngineVersionString(void)
{
	return TuneManager::GetChiptuneEngineVersionString();
}

/**********************************************************************************/

ChiptuneMidiOut::ChiptuneMidiOut(bool const is_stereo,
								 int const sampling_rate,
								 int const sampling_size,
								 int const audio_player_buffer_in_milliseconds)
	: m_p_private(new ChiptuneMidiOutPrivate())
{
	m_p_private->m_p_context = new QObject();
	m_p_private->m_p_tune_manager = new TuneManager(is_stereo, sampling_rate, sampling_size);
	m_p_private->m_p_tune_manager_working_thread = new QThread();
	QObject::connect(m_p_private->m_p_tune_manager, &QObject::destroyed,
					 m_p_private->m_p_tune_manager_working_thread, &QThread::quit);
	m_p_private->m_p_tune_manager->QObject::moveToThread(m_p_private->m_p_tune_manager_working_thread);
	m_p_private->m_p_tune_manager_working_thread->QThread::start(QThread::HighPriority);

	m_p_private->m_p_audio_player = new AudioPlayer(m_p_private->m_p_tune_manager->GetNumberOfChannels(),
									   m_p_private->m_p_tune_manager->GetSamplingRate(),
									   m_p_private->m_p_tune_manager->GetSamplingSize(),
									   audio_player_buffer_in_milliseconds,
									   nullptr);
	QObject::connect(m_p_private->m_p_tune_manager, &TuneManager::WaveDelivered,
					 m_p_private->m_p_audio_player, &AudioPlayer::FeedData, Qt::DirectConnection);
	QObject::connect(m_p_private->m_p_audio_player, &AudioPlayer::DataRequested,
					 m_p_private->m_p_tune_manager, &TuneManager::RequestWave, Qt::DirectConnection);

	std::function<void(AudioPlayer::PlaybackState)> idle_to_restart_play =
			[this](AudioPlayer::PlaybackState state) {
		do
		{
			if(AudioPlayer::PlaybackStateIdle != state){
				break;
			}
			qInfo() << "ChiptuneMidiOut audio player idle, restarting playback";
			m_p_private->m_p_audio_player->Play();
		}while(0);
	};
	QObject::connect(m_p_private->m_p_audio_player, &AudioPlayer::StateChanged,
					 m_p_private->m_p_context, idle_to_restart_play, Qt::QueuedConnection);

	m_p_private->m_p_audio_player_working_thread = new QThread();
	QObject::connect(m_p_private->m_p_audio_player, &QObject::destroyed,
					 m_p_private->m_p_audio_player_working_thread, &QThread::quit);
	m_p_private->m_p_audio_player->QObject::moveToThread(m_p_private->m_p_audio_player_working_thread);
	m_p_private->m_p_audio_player_working_thread->QThread::start(QThread::NormalPriority);
}

/**********************************************************************************/

ChiptuneMidiOut::~ChiptuneMidiOut(void)
{
	Stop();

	if(nullptr != m_p_private->m_p_audio_player){
		m_p_private->m_p_audio_player->QObject::deleteLater();
		m_p_private->m_p_audio_player = nullptr;
	}
	if(nullptr != m_p_private->m_p_audio_player_working_thread){
		m_p_private->m_p_audio_player_working_thread->QThread::quit();
		m_p_private->m_p_audio_player_working_thread->QThread::wait();
		delete m_p_private->m_p_audio_player_working_thread;
		m_p_private->m_p_audio_player_working_thread = nullptr;
	}

	if(nullptr != m_p_private->m_p_tune_manager){
		m_p_private->m_p_tune_manager->QObject::deleteLater();
		m_p_private->m_p_tune_manager = nullptr;
	}
	if(nullptr != m_p_private->m_p_tune_manager_working_thread){
		m_p_private->m_p_tune_manager_working_thread->QThread::quit();
		m_p_private->m_p_tune_manager_working_thread->QThread::wait();
		delete m_p_private->m_p_tune_manager_working_thread;
		m_p_private->m_p_tune_manager_working_thread = nullptr;
	}

	if(nullptr != m_p_private->m_p_context){
		delete m_p_private->m_p_context;
		m_p_private->m_p_context = nullptr;
	}
	delete m_p_private;
}

/**********************************************************************************/

void ChiptuneMidiOut::Start(void)
{
	if(nullptr == m_p_private->m_p_audio_player){
		return;
	}
	QTimer::singleShot(CHIPTUNE_MIDI_OUT_AUDIO_PLAYER_THREAD_STARTUP_DELAY_IN_MILLISECONDS,
					   m_p_private->m_p_audio_player, &AudioPlayer::Play);
}

/**********************************************************************************/

void ChiptuneMidiOut::Stop(void)
{
	if(nullptr == m_p_private->m_p_audio_player){
		return;
	}
	m_p_private->m_p_audio_player->Stop();
}

/**********************************************************************************/

int ChiptuneMidiOut::SendMidiMessage(uint32_t const midi_message)
{
	if(nullptr == m_p_private->m_p_tune_manager){
		return -1;
	}
	return m_p_private->m_p_tune_manager->SendMidiMessage(midi_message);
}

/**********************************************************************************/

void ChiptuneMidiOut::SetPitchShift(int const pitch_shift_in_semitones)
{
	if(nullptr == m_p_private->m_p_tune_manager){
		return;
	}
	m_p_private->m_p_tune_manager->SetPitchShift(pitch_shift_in_semitones);
}

/**********************************************************************************/

int ChiptuneMidiOut::SetMelodicChannelTimbre(int8_t const channel_index,
											 int8_t const waveform,
											 int8_t const envelope_attack_curve,
											 float const envelope_attack_duration_in_seconds,
											 int8_t const envelope_decay_curve,
											 float const envelope_decay_duration_in_seconds,
											 uint8_t const envelope_note_on_sustain_level,
											 int8_t const envelope_release_curve,
											 float const envelope_release_duration_in_seconds,
											 uint8_t const envelope_damper_sustain_level,
											 int8_t const envelope_damper_sustain_curve,
											 float const envelope_damper_sustain_duration_in_seconds)
{
	if(nullptr == m_p_private->m_p_tune_manager){
		return -1;
	}

	return m_p_private->m_p_tune_manager->SetMelodicChannelTimbre(
		channel_index,
		waveform,
		envelope_attack_curve,
		envelope_attack_duration_in_seconds,
		envelope_decay_curve,
		envelope_decay_duration_in_seconds,
		envelope_note_on_sustain_level,
		envelope_release_curve,
		envelope_release_duration_in_seconds,
		envelope_damper_sustain_level,
		envelope_damper_sustain_curve,
		envelope_damper_sustain_duration_in_seconds);
}
