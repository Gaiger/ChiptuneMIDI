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
	int GetNextMidiMessage(uint32_t * const p_message, uint32_t * const p_tick)
	{
		QMidiEvent *p_midi_event;
		while(1)
		{
			do
			{
				if(m_p_midi_file->events().size() <= m_current_midi_event_index){
					return -1;
				}

				p_midi_event = m_p_midi_file->events().at(m_current_midi_event_index);
				m_current_midi_event_index += 1;

				if(QMidiEvent::Meta == p_midi_event->type()){
					if(QMidiEvent::Tempo == p_midi_event->number()){
						chiptune_set_tempo(p_midi_event->tempo());
					}
				}
			}while(p_midi_event->type() == QMidiEvent::Meta || p_midi_event->type() == QMidiEvent::SysEx);

			//if(0 == p_midi_event->track() || 1 == p_midi_event->track())
			{
				break;
			}
		}

		//qDebug() << "track = " << p_midi_event->track();
		*p_message = p_midi_event->message();
		*p_tick = (uint32_t)p_midi_event->tick();

		return 0;
	}

	void GenerateWave(int const length)
	{
		QByteArray generated_bytearray;
		generated_bytearray.reserve(length);
		for(int i = 0; i < length; i++) {
			uint8_t value = chiptune_fetch_wave();
			generated_bytearray += value;
		}

		m_wave_bytearray += generated_bytearray;
	}

public:
	int m_sampling_rate;
	QMidiFile *m_p_midi_file;
	int m_current_midi_event_index;
	int m_wave_prebuffer_length;

	QByteArray m_wave_bytearray;

	QTimer m_inquiring_tune_ending_timer;
	TuneManager *m_p_public;
};

/**********************************************************************************/

static TuneManagerPrivate *s_p_private = nullptr;

extern "C" int get_next_midi_message(uint32_t * const p_message, uint32_t * const p_tick)
{
	return s_p_private->GetNextMidiMessage(p_message, p_tick);
}

/**********************************************************************************/

TuneManager::TuneManager(int sampliing_rate, QObject *parent)
	: QObject(parent)
{
	QMutexLocker locker(&m_mutex);
	m_p_private = new TuneManagerPrivate();
	m_p_private->m_sampling_rate = sampliing_rate;
	m_p_private->m_p_midi_file = nullptr;
	m_p_private->m_p_public = this;

	s_p_private = m_p_private;
	chiptune_set_midi_message_callback(get_next_midi_message);
}

/**********************************************************************************/

TuneManager::~TuneManager(void)
{
	QMutexLocker locker(&m_mutex);
	if(nullptr != m_p_private->m_p_midi_file){
		delete m_p_private->m_p_midi_file;
		m_p_private->m_p_midi_file = nullptr;
	}

	delete m_p_private;
	m_p_private = nullptr;
}

/**********************************************************************************/

int TuneManager::SetMidiFile(QString midi_file_name_string)
{
	QMutexLocker locker(&m_mutex);
	QFileInfo file_info(midi_file_name_string);
	if(false == file_info.isFile()){
		return -1;
	}

	m_p_private->m_p_midi_file = new QMidiFile();
	if(false == m_p_private->m_p_midi_file->load(midi_file_name_string))
	{
		delete m_p_private->m_p_midi_file; m_p_private->m_p_midi_file = nullptr;
		return -2;
	}

	InitializeTune();
	return 0;
}
/**********************************************************************************/

int TuneManager::InitializeTune(void)
{
	if(nullptr == m_p_private->m_p_midi_file){
		return -1;
	}

	m_p_private->m_current_midi_event_index = 0;
	chiptune_initialize((uint32_t)m_p_private->m_sampling_rate);
	chiptune_set_resolution(m_p_private->m_p_midi_file->resolution());

	m_p_private->m_inquiring_tune_ending_timer.disconnect();
	m_p_private->m_wave_prebuffer_length = 0;
	QObject::connect(&m_p_private->m_inquiring_tune_ending_timer, &QTimer::timeout, this, [&](){
		do
		{
			if(false == chiptune_is_tune_ending()){
				break;
			}
			m_p_private->m_inquiring_tune_ending_timer.stop();
			emit TuneEnded();
		}while(0);
	}, Qt::DirectConnection);

	m_p_private->m_inquiring_tune_ending_timer.start(100);
	return 0;
}

/**********************************************************************************/

void TuneManager::HandleGenerateWaveRequested(int length)
{
	m_p_private->GenerateWave(length);
}

/**********************************************************************************/

void TuneManager::GenerateWave(int length, bool is_synchronized)
{
	QObject::disconnect(this, &TuneManager::GenerateWaveRequested,
						this, &TuneManager::HandleGenerateWaveRequested);

	Qt::ConnectionType type = Qt::DirectConnection;
	do
	{
		if( QObject::thread() == QThread::currentThread()){
			break;
		}

		//qDebug() << Q_FUNC_INFO << "is_synchronized = " << is_synchronized;
		if(false == is_synchronized){
			type = Qt::QueuedConnection;
			break;
		}
		type = Qt::BlockingQueuedConnection;
	}while(0);

	QObject::connect(this, &TuneManager::GenerateWaveRequested,
					 this, &TuneManager::HandleGenerateWaveRequested, type);
	emit GenerateWaveRequested(length);
}

/**********************************************************************************/

int TuneManager::GetSamplingRate(void){ return m_p_private->m_sampling_rate;}

/**********************************************************************************/

QByteArray TuneManager::FetchWave(int const length)
{
	QMutexLocker locker(&m_mutex);
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

