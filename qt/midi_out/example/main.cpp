#include <stdio.h>
#include <signal.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QtGlobal>
#include <QString>
#include <QStringList>
#include <QThread>

#include "ChiptuneMidiOut.h"
#include "mid_reader/qt/MidSong.h"

/**********************************************************************************/

static QThread *g_p_player_thread = nullptr;

static void request_termination(int)
{
	if(nullptr != g_p_player_thread){
		g_p_player_thread->requestInterruption();
	}
}

/**********************************************************************************/
class ChiptuneMidiConsolePlayer : public QThread
{
public:
	ChiptuneMidiConsolePlayer(QString const &file_name)
		: m_file_name(file_name)
	{
	}

protected:
	void run(void)
	{
		ChiptuneMidiOut midi_out;
		midi_out.Start();
		//midi_out.SetPitchShift(-1);
		for(int i = 0; i < 16; ++i){
			midi_out.SetMelodicChannelTimbre(
				i,
				ChiptuneMidiOut::WaveformSquareDutyCycle50,
				ChiptuneMidiOut::EnvelopeCurveLinear,
				0.03f,
				ChiptuneMidiOut::EnvelopeCurveLinear,
				0.02f,
				80,
				ChiptuneMidiOut::EnvelopeCurveLinear,
				0.03f,
				72,
				ChiptuneMidiOut::EnvelopeCurveLinear,
				8.0f);
		}

		MidSong mid_song; mid_song.Load(m_file_name);
		QElapsedTimer timer;
		timer.start();

		for(int i = 0; i < mid_song.GetEventCount(); ++i){
			if(isInterruptionRequested()){
				break;
			}

			MidEvent const event = mid_song.GetEvent(i);
			if(false == event.IsMessage()){
				continue;
			}

			qint64 const event_time_in_milliseconds =
				static_cast<qint64>((mid_song.TimeFromTick(event.GetTick()) * 1000.0f) + 0.5f);
			qint64 const wait_time_in_milliseconds = event_time_in_milliseconds - timer.elapsed();
			if(wait_time_in_milliseconds > 0){
				QThread::msleep(static_cast<unsigned long>(wait_time_in_milliseconds));
				if(isInterruptionRequested()){
					break;
				}
			}
			midi_out.SendMidiMessage(event.GetMessage());
			qInfo() << Q_FUNC_INFO << Qt::hex << event.GetMessage();
		}

		midi_out.Stop();
	}

private:
	QString const m_file_name;
};

/**********************************************************************************/
static void usage(char const *p_program_name)
{
	fprintf(stderr, "usage: %s <file.mid>\n", p_program_name);
}

/**********************************************************************************/
int main(int argc, char *argv[])
{
	QCoreApplication application(argc, argv);
	MidSong mid_song;
	if(1 == argc){
		usage(argv[0]);
		return -1;
	}
	QFileInfo file_info(argv[1]);
	if(false == file_info.isFile()){
		fprintf(stderr, "%s is not a file\n", file_info.absoluteFilePath().toUtf8().constData());
		return -1;
	}

	QString const midi_file_name = file_info.absoluteFilePath();
	if(false == mid_song.Load(midi_file_name)){
		fprintf(stderr, "failed to load MIDI file: %s\n", midi_file_name.toLocal8Bit().constData());
		return -2;
	}	

	ChiptuneMidiConsolePlayer player(midi_file_name);
	QObject::connect(&player, &QThread::finished, &application, &QCoreApplication::quit);
	g_p_player_thread = &player;
	signal(SIGINT, request_termination);
	signal(SIGTERM, request_termination);

	player.start();

	int const result = application.exec();
	player.requestInterruption();
	player.wait();
	g_p_player_thread = nullptr;
	return result;
}
