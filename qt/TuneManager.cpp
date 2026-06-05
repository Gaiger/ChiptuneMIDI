#include <QThread>
#include <QTimer>

#include <cfloat>

#include <QDebug>

#include "mid_reader/qt/MidSong.h"

#include "chiptune_midi_define.h"
#include "chiptune.h"

#include "TuneManager.h"

class TuneManagerPrivate
{
public:
	void ChiptuneLock(bool const is_to_lock){
		do
		{
			if(false == is_to_lock){
				m_chiptune_mutex.unlock();
				break;
			}
			m_chiptune_mutex.lock();
		}while(0);
	}

	int GetMidiMessage(int const index, uint32_t * const p_tick, uint32_t * const p_message)
	{
		int ret = -1;
		MidEvent midi_event;
		do
		{
			if((nullptr == m_p_midi_message_provider) || (index < 0)){
				break;
			}
			if(m_p_midi_message_provider->GetMidSongPointer()->GetEventCount() <= index){
				break;
			}

			midi_event = m_p_midi_message_provider->GetMidSongPointer()->GetEvent(index);
			if(false == midi_event.IsValid()){
				break;
			}

			if(MidEvent::Tempo == midi_event.GetType()){
				if(0.0f < midi_event.GetTempo()){
					chiptune_set_tempo(midi_event.GetTempo());
				}
			}

			*p_tick = midi_event.GetTick();
			*p_message = midi_event.GetMessage();
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
	bool IsPushMode(void) const
	{
		return (nullptr == m_p_midi_message_provider);
	}

public:
	MidiMessageProvider *m_p_midi_message_provider;
	int m_number_of_channels;
	int m_sampling_rate;
	int m_sampling_size;

	int m_wave_prebuffer_size;
	QByteArray m_wave_bytearray;

	Qt::ConnectionType m_connection_type;
	QList<QPair<int, int>> m_channel_instrument_pair_list;

	QMutex m_tune_manager_mutex;
	QMutex m_chiptune_mutex;
};

/**********************************************************************************/
static TuneManagerPrivate *s_p_private_instance = nullptr;

extern "C" void chiptune_lock(bool is_to_lock)
{
	s_p_private_instance->ChiptuneLock(is_to_lock);
}

extern "C" int get_midi_message(uint32_t const message_index, uint32_t * const p_tick, uint32_t * const p_message)
{
	return s_p_private_instance->GetMidiMessage((int)message_index, p_tick, p_message);
}

/**********************************************************************************/
TuneManager::TuneManager(bool const is_stereo,
						 int const sampling_rate, int const sampling_size,
						 QObject *parent)
	: QObject(parent),
	  m_p_private(new TuneManagerPrivate())
{
	QMutexLocker locker(&m_p_private->m_tune_manager_mutex);

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
	m_p_private->m_p_midi_message_provider = nullptr;
	m_p_private->m_channel_instrument_pair_list.clear();
	m_p_private->m_connection_type = Qt::AutoConnection;

	s_p_private_instance = m_p_private;

	chiptune_set_lock_callback(chiptune_lock);
	chiptune_set_pull_message_callback(nullptr);
	chiptune_initialize( 2 == m_p_private->m_number_of_channels ? true : false,
						 (uint32_t)m_p_private->m_sampling_rate);
	chiptune_prepare_session(MIDI_DEFAULT_RESOLUTION);
}

/**********************************************************************************/

TuneManager::~TuneManager(void)
{
	{
		QMutexLocker locker(&m_p_private->m_tune_manager_mutex);

		chiptune_finalize();
		m_p_private->m_channel_instrument_pair_list.clear();
	}
	delete m_p_private;
}

/**********************************************************************************/

void TuneManager::SetMidiMessageProvider(MidiMessageProvider *p_midi_message_provider)
{
	QMutexLocker locker(&m_p_private->m_tune_manager_mutex);
	m_p_private->m_p_midi_message_provider = p_midi_message_provider;
	m_p_private->m_channel_instrument_pair_list.clear();
	do
	{
		if(nullptr == p_midi_message_provider){
			chiptune_set_pull_message_callback(nullptr);
			chiptune_prepare_session(MIDI_DEFAULT_RESOLUTION);
			break;
		}

		chiptune_set_pull_message_callback(get_midi_message);
		chiptune_prepare_session((uint32_t)p_midi_message_provider->GetMidSongPointer()->GetResolution());

		int8_t instrument_code_array[MIDI_MAX_CHANNEL_NUMBER];
		chiptune_get_ending_instruments(&instrument_code_array[0]);
		for(int voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
			if(CHIPTUNE_INSTRUMENT_UNUSED_CHANNEL != instrument_code_array[voice]){
				m_p_private->m_channel_instrument_pair_list.append(QPair<int, int>(voice, instrument_code_array[voice]));
			}
		}
	} while(0);

	m_p_private->m_wave_bytearray.clear();
	m_p_private->m_wave_prebuffer_size = 0;
}

/**********************************************************************************/
int TuneManager::SendMidiMessage(uint32_t const midi_message)
{
	if(false == m_p_private->IsPushMode()){
		return INT_MIN;
	}
	return chiptune_push_midi_message(midi_message);
}

/**********************************************************************************/

void TuneManager::HandleGenerateWaveRequested(int const size)
{
	QMutexLocker locker(&m_p_private->m_tune_manager_mutex);
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
QByteArray TuneManager::FetchWave(int const size)
{
	if(size > m_p_private->m_wave_prebuffer_size){
		m_p_private->m_wave_prebuffer_size = size;
	}

	do
	{
		int submit_size = 0;
		{
			QMutexLocker lock(&m_p_private->m_tune_manager_mutex);
			if(m_p_private->m_wave_bytearray.size() >= size){
				break;
			}
			//qDebug() << Q_FUNC_INFO << "WARNING :: m_p_private->m_wave_bytearray.size() < size";
			submit_size = size - m_p_private->m_wave_bytearray.size();
		}
		SubmitWaveGeneration(submit_size, true);
	}while(0);

	QByteArray fetched_wave_bytearray;
	do
	{
		{
			QMutexLocker lock(&m_p_private->m_tune_manager_mutex);
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
QByteArray TuneManager::TakeWave(int const size)
{
	if(true == m_p_private->IsPushMode()){
		 qWarning() << Q_FUNC_INFO << "TakeWave() is unavailable in push mode";
		return QByteArray();
	}

	return FetchWave(size);
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
			QMutexLocker lock(&m_p_private->m_tune_manager_mutex);
			if(0 < m_p_private->m_wave_bytearray.size()){
				break;
			}
		}
		is_ending = true;
	} while(0);

	return is_ending;
}

/**********************************************************************************/

float TuneManager::GetCurrentElapsedTimeInSeconds(void)
{
	if(true == m_p_private->IsPushMode()){
		return -1.0;
	}
	MidSong * const p_mid_song = m_p_private->m_p_midi_message_provider->GetMidSongPointer();

	return p_mid_song->TimeFromTick((uint32_t)chiptune_get_current_tick());
}

/**********************************************************************************/

int TuneManager::GetCurrentTick(void)
{
	if(true == m_p_private->IsPushMode()){
		return -1;
	}

	return chiptune_get_current_tick();
}

/**********************************************************************************/

double TuneManager::GetCurrentTempo(void)
{
	if(true == m_p_private->IsPushMode()){
		return FLT_MAX;
	}
	return chiptune_get_tempo();
}

/**********************************************************************************/

double TuneManager::GetPlayingEffectiveTempo(void)
{
	if(true == m_p_private->IsPushMode()){
		return FLT_MAX;
	}
	return chiptune_get_playing_effective_tempo();
}

/**********************************************************************************/

void TuneManager::SetPlayingSpeedRatio(double const playing_speed_raio)
{
	QMutexLocker locker(&m_p_private->m_tune_manager_mutex);
	qDebug() << Q_FUNC_INFO << playing_speed_raio;
	chiptune_set_playing_speed_ratio((float)playing_speed_raio);
}

/**********************************************************************************/

void TuneManager::SetPitchShift(int const pitch_shift_in_semitones)
{
	QMutexLocker locker(&m_p_private->m_tune_manager_mutex);
	chiptune_set_pitch_shift_in_semitones((int8_t)pitch_shift_in_semitones);
}

/**********************************************************************************/

int TuneManager::SetStartTimeInSeconds(float const target_start_time_in_seconds)
{
	QMutexLocker locker(&m_p_private->m_tune_manager_mutex);
	if(true == m_p_private->IsPushMode()){
		return -1;
	}
	MidSong * const p_mid_song = m_p_private->m_p_midi_message_provider->GetMidSongPointer();

	int set_index = -1;
	int const event_count = p_mid_song->GetEventCount();

	do {
		if(target_start_time_in_seconds < 0){
			break;
		}

		if(0 == event_count){
			break;
		}

		int left_index = 0;
		int right_index = event_count - 1;
		while(left_index <= right_index){
			int middle_index = (left_index + right_index) / 2;
			do
			{
				float const middle_index_event_time_in_seconds
						= p_mid_song->TimeFromTick(p_mid_song->GetEvent(middle_index).GetTick());
				if(middle_index_event_time_in_seconds > target_start_time_in_seconds){
					right_index = middle_index - 1;
					break;
				}
				left_index = middle_index + 1;
			} while(0);
		}
		set_index = right_index;
		if(-1 == set_index){
			set_index = 0;
		}
	} while(0);

	int ret = -1;
	do
	{
		if((set_index < 0) || ((set_index + 1) >= event_count)){
			break;
		}
		{
			float found_timestamp_tail_index_time_in_seconds = p_mid_song->TimeFromTick(p_mid_song->GetEvent(set_index).GetTick());
			float found_timestamp_tail_index_plus_one_time_in_seconds
					= p_mid_song->TimeFromTick(p_mid_song->GetEvent(set_index + 1).GetTick());
			if(false == (found_timestamp_tail_index_time_in_seconds <= target_start_time_in_seconds &&
					target_start_time_in_seconds < found_timestamp_tail_index_plus_one_time_in_seconds)){
				qDebug() << Q_FUNC_INFO << "ERROR :: found_timestamp_tail_index_time_in_seconds = " << found_timestamp_tail_index_time_in_seconds
						 << "target_start_time_in_seconds = " << target_start_time_in_seconds
						 << "found_timestamp_tail_index_plus_one_time_in_seconds = " << found_timestamp_tail_index_plus_one_time_in_seconds;
				qDebug() << "It voilates found_timestamp_tail_index_time_in_seconds <= target_start_time_in_seconds"
						 << " and target_start_time_in_seconds < found_timestamp_tail_index_plus_one_time_in_seconds";
				break;
			}
		}

		int const found_timestamp_tail_index = set_index;
		do
		{
			if(0 == set_index){
				break;
			}
			int left_index = 0;
			int right_index = set_index;
			uint32_t const found_timestamp_in_ticks = p_mid_song->GetEvent(set_index).GetTick();
			while(left_index <= right_index){
				int middle_index = (left_index + right_index) / 2;
				do
				{
					uint32_t const middle_index_event_time_in_tick = p_mid_song->GetEvent(middle_index).GetTick();
					if(middle_index_event_time_in_tick >= found_timestamp_in_ticks){
						right_index = middle_index - 1;
						break;
					}
					left_index = middle_index + 1;
				} while(0);
			}
			set_index = left_index;

			if(0 == set_index){
				break;
			}
			if(p_mid_song->GetEvent(set_index - 1).GetTick() == p_mid_song->GetEvent(set_index).GetTick()){
				qDebug() << Q_FUNC_INFO << "WARNING :: set_index is not the head for the same tick";
			}
		} while(0);

		qDebug() << Q_FUNC_INFO << "target_start_time_in_seconds = " << target_start_time_in_seconds
				 << ", found time in seconds = " << p_mid_song->TimeFromTick(p_mid_song->GetEvent(set_index).GetTick())
				 << ", found_timestamp_tail_index = " << found_timestamp_tail_index
				 << ", set_index = " << set_index
				 << ", tick = " << p_mid_song->GetEvent(set_index).GetTick();
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
	QMutexLocker locker(&m_p_private->m_tune_manager_mutex);
	return m_p_private->m_channel_instrument_pair_list;
}

/**********************************************************************************/
int TuneManager::GetCurrentChannelInstrument(int const channel_index)
{
	QMutexLocker locker(&m_p_private->m_tune_manager_mutex);
	return chiptune_get_channel_instrument(channel_index);
}

/**********************************************************************************/
void TuneManager::SetChannelOutputEnabled(int const channel_index, bool const is_enabled)
{
	QMutexLocker locker(&m_p_private->m_tune_manager_mutex);
	chiptune_set_channel_output_enabled((int8_t)channel_index, is_enabled);
}

/**********************************************************************************/
int TuneManager::SetMelodicChannelTimbre(int8_t const channel_index,
						   int8_t const waveform,
						   int8_t const envelope_attack_curve, float const envelope_attack_duration_in_seconds,
						   int8_t const envelope_decay_curve, float const envelope_decay_duration_in_seconds,
						   uint8_t const envelope_note_on_sustain_level,
						   int8_t const envelope_release_curve, float const envelope_release_duration_in_seconds,
						   uint8_t const envelope_note_off_hold_sustain_level,
						   int8_t const envelope_note_off_hold_sustain_curve,
						   float const envelope_note_off_hold_sustain_duration_in_seconds)
{
	QMutexLocker locker(&m_p_private->m_tune_manager_mutex);
	return chiptune_set_melodic_channel_timbre(channel_index, waveform,
											  envelope_attack_curve, envelope_attack_duration_in_seconds,
											  envelope_decay_curve, envelope_decay_duration_in_seconds,
											  envelope_note_on_sustain_level,
											  envelope_release_curve, envelope_release_duration_in_seconds,
											  envelope_note_off_hold_sustain_level,
											  envelope_note_off_hold_sustain_curve,
											  envelope_note_off_hold_sustain_duration_in_seconds);
}

/**********************************************************************************/
QString TuneManager::GetChiptuneEngineVersionString(void)
{
	uint8_t major_version = 0;
	uint8_t minor_version = 0;
	uint8_t micro_version = 0;
	chiptune_get_version(&major_version, &minor_version, &micro_version);

	return QStringLiteral("%1.%2.%3").arg(major_version).arg(minor_version).arg(micro_version);
}

