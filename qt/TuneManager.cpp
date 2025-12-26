#include <QThread>
#include <QFileInfo>
#include <QTimer>

#include <QDebug>

#include "QMidiFile.h"

#include "chiptune_midi_define.h"
#include "chiptune.h"

#include "TuneManager.h"

class TuneManagerPrivate
{
public:
	int GetMidiMessage(int const index, uint32_t * const p_tick, uint32_t * const p_message)
	{
		int ret = -1;
		do
		{
			if(m_p_midi_file->events().size() <= index){
				break;
			}

			QMidiEvent *p_midi_event = m_p_midi_file->events().at(index);
			if(QMidiEvent::Meta == p_midi_event->type()){
				if(QMidiEvent::Tempo == p_midi_event->number()){
					chiptune_set_tempo(p_midi_event->tempo());
				}
			}

			*p_tick = (uint32_t)p_midi_event->tick();
			*p_message = p_midi_event->message();
			ret = 0;
		} while(0);

		return ret;
	}

	void GenerateWave(int const size)
	{
		QByteArray generated_bytearray;
		generated_bytearray.reserve(size);
		int fetch_times = size;
		if(TuneManager::SamplingSize16Bit == m_sampling_size){
			fetch_times /= 2;
		}

		for(int i = 0; i < fetch_times; i++) {
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

	void ResetSongResources(void)
	{
		m_wave_bytearray.clear();
		m_wave_prebuffer_size = 0;
		m_channel_instrument_pair_list.clear();
	}

public:
	QMidiFile *m_p_midi_file;
	int m_number_of_channels;
	int m_sampling_rate;
	int m_sampling_size;

	int m_wave_prebuffer_size;
	QByteArray m_wave_bytearray;

	Qt::ConnectionType m_connection_type;
	QList<QPair<int, int>> m_channel_instrument_pair_list;

	QMutex m_mutex;
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
	m_p_private = new TuneManagerPrivate();
	QMutexLocker locker(&m_p_private->m_mutex);

	do
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		if (false != QMetaType::fromName("TuneManager::SamplingSize").isValid()) {
			break;
		}
#else
		if( QMetaType::UnknownType != QMetaType::type("TuneManager::SamplingSize")){
			break;
		}
#endif
		qRegisterMetaType<TuneManager::SamplingSize>("TuneManager::SamplingSize");
	} while(0);

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
	m_p_private->m_channel_instrument_pair_list.clear();
	m_p_private->m_connection_type = Qt::AutoConnection;

	s_p_private_instance = m_p_private;
	chiptune_initialize( 2 == m_p_private->m_number_of_channels ? true : false,
						 (uint32_t)m_p_private->m_sampling_rate, get_midi_message);
}

/**********************************************************************************/

TuneManager::~TuneManager(void)
{
	{
		QMutexLocker locker(&m_p_private->m_mutex);

		chiptune_finalize();
		if(nullptr != m_p_private->m_p_midi_file){
			delete m_p_private->m_p_midi_file;
			m_p_private->m_p_midi_file = nullptr;
		}
		m_p_private->m_channel_instrument_pair_list.clear();
	}
	delete m_p_private;
	m_p_private = nullptr;
}

/**********************************************************************************/

int TuneManager::LoadMidiFile(QString const midi_file_name_string)
{
	QMutexLocker locker(&m_p_private->m_mutex);
	int ret = 0;
	do
	{
		QFileInfo file_info(midi_file_name_string);
		if(false == file_info.isFile()){
			qDebug() << Q_FUNC_INFO << midi_file_name_string << "is not a file";
			ret = -1;
			break;
		}

		QMidiFile *p_midi_file = new QMidiFile();
		if(false == p_midi_file->load(midi_file_name_string))
		{
			delete p_midi_file; p_midi_file = nullptr;
			ret = -2;
			break;
		}

		if(nullptr != m_p_private->m_p_midi_file){
			delete m_p_private->m_p_midi_file;
		}
		m_p_private->m_p_midi_file = p_midi_file;

		qDebug()  << "Music time length = " << GetMidiFileDurationInSeconds() << "seconds";
		m_p_private->ResetSongResources();
		chiptune_prepare_song(m_p_private->m_p_midi_file->resolution());
	} while(0);
	return ret;
}

/**********************************************************************************/

void TuneManager::ClearOutMidiFile(void)
{
	QMutexLocker locker(&m_p_private->m_mutex);
	m_p_private->m_wave_bytearray.clear();
	m_p_private->m_wave_prebuffer_size = 0;

	if(nullptr != m_p_private->m_p_midi_file){
		delete m_p_private->m_p_midi_file;
		m_p_private->m_p_midi_file = nullptr;
	}
	m_p_private->m_channel_instrument_pair_list.clear();
}

/**********************************************************************************/

QMidiFile * TuneManager::GetMidiFilePointer(void)
{
	if(false == IsFileLoaded()){
		return nullptr;
	}

	return m_p_private->m_p_midi_file;
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

void TuneManager::HandleGenerateWaveRequested(int const size)
{
	QMutexLocker locker(&m_p_private->m_mutex);
	m_p_private->GenerateWave(size);
}

/**********************************************************************************/

void TuneManager::SubmitWaveGeneration(int const size, bool const is_synchronized)
{
	bool is_to_reconnect = false;
	do{
		if(QObject::thread() == QThread::currentThread()){
			if(Qt::DirectConnection != m_p_private->m_connection_type){
				m_p_private->m_connection_type = Qt::DirectConnection;
				is_to_reconnect = true;
			}
			break;
		}

		//qDebug() << Q_FUNC_INFO << "is_synchronized = " << is_synchronized;
		if(false == is_synchronized){
			if(Qt::QueuedConnection != m_p_private->m_connection_type){
				m_p_private->m_connection_type = Qt::QueuedConnection;
				is_to_reconnect = true;
			}
			break;
		}

		if(Qt::BlockingQueuedConnection != m_p_private->m_connection_type){
			m_p_private->m_connection_type = Qt::BlockingQueuedConnection;
			is_to_reconnect = true;
		}
	}while(0);

	if(true == is_to_reconnect){
		QObject::disconnect(this, &TuneManager::GenerateWaveRequested,
							this, &TuneManager::HandleGenerateWaveRequested);
		QObject::connect(this, &TuneManager::GenerateWaveRequested,
						 this, &TuneManager::HandleGenerateWaveRequested,
						 m_p_private->m_connection_type);
	}

	emit GenerateWaveRequested(size);
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

QByteArray TuneManager::FetchWave(int const size)
{
	if(size > m_p_private->m_wave_prebuffer_size){
		m_p_private->m_wave_prebuffer_size = size;
	}

	do
	{
		int submit_size = 0;
		{
			QMutexLocker lock(&m_p_private->m_mutex);
			if(m_p_private->m_wave_bytearray.size() > size){
				break;
			}
			submit_size = size - m_p_private->m_wave_bytearray.size();
		}
		SubmitWaveGeneration(submit_size, true);
	}while(0);

	QByteArray fetched_wave_bytearray;
	do
	{
		{
			QMutexLocker lock(&m_p_private->m_mutex);
			fetched_wave_bytearray = m_p_private->m_wave_bytearray.mid(0, size);
			m_p_private->m_wave_bytearray.remove(0, size);
			if(m_p_private->m_wave_bytearray.size() > m_p_private->m_wave_prebuffer_size){
				break;
			}
		}
		SubmitWaveGeneration(m_p_private->m_wave_prebuffer_size, false);
	} while(0);

	return fetched_wave_bytearray;
}

/**********************************************************************************/

void TuneManager::RequestWave(int const size)
{
	QByteArray requested_wave_bytearray = FetchWave(size);
	emit WaveDelivered(requested_wave_bytearray);
}

/**********************************************************************************/

bool TuneManager::IsTuneEnding(void)
{
	bool is_ending = false;
	do {
		if(false == chiptune_is_tune_ending()){
			break;
		}
		{
			QMutexLocker lock(&m_p_private->m_mutex);
			if(0 < m_p_private->m_wave_bytearray.size()){
				break;
			}
		}
		is_ending = true;
	} while(0);

	return is_ending;
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

int TuneManager::GetCurrentTick(void)
{
	if(nullptr == m_p_private->m_p_midi_file){
		return -1;
	}

	return chiptune_get_current_tick();
}

/**********************************************************************************/

double TuneManager::GetTempo(void)
{
	if(nullptr == m_p_private->m_p_midi_file){
		return FLT_MAX;
	}
	return chiptune_get_tempo();
}

/**********************************************************************************/

double TuneManager::GetPlayingEffectiveTempo(void)
{
	if(nullptr == m_p_private->m_p_midi_file){
		return FLT_MAX;
	}
	return chiptune_get_playing_effective_tempo();
}

/**********************************************************************************/

void TuneManager::SetPlayingSpeedRatio(double playing_speed_raio)
{
	QMutexLocker locker(&m_p_private->m_mutex);
	qDebug() << Q_FUNC_INFO << playing_speed_raio;
	chiptune_set_playing_speed_ratio((float)playing_speed_raio);
}

/**********************************************************************************/

void TuneManager::SetPitchShift(int pitch_shift_in_semitones)
{
	QMutexLocker locker(&m_p_private->m_mutex);
	chiptune_set_pitch_shift_in_semitones((int8_t)pitch_shift_in_semitones);
}

/**********************************************************************************/

int TuneManager::SetStartTimeInSeconds(float target_start_time_in_seconds)
{
	QMutexLocker locker(&m_p_private->m_mutex);

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

	int ret = -1;
	do
	{
		if(-1 == set_index){
			qDebug() << Q_FUNC_INFO <<"ERROR :: could not find target_start_time_in_seconds = " << target_start_time_in_seconds;
			break;
		}
		qDebug() << Q_FUNC_INFO << "target_start_time_in_seconds = " << target_start_time_in_seconds
				 << ", found time = " << m_p_private->m_p_midi_file->timeFromTick(p_midi_event_list.at(set_index)->tick())
				 << ", index = " << set_index
				 << ", tick = " << p_midi_event_list.at(set_index)->tick();
		chiptune_set_current_message_index(set_index);

		m_p_private->m_wave_bytearray.clear();
		m_p_private->m_wave_prebuffer_size = 0;
		ret = 0;
	} while(0);

	return ret;
}

/**********************************************************************************/

QList<QPair<int, int>> TuneManager::GetChannelInstrumentPairList(void)
{
	QMutexLocker locker(&m_p_private->m_mutex);
	do {
		if(0 != m_p_private->m_channel_instrument_pair_list.size()){
			break;
		}

		int8_t instrument_array[CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER];
		chiptune_get_ending_instruments(&instrument_array[0]);
		m_p_private->m_channel_instrument_pair_list.clear();
		for(int voice = 0; voice < CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER; voice++){
			if(CHIPTUNE_INSTRUMENT_UNUSED_CHANNEL != instrument_array[voice]){
				m_p_private->m_channel_instrument_pair_list.append(QPair<int, int>(voice, instrument_array[voice]));
			}
		}
	} while(0);
	return m_p_private->m_channel_instrument_pair_list;
}

/**********************************************************************************/

void TuneManager::SetChannelOutputEnabled(int index, bool is_enabled)
{
	QMutexLocker locker(&m_p_private->m_mutex);
	do
	{
		if(index < 0 || index >= CHIPTUNE_MIDI_MAX_CHANNEL_NUMBER){
			break;
		}

		chiptune_set_channel_output_enabled((int8_t)index, is_enabled);
	} while(0);
}

/**********************************************************************************/

int TuneManager::SetPitchChannelTimbre(int8_t const channel_index,
						   int8_t const waveform,
						   int8_t const envelope_attack_curve, float const envelope_attack_duration_in_seconds,
						   int8_t const envelope_decay_curve, float const envelope_decay_duration_in_seconds,
						   uint8_t const envelope_note_on_sustain_level,
						   int8_t const envelope_release_curve, float const envelope_release_duration_in_seconds,
						   uint8_t const envelope_damper_on_but_note_off_sustain_level,
						   int8_t const envelope_damper_on_but_note_off_sustain_curve,
						   float const envelope_damper_on_but_note_off_sustain_duration_in_seconds)
{
	QMutexLocker locker(&m_p_private->m_mutex);

	int ret = -1;
	do
	{
		if(nullptr == m_p_private->m_p_midi_file){
			break;
		}
		if(MIDI_PERCUSSION_CHANNEL == channel_index){
			qDebug() << "WARNING :: MIDI_PERCUSSION_CHANNEL timbre is unsettable, ignored";
			break;
		}
		ret = chiptune_set_pitch_channel_timbre(channel_index, waveform,
												envelope_attack_curve, envelope_attack_duration_in_seconds,
												envelope_decay_curve, envelope_decay_duration_in_seconds,
												envelope_note_on_sustain_level,
												envelope_release_curve, envelope_release_duration_in_seconds,
												envelope_damper_on_but_note_off_sustain_level,
												envelope_damper_on_but_note_off_sustain_curve,
												envelope_damper_on_but_note_off_sustain_duration_in_seconds);
	} while(0);
	return ret;
}

