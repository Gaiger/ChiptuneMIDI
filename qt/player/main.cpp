#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <QtGlobal>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <stdio.h>
#include <QThread>
#include <QElapsedTimer>
#include <QApplication>
#include <QDebug>

#include "TuneManager.h"
#include "ChiptuneMidiWidget.h"

/**********************************************************************************/

int main(int argc, char* argv[])
{
#if(1)
#ifdef Q_OS_WIN
	if (AttachConsole(ATTACH_PARENT_PROCESS) /*|| AllocConsole()*/){
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		freopen("CONIN$", "r", stdin);
	}
	setvbuf(stdout, NULL, _IONBF, 0);
#endif
#endif
#ifdef Q_OS_WIN
	_putenv("QT_SCALE_FACTOR=1");
	_putenv("QT_AUTO_SCREEN_SCALE_FACTOR=0");
#endif
	QApplication app(argc, argv);

	int sampling_rate = 44100;
#ifdef _DEBUG
	sampling_rate = 16000;
#endif
	TuneManager *p_tune_manager = new TuneManager(true, sampling_rate, 16);
	QObject::connect(&app, &QCoreApplication::aboutToQuit,
					   p_tune_manager, &QObject::deleteLater);

	QThread *p_tune_manager_working_thread = new QThread();
	QObject::connect(p_tune_manager, &QObject::destroyed,
					 p_tune_manager_working_thread, &QThread::quit);
	QObject::connect(p_tune_manager_working_thread, &QThread::finished,
					 p_tune_manager_working_thread, &QObject::deleteLater);
	p_tune_manager->QObject::moveToThread(p_tune_manager_working_thread);
	p_tune_manager_working_thread->QThread::start(QThread::HighPriority);

	ChiptuneMidiWidget chiptune_midi_widget(p_tune_manager);
	chiptune_midi_widget.show();
	chiptune_midi_widget.setFocus();
	return app.exec();
}
