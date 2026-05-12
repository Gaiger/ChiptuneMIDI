#include <QThread>
#include <QDebug>
#include <QGridLayout>

#include "ui_ChiptuneMidiSynthesizerWidgetForm.h"

#include "WaveChartView.h"

#include "ChiptuneMidiSynthesizerWidget.h"

/**********************************************************************************/

static void FillWidget(QWidget *p_widget, QWidget *p_filled_widget)
{
	QGridLayout *p_layout = nullptr;
	do
	{
		p_layout = (QGridLayout*)p_filled_widget->layout();
		if(nullptr != p_layout){
			break;
		}
		p_layout = new QGridLayout(p_filled_widget);
		p_layout->setContentsMargins(0, 0, 0, 0);
		p_layout->setSpacing(0);
	} while(0);
	p_layout->addWidget(p_widget, 0, 0);
}

/**********************************************************************************/

#define SYNTHESIZER_PLAYBACK_TICK_INQUIRY_INTERVAL_IN_MILLISECONDS		(10)

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	#define SYNTHESIZER_AUDIO_BUFFER_TIME_FACTOR    					(2)
#else
	#define SYNTHESIZER_AUDIO_BUFFER_TIME_FACTOR    					(6)
#endif

#define SYNTHESIZER_AUDIO_PLAYER_BUFFER_IN_MILLISECONDS						\
	(SYNTHESIZER_AUDIO_BUFFER_TIME_FACTOR * SYNTHESIZER_PLAYBACK_TICK_INQUIRY_INTERVAL_IN_MILLISECONDS)

#define SYNTHESIZER_NOTE_MIDI_CHANNEL				(0)
#define SYNTHESIZER_NOTE_NUMBER						(64)
#define SYNTHESIZER_NOTE_ON_VELOCITY				(127)
#define SYNTHESIZER_NOTE_OFF_VELOCITY				(64)
#define MIDI_MESSAGE_NOTE_ON						(0x90)
#define MIDI_MESSAGE_NOTE_OFF						(0x80)

static uint32_t MakeShortMidiMessage(uint8_t const status_byte, uint8_t const data_byte_1, uint8_t const data_byte_2)
{
	return (uint32_t)status_byte | ((uint32_t)data_byte_1 << 8) | ((uint32_t)data_byte_2 << 16);
}

ChiptuneMidiSynthesizerWidget::ChiptuneMidiSynthesizerWidget(TuneManager * p_tune_manager, QWidget *parent)
	: QWidget(parent)
	, m_p_tune_manager(p_tune_manager)
	, m_p_audio_player(nullptr)
	, m_p_wave_chartview(nullptr)
	, m_audio_player_buffer_in_milliseconds(SYNTHESIZER_AUDIO_PLAYER_BUFFER_IN_MILLISECONDS)
	, ui(new Ui::ChiptuneMidiSynthesizerWidget)
{
	ui->setupUi(this);

	do {
		m_p_wave_chartview = new WaveChartView(
					p_tune_manager->GetNumberOfChannels(),
					p_tune_manager->GetSamplingRate(), p_tune_manager->GetSamplingSize(), this);
		FillWidget(m_p_wave_chartview, ui->WaveWidget);
		QObject::connect(p_tune_manager, &TuneManager::WaveDelivered,
						 m_p_wave_chartview, &WaveChartView::UpdateWave, Qt::QueuedConnection);
	} while(0);

	m_p_audio_player = new AudioPlayer(m_p_tune_manager->GetNumberOfChannels(),
									   m_p_tune_manager->GetSamplingRate(),
									   m_p_tune_manager->GetSamplingSize(),
									   m_audio_player_buffer_in_milliseconds, nullptr);
	QObject::connect(p_tune_manager, &TuneManager::WaveDelivered,
					 m_p_audio_player, &AudioPlayer::FeedData, Qt::DirectConnection);
	QObject::connect(m_p_audio_player, &AudioPlayer::DataRequested,
					 p_tune_manager, &TuneManager::RequestWave, Qt::DirectConnection);

	QThread *p_audio_player_working_thread = new QThread();
	QObject::connect(m_p_audio_player, &QObject::destroyed,
					 p_audio_player_working_thread, &QThread::quit);
	QObject::connect(p_audio_player_working_thread, &QThread::finished,
					 p_audio_player_working_thread, &QThread::deleteLater);
	m_p_audio_player->QObject::moveToThread(p_audio_player_working_thread);
	p_audio_player_working_thread->QThread::start(QThread::NormalPriority);

	QWidget::setFocusPolicy(Qt::StrongFocus);
	QWidget::setFixedSize(QWidget::size());

	m_p_audio_player->Play();
}

/**********************************************************************************/

ChiptuneMidiSynthesizerWidget::~ChiptuneMidiSynthesizerWidget()
{
	do
	{
		if(nullptr == m_p_audio_player){
			break;
		}
		m_p_audio_player->Stop();
		if (m_p_audio_player->QObject::thread() == QThread::currentThread()){
			delete m_p_audio_player;
			break;
		}
		m_p_audio_player->QObject::deleteLater();
	}while(0);
	m_p_audio_player = nullptr;
	delete ui;
}

/**********************************************************************************/

void ChiptuneMidiSynthesizerWidget::on_NotePushButton_pressed(void)
{
	uint32_t const midi_message = MakeShortMidiMessage(MIDI_MESSAGE_NOTE_ON | SYNTHESIZER_NOTE_MIDI_CHANNEL,
													   SYNTHESIZER_NOTE_NUMBER,
													   SYNTHESIZER_NOTE_ON_VELOCITY);
	qDebug() << "midi message =" << Qt::hex << midi_message;
	m_p_tune_manager->SendMidiMessage(midi_message);
}

/**********************************************************************************/

void ChiptuneMidiSynthesizerWidget::on_NotePushButton_released(void)
{
	uint32_t const midi_message = MakeShortMidiMessage(MIDI_MESSAGE_NOTE_OFF | SYNTHESIZER_NOTE_MIDI_CHANNEL,
													   SYNTHESIZER_NOTE_NUMBER,
													   SYNTHESIZER_NOTE_OFF_VELOCITY);
	qDebug() << "midi message =" << Qt::hex << midi_message;
	m_p_tune_manager->SendMidiMessage(midi_message);
}
