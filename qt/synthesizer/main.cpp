#include <QApplication>

#include "ChiptuneMidiSynthesizerWidget.h"

/**********************************************************************************/

int main(int argc, char* argv[])
{
#ifdef Q_OS_WIN
	_putenv("QT_SCALE_FACTOR=1");
	_putenv("QT_AUTO_SCREEN_SCALE_FACTOR=0");
#endif
	QApplication app(argc, argv);

	ChiptuneMidiSynthesizerWidget chiptune_midi_synthesizer_widget;
	chiptune_midi_synthesizer_widget.show();
	chiptune_midi_synthesizer_widget.setFocus();
	return app.exec();
}
