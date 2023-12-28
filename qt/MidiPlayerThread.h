#ifndef MIDIPLAYERTHREAD_H
#define MIDIPLAYERTHREAD_H

#include <stdio.h>
#include <QThread>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QDebug>

#include <QMidiOut.h>
#include <QMidiFile.h>

class MidiPlayer : public QThread
{
	Q_OBJECT
public:
	MidiPlayer(QMidiFile* file, QMidiOut* out)
	{
		midi_file = file;
		midi_out = out;
	}

private:
	QMidiFile* midi_file;
	QMidiOut* midi_out;

protected:
	void run()
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
			if (e->type() != QMidiEvent::Meta) {
				qint64 event_time = midi_file->timeFromTick(e->tick()) * 1000;

				qint32 waitTime = event_time - t.elapsed();
				if (waitTime > 0) {
					msleep(waitTime);
				}
				if (e->type() == QMidiEvent::SysEx) {
					// TODO: sysex
				} else {
					qint32 message = e->message();
					midi_out->sendMsg(message);
				}
			}
		}

		midi_out->disconnect();
	}
};

#endif // MIDIPLAYERTHREAD_H
