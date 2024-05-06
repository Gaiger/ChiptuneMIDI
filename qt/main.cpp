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
#include <QApplication>
#include <QDebug>

#include <QMidiOut.h>
#include <QMidiFile.h>
#include "MidiPlayer.h"

#include "TuneManager.h"
#include "AudioPlayer.h"
#include "ChiptuneMidiWidget.h"

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
#if(1)
#if defined( Q_OS_WIN )
	if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()){
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		freopen("CONIN$", "r", stdin);
	}
	setvbuf(stdout, NULL, _IONBF, 0);
#endif
#endif
	QApplication a(argc, argv);

#if(0)
	ListAvailableMidiDevices();
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
	TuneManager tune_manager(true, 16000, 16);

	ChiptuneMidiWidget chiptune_midi_widget(&tune_manager);
	chiptune_midi_widget.show();
	chiptune_midi_widget.setFocus();
	return a.exec();
}
