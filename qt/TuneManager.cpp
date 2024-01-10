#include <QThread>
#include <QFileInfo>

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

	void NotifyTuneHasEnded(void)
	{
		emit m_p_public->TuneEnded();
	}

public:
	int m_sampling_rate;
	QMidiFile *m_p_midi_file;
	int m_current_midi_event_index;
	int m_wave_prebuffer_length;

	QByteArray m_wave_bytearray;

	TuneManager *m_p_public;
};

/**********************************************************************************/

static TuneManagerPrivate *s_p_private = nullptr;

extern "C" int get_next_midi_message(uint32_t * const p_message, uint32_t * const p_tick)
{
	return s_p_private->GetNextMidiMessage(p_message, p_tick);
}

/**********************************************************************************/

extern "C" void tune_ending_notification(void)
{
	s_p_private->NotifyTuneHasEnded();
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
	chiptune_set_tune_ending_notfication_callback(tune_ending_notification);
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
	if(false == file_info.isFile())
	{
		return -1;
	}

	m_p_private->m_p_midi_file = new QMidiFile();
	if(false == m_p_private->m_p_midi_file->load(midi_file_name_string))
	{
		delete m_p_private->m_p_midi_file; m_p_private->m_p_midi_file = nullptr;
		return -2;
	}

	m_p_private->m_current_midi_event_index = 0;
	chiptune_initialize((uint32_t)m_p_private->m_sampling_rate);
	chiptune_set_resolution(m_p_private->m_p_midi_file->resolution());

	m_p_private->m_wave_prebuffer_length = 0;
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

