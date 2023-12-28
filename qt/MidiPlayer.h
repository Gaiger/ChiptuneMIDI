#ifndef MIDIPLAYERTHREAD_H
#define MIDIPLAYERTHREAD_H

#include <stdio.h>
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
			if (e->type() != QMidiEvent::Meta) {
				qint64 event_time = midi_file->timeFromTick(e->tick()) * 1000;

				qint32 waitTime = event_time - t.elapsed();
				if (waitTime > 0) {
					QThread::msleep(waitTime);
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
		emit Finished();
	}

private:
	QMidiFile* midi_file;
	QMidiOut* midi_out;
};

#endif // MIDIPLAYERTHREAD_H
