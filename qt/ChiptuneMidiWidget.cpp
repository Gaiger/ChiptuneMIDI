
#include "ui_ChiptuneMidiWidgetForm.h"
#include "ChiptuneMidiWidget.h"


ChiptuneMidiWidget::ChiptuneMidiWidget(TuneManager *const p_tune_manager, QWidget *parent)
	: QWidget(parent),
	m_p_tune_manager(p_tune_manager),
	ui(new Ui::ChiptuneMidiWidget)
{
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
