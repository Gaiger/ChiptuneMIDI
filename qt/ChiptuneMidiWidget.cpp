
#include "ui_ChiptuneMidiWidgetForm.h"
#include "ChiptuneMidiWidget.h"


ChiptuneMidiWidget::ChiptuneMidiWidget(TuneManager *const p_tune_manager, QWidget *parent)
	: QWidget(parent),
	m_p_tune_manager(p_tune_manager),
	ui(new Ui::ChiptuneMidiWidget)
{
	//QString filename = "8bit(bpm185)v0727T1.mid";
	//QString filename = "totoro.mid";
	//QString filename = "evil_eye.mid";
	//QString filename = "black_star.mid";
	//QString filename = "crying.mid";
	//QString filename = "triligy.mid";
	//QString filename = "6Oclock.mid";
	//QString filename = "requiem.mid";
	//QString filename = "Laputa.mid";
	//QString filename = "Ironforge.mid";
	//QString filename = "23401.mid";
	//QString filename = "never_enough.mid";

	//QString filename = "67573.mid";
	//QString filename = "79538.mid";

	//QString filename = "2012111316917146.mid";
	//QString filename = "25626oth34.mid";
	//QString filename = "duck.mid";
	//QString filename = "201211212129826.mid";
	//QString filename = "pray_for_buddha.mid";
	QString filename = "those_years.mid";

	//QString filename = "102473.mid";
	//QString filename = "korobeiniki.mid";
	//QString filename = "nekrasov-korobeiniki-tetris-theme-20230417182739.mid";

	//SaveAsWavFile(m_p_tune_manager, "20240206ankokuButo.wav");
	//SaveAsWavFile(m_p_tune_manager, "20240205Laputa.wav");
	//SaveAsWavFile(m_p_tune_manager, "20240205Ironforge.wav");
	//SaveAsWavFile(m_p_tune_manager, "202402156Oclock.wav");
	//SaveAsWavFile(m_p_tune_manager, "20240215never_enough.wav");
	//SaveAsWavFile(m_p_tune_manager, "202402016pray_for_buddha.wav");

	m_p_tune_manager->SetMidiFile(filename);
	m_p_tune_manager->moveToThread(&m_tune_manager_working_thread);
	m_tune_manager_working_thread.start(QThread::HighPriority);
	m_p_audio_player = new AudioPlayer(m_p_tune_manager, this);
	m_p_audio_player->Play();

	ui->setupUi(this);
}

/**********************************************************************************/

ChiptuneMidiWidget::~ChiptuneMidiWidget()
{
	m_tune_manager_working_thread.quit();
	while( false == m_tune_manager_working_thread.isFinished())
	{
		QThread::msleep(10);
	}

	m_p_audio_player->Stop();
	delete m_p_audio_player; m_p_audio_player = nullptr;
}
