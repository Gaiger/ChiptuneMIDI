/*
 *
 * Copyright 2003-2012 by David G. Slomin
 * Copyright 2012-2016 Augustin Cavalier <waddlesplash>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <QtGlobal>
#if defined( Q_OS_WIN )
#include <windows.h>
#endif

#include <stdio.h>
#include <QThread>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QDebug>

#include <QMidiOut.h>
#include <QMidiFile.h>
#include "MidiPlayer.h"

#include "TuneManager.h"
#include "AudioPlayer.h"

static void ListAvailableMidiDevices(void)
{
	QMap<QString, QString> vals = QMidiOut::devices();
	QList<QString> key_list = vals.keys();
	for(QString & key : key_list) {
		QString value = vals.value(key);
		fputs(key.toUtf8().constData(), stderr);
		fputs("\t", stderr);
		fputs(value.toUtf8().constData(), stderr);
		fputs("\n", stderr);
	}
}

/**********************************************************************************/

int main(int argc, char* argv[])
{
#if defined( Q_OS_WIN )
	if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()){
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		freopen("CONIN$", "r", stdin);
	}
	setvbuf(stdout, NULL, _IONBF, 0);
#endif

	QCoreApplication a(argc, argv);
	ListAvailableMidiDevices();

	QString filename = "8bit(bpm185)v0727T1.mid";
#if(0)
	QString midiOutName = "0";
	QMidiFile* midi_file = new QMidiFile();
	midi_file->load(filename);

	QMidiOut* midi_out = new QMidiOut();
	midi_out->connect(midiOutName);

	MidiPlayer* p_player = new MidiPlayer(midi_file, midi_out, &a);
	QObject::connect(p_player, &MidiPlayer::Finished, &a,  &QCoreApplication::quit);
	//p_player->moveToThread(a.thread());
	p_player->Play();
#endif
#if(1)
	TuneManager tune_manager(44100);
	QThread tune_manager_working_thread;
	tune_manager.moveToThread(&tune_manager_working_thread);
	tune_manager_working_thread.start(QThread::HighPriority);
	tune_manager.SetMidiFile(filename);
	AudioPlayer audio_player(&tune_manager, &a);
	audio_player.Play();
#endif
	return a.exec();
}

