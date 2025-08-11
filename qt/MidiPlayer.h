#ifndef _MIDIPLAYERTHREAD_H_
#define _MIDIPLAYERTHREAD_H_

#include <QThread>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QDebug>

#include <QMidiOut.h>
#include <QMidiFile.h>

class MidiPlayer : public QObject
{
	Q_OBJECT
public:
	MidiPlayer(QMidiFile* file, QMidiOut* out, QObject *parent)
	: QObject(parent)
	{
		midi_file = file;
		midi_out = out;

		QObject::connect(this, &MidiPlayer::PlayRequested, this,  &MidiPlayer::HandlePlayRequested, Qt::QueuedConnection);
	}

public :
	void Play(void)
	{
		emit PlayRequested();
	}
public :
	signals:
	void Finished(void);

private:
	signals:
	void PlayRequested(void);

private slots:
	void HandlePlayRequested(void)
	{
		PlayMidi();
	}

private:
	void PlayMidi()
	{
		QElapsedTimer t;
		t.start();
		QList<QMidiEvent*> events = midi_file->events();
		//qDebug() << midi_file->tracks();
		for (QMidiEvent* e : qAsConst(events)) {
#if(0)
			if(2 != e->track()){
				continue;
			}
			qDebug() << "note = " <<e->note() << ", tick = " << e->tick() << ", type = " <<e->type();
#endif
			if (e->type() == QMidiEvent::Meta)
			{
				if(QMidiEvent::Tempo == e->number()){
					qDebug() << "temp = " <<  e->tempo();
					qDebug() << "";
				}
			}

			if (e->type() != QMidiEvent::Meta)
			{
				qint64 event_time = midi_file->timeFromTick(e->tick()) * 1000;

				qint32 waitTime = event_time - t.elapsed();
				//qDebug() <<"track = "<< e->track() << "type = " << e->type();
#if(1)
				if(8 != e->track()){
					continue;
				}
#endif
				switch(e->type()){
				case QMidiEvent::ControlChange:
					qDebug() << "QMidiEvent::ControlChange";
					qDebug().nospace() << "tick = "<< e->tick() <<
								", track = "<< e->track() << ", voice = "<< e->voice() <<", number = "<< e->number() <<", value = " << e->value();
					break;

				case QMidiEvent::ProgramChange:
					qDebug() << "QMidiEvent::ProgramChange";
					qDebug().nospace() << "tick = "<< e->tick() <<
										  ", tick = "<< e->tick() << ", track = "<< e->track() << ", voice = "<< e->voice()  << ", number = "<< e->number();
					break;
#if(1)
				case QMidiEvent::PitchWheel:
					qDebug() << "QMidiEvent::PitchWheel";
					qDebug().nospace() << "tick = "<< e->tick() <<
										  ", track = "<< e->track() <<", voice = "<< e->voice()  <<", value = " << e->value();
					//e->setValue(0x2000*6/12);
					//continue;
					break;
#endif
#if(1)
				case QMidiEvent::NoteOn:
					qDebug() << "QMidiEvent::NoteOn";
					qDebug().nospace() << "tick = "<< e->tick() <<
										", track = "<< e->track() <<", voice = "<< e->voice()  <<", note = " << e->note();

					break;

				case QMidiEvent::NoteOff:
					qDebug() << "QMidiEvent::NoteOff";
					qDebug().nospace() << "tick = "<< e->tick() <<
										  ", track = "<< e->track() <<", voice = "<< e->voice()  <<", note = " << e->note();
					break;
#endif
				default:
					break;
				}

				if (waitTime > 0) {
					QThread::msleep(waitTime);
				}
				if (e->type() == QMidiEvent::SysEx) {
					// TODO: sysex
				} else {

					//if(1 != e->track() && (0 == e->type() || 1 == e->type()) ){
					//	continue;
					//}
					//qDebug() << "voice = "<< e->voice();
					//qDebug() << "numerator" << e->numerator();
					//qDebug() << "denominator" << e->denominator();
					//qDebug() << "tick = " << e->tick();
					//qDebug() << "tempo = " << e->tempo();
					//qDebug() << "resolution" << midi_file->resolution();
					qint32 message = e->message();
					midi_out->sendMsg(message);
				}
			}
		}

		midi_out->disconnect();
		emit Finished();
	}

private:
	QMidiFile* midi_file;
	QMidiOut* midi_out;
};

#endif // _MIDIPLAYERTHREAD_H_
