#include <QThread>
#include <QFileInfo>
#include <QTimer>

#include <QDebug>

#include "QMidiFile.h"
#include "../chiptune.h"

#include "TuneManager.h"



class TuneManagerPrivate
{
public:
	int GetMidiMessage(int const index, uint32_t * const p_tick, uint32_t * const p_message)
	{

		if(m_p_midi_file->events().size() <= index){
			return -1;
		}

		QMidiEvent *p_midi_event = m_p_midi_file->events().at(index);
		if(QMidiEvent::Meta == p_midi_event->type()){
			if(QMidiEvent::Tempo == p_midi_event->number()){
				chiptune_set_tempo(p_midi_event->tempo());
			}
		}

		*p_tick = (uint32_t)p_midi_event->tick();
		*p_message = p_midi_event->message();
		return 0;
	}

	void GenerateWave(int const length)
	{
		QByteArray generated_bytearray;
		generated_bytearray.reserve(length);
		for(int i = 0; i < length; i++) {
			if(true == chiptune_is_tune_ending()){
				break;
			}

			do {
				if(TuneManager::SamplingSize16Bit == m_sampling_size){
					int16_t value = chiptune_fetch_16bit_wave();
					generated_bytearray += QByteArray((char*)&value, 2);
					break;
				}
				generated_bytearray += chiptune_fetch_8bit_wave();
			} while(0);
		}

		m_wave_bytearray += generated_bytearray;
	}

public:
	QMidiFile *m_p_midi_file;
	int m_number_of_channels;
	int m_sampling_rate;
	int m_sampling_size;

	int m_wave_prebuffer_length;
	QByteArray m_wave_bytearray;

	QList<int> m_noted_channel_list;

	TuneManager *m_p_public;
};

/**********************************************************************************/

static TuneManagerPrivate *s_p_private_instance = nullptr;

extern "C" int get_midi_message(uint32_t index, uint32_t * const p_tick, uint32_t * const p_message)
{
	return s_p_private_instance->GetMidiMessage((int)index, p_tick, p_message);
}

/**********************************************************************************/

TuneManager::TuneManager(bool is_stereo,
						 int const sampling_rate, int const sampling_size, QObject *parent)
	: QObject(parent)
{
	QMutexLocker locker(&m_mutex);
	if( QMetaType::UnknownType == QMetaType::type("SamplingSize")){
			qRegisterMetaType<TuneManager::SamplingSize>("SamplingSize");
	}

	m_p_private = new TuneManagerPrivate();
	m_p_private->m_sampling_rate = sampling_rate;
	do {
		if(SamplingSize16Bit == sampling_size){
			m_p_private->m_sampling_size = SamplingSize16Bit;
			break;
		}
		m_p_private->m_sampling_size = SamplingSize8Bit;
	} while(0);

	do {
		if(true == is_stereo){
			m_p_private->m_number_of_channels = 2;
			break;
		}
		m_p_private->m_number_of_channels = 1;
	} while(0);
	m_p_private->m_p_midi_file = nullptr;
	m_p_private->m_noted_channel_list.clear();
	m_p_private->m_p_public = this;

	s_p_private_instance = m_p_private;
	chiptune_set_midi_message_callback(get_midi_message);
}

/**********************************************************************************/

TuneManager::~TuneManager(void)
{
	QMutexLocker locker(&m_mutex);
	if(nullptr != m_p_private->m_p_midi_file){
		delete m_p_private->m_p_midi_file;
		m_p_private->m_p_midi_file = nullptr;
	}
	m_p_private->m_noted_channel_list.clear();

	delete m_p_private;
	m_p_private = nullptr;
}

/**********************************************************************************/

int TuneManager::LoadMidiFile(QString const midi_file_name_string)
{
	QMutexLocker locker(&m_mutex);
	QFileInfo file_info(midi_file_name_string);
	if(false == file_info.isFile()){
		qDebug() << Q_FUNC_INFO << midi_file_name_string << "is not a file";
		return -1;
	}

	m_p_private->m_p_midi_file = new QMidiFile();
	if(false == m_p_private->m_p_midi_file->load(midi_file_name_string))
	{
		delete m_p_private->m_p_midi_file; m_p_private->m_p_midi_file = nullptr;
		return -2;
	}

	qDebug()  << "Music time length = " << GetMidiFileDurationInSeconds() << "seconds";
	InitializeTune();
	return 0;
}

/**********************************************************************************/

void TuneManager::ClearOutMidiFile(void)
{
	QMutexLocker locker(&m_mutex);
	m_p_private->m_wave_bytearray.clear();
	m_p_private->m_wave_prebuffer_length = 0;

	if(nullptr != m_p_private->m_p_midi_file){
		delete m_p_private->m_p_midi_file;
		m_p_private->m_p_midi_file = nullptr;
	}
	m_p_private->m_noted_channel_list.clear();
}

/**********************************************************************************/

int TuneManager::InitializeTune(void)
{
	m_p_private->m_wave_bytearray.clear();
	m_p_private->m_wave_prebuffer_length = 0;

	if(nullptr == m_p_private->m_p_midi_file){
		return -1;
	}

	m_p_private->m_noted_channel_list.clear();
	bool is_channels_noted_array[CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER];
	chiptune_initialize( 2 == m_p_private->m_number_of_channels ? true : false,
						 (uint32_t)m_p_private->m_sampling_rate, m_p_private->m_p_midi_file->resolution(),
						 &is_channels_noted_array[0]);
	for(int i = 0; i < CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER; i++){
		if(true == is_channels_noted_array[i]){
			m_p_private->m_noted_channel_list.append(i);
		}
	}
	return 0;
}

/**********************************************************************************/

bool TuneManager::IsFileLoaded(void)
{
	if(nullptr == m_p_private->m_p_midi_file){
		return false;
	}
	return true;
}

/**********************************************************************************/

void TuneManager::HandleGenerateWaveRequested(int const length)
{
	QMutexLocker locker(&m_mutex);
	m_p_private->GenerateWave(length);
}

/**********************************************************************************/

void TuneManager::GenerateWave(int const length, bool const is_synchronized)
{
	QObject::disconnect(this, &TuneManager::GenerateWaveRequested,
						this, &TuneManager::HandleGenerateWaveRequested);

	Qt::ConnectionType type = Qt::DirectConnection;
	do {
		if( QObject::thread() == QThread::currentThread()){
			break;
		}

		//qDebug() << Q_FUNC_INFO << "is_synchronized = " << is_synchronized;
		if(false == is_synchronized){
			type = Qt::QueuedConnection;
			break;
		}
		type = Qt::BlockingQueuedConnection;
	} while(0);

	QObject::connect(this, &TuneManager::GenerateWaveRequested,
					 this, &TuneManager::HandleGenerateWaveRequested, type);
	emit GenerateWaveRequested(length);
}

/**********************************************************************************/

int TuneManager::GetNumberOfChannels(void){return m_p_private->m_number_of_channels; }

/**********************************************************************************/

int TuneManager::GetSamplingRate(void){ return m_p_private->m_sampling_rate; }

/**********************************************************************************/

int TuneManager::GetSamplingSize(void){ return m_p_private->m_sampling_size; }

/**********************************************************************************/

int TuneManager::GetAmplitudeGain(void)
{
	if(nullptr == m_p_private->m_p_midi_file){
		return -1;
	}
	return chiptune_get_amplitude_gain();
}

/**********************************************************************************/

void TuneManager::SetAmplitudeGain(int amplitude_gain)
{
	chiptune_set_amplitude_gain(amplitude_gain);
}

/**********************************************************************************/

QByteArray TuneManager::FetchWave(int const length)
{
	if(length > m_p_private->m_wave_prebuffer_length){
		m_p_private->m_wave_prebuffer_length = length;
	}

	if(m_p_private->m_wave_bytearray.mid(0, length).size() < length){
			GenerateWave(length - m_p_private->m_wave_bytearray.mid(0, length).size(),
							 true);
	}

	QByteArray fetched_wave_bytearray = m_p_private->m_wave_bytearray.mid(0, length);
	m_p_private->m_wave_bytearray.remove(0, length);

	if(m_p_private->m_wave_bytearray.mid(length, -1).size() < m_p_private->m_wave_prebuffer_length){
		GenerateWave(m_p_private->m_wave_prebuffer_length, false);
	}

	emit WaveFetched(fetched_wave_bytearray);
	return fetched_wave_bytearray;
}

/**********************************************************************************/

bool TuneManager::IsTuneEnding(void)
{
	do {
		if(false == chiptune_is_tune_ending()){
			break;
		}
		if(0 < m_p_private->m_wave_bytearray.size()){
			break;
		}

		return true;
	} while(0);

	return false;
}

/**********************************************************************************/

float TuneManager::GetMidiFileDurationInSeconds(void)
{
	if(nullptr == m_p_private->m_p_midi_file){
		return -1.0;
	}

	QMidiEvent *p_midi_event = m_p_private->m_p_midi_file->events().at(
				m_p_private->m_p_midi_file->events().size() -1);
	return m_p_private->m_p_midi_file->timeFromTick(p_midi_event->tick());
}

/**********************************************************************************/

float TuneManager::GetCurrentElapsedTimeInSeconds(void)
{
	if(nullptr == m_p_private->m_p_midi_file){
		return -1.0;
	}

	return m_p_private->m_p_midi_file->timeFromTick(chiptune_get_current_tick());
}

/**********************************************************************************/

int TuneManager::SetStartTimeInSeconds(float target_start_time_in_seconds)
{
	QMutexLocker locker(&m_mutex);

	int set_index = -1;
	QList<QMidiEvent *> const p_midi_event_list = m_p_private ->m_p_midi_file->events();

	do {
		if(nullptr == m_p_private->m_p_midi_file){
			break;
		}

		if(0.05 > qAbs(GetMidiFileDurationInSeconds() - target_start_time_in_seconds)){
			set_index = m_p_private->m_p_midi_file->events().size() - 1;
			break;
		}

		if(target_start_time_in_seconds < 0){
			target_start_time_in_seconds = 0;
		}


		float current_event_time_in_seconds = m_p_private->m_p_midi_file->timeFromTick(p_midi_event_list.at(0)->tick());
		float next_event_time_in_seconds = m_p_private->m_p_midi_file->timeFromTick(p_midi_event_list.at(0 + 1)->tick());

		for(int i = 0; i < p_midi_event_list.size() - 1; i++){
			if(0.05 > qAbs(current_event_time_in_seconds - target_start_time_in_seconds)){
				set_index = i;
				break;
			}

			if(current_event_time_in_seconds < target_start_time_in_seconds){
				if(next_event_time_in_seconds > target_start_time_in_seconds){
					set_index = i + 1;
					break;
				}
			}

			current_event_time_in_seconds = next_event_time_in_seconds;
			next_event_time_in_seconds = m_p_private->m_p_midi_file->timeFromTick(p_midi_event_list.at(i + 1)->tick());
		}
	} while(0);

	if(-1 == set_index){
		qDebug() << Q_FUNC_INFO <<"ERROR :: could not find target_start_time_in_seconds = " << target_start_time_in_seconds;
		return -1;
	}
	qDebug() << Q_FUNC_INFO << "target_start_time_in_seconds = " << target_start_time_in_seconds
			 << ", found time = " << m_p_private->m_p_midi_file->timeFromTick(p_midi_event_list.at(set_index)->tick())
			 << ", index = " << set_index
			 << ", tick = " << p_midi_event_list.at(set_index)->tick();
	chiptune_move_toward(set_index);

	m_p_private->m_wave_bytearray.clear();
	m_p_private->m_wave_prebuffer_length = 0;
	return 0;
}

/**********************************************************************************/

QList<int> TuneManager::GetNotedChannelList(void)
{
	return m_p_private->m_noted_channel_list;
}

/**********************************************************************************/

void TuneManager::SetChannelOutputEnabled(int index, bool is_enabled)
{
	QMutexLocker locker(&m_mutex);
	if(index < 0 || index >= CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER){
		return ;
	}

	chiptune_set_channel_output_enabled((int8_t)index, is_enabled);
}

/**********************************************************************************/

int TuneManager::SetPitchChannelTimbre(int8_t const channel_index,
						   int8_t const waveform,
						   int8_t const envelope_attack_curve, float const envelope_attack_duration_in_seconds,
						   int8_t const envelope_decay_curve, float const envelope_decay_duration_in_seconds,
						   uint8_t const envelope_sustain_level,
						   int8_t const envelope_release_curve, float const envelope_release_duration_in_seconds,
						   uint8_t const envelope_damper_on_but_note_off_sustain_level,
						   int8_t const envelope_damper_on_but_note_off_sustain_curve,
						   float const envelope_damper_on_but_note_off_sustain_duration_in_seconds)
{
	QMutexLocker locker(&m_mutex);
	if(nullptr == m_p_private->m_p_midi_file){
		return -1;
	}

	int ret = chiptune_set_pitch_channel_timbre(channel_index, waveform,
									  envelope_attack_curve, envelope_attack_duration_in_seconds,
									  envelope_decay_curve, envelope_decay_duration_in_seconds,
									  envelope_sustain_level,
									  envelope_release_curve, envelope_release_duration_in_seconds,
									  envelope_damper_on_but_note_off_sustain_level,
									  envelope_damper_on_but_note_off_sustain_curve,
									  envelope_damper_on_but_note_off_sustain_duration_in_seconds);
	return ret;
}
