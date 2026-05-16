#include <QThread>
#include <QDebug>
#include <QGridLayout>
#include <QKeyEvent>
#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QPair>
#include <QStringList>
#include <QTimer>
#include <QTimerEvent>

#include "ui_ChiptuneMidiSynthesizerWidgetForm.h"

#include "chiptune_midi_define.h"

#include "ChannelListWidget.h"
#include "ChiptuneMidiValues.h"
#include "InstrumentTimbreIniFile.h"
#include "WaveChartView.h"
#include "MidiInputManager.h"

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
#define SYNTHESIZER_PLAYBACK_TICK_INQUIRY_INTERVAL_IN_MILLISECONDS		(30)

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	#define SYNTHESIZER_AUDIO_BUFFER_TIME_FACTOR    					(2)
#else
	#define SYNTHESIZER_AUDIO_BUFFER_TIME_FACTOR    					(6)
#endif

#define SYNTHESIZER_AUDIO_PLAYER_BUFFER_IN_MILLISECONDS						\
	(SYNTHESIZER_AUDIO_BUFFER_TIME_FACTOR * SYNTHESIZER_PLAYBACK_TICK_INQUIRY_INTERVAL_IN_MILLISECONDS)

#define SYNTHESIZER_INQUIRY_INSTRUMENT_INTERVAL_IN_MILLISECONDS			(100)

#define INSTRUMENT_TIMBRES_INI_FILE_NAME_STRING	QStringLiteral("instrument_timbres.ini")

/**********************************************************************************/
static void SetupChannelListWidget(ChannelListWidget * const p_channel_list_widget, TuneManager * const p_tune_manager)
{
	instrument_timbre_t const default_instrument_timbre = GetDefaultInstrumentTimbre();
	for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; ++channel_index){
		p_channel_list_widget->AddChannel(channel_index, 0, true);

		if(MIDI_PERCUSSION_CHANNEL == channel_index){
			continue;
		}
		p_channel_list_widget->SetMelodicChannelTimbre(channel_index,
													   default_instrument_timbre.waveform,
													   default_instrument_timbre.envelope_attack_curve,
													   default_instrument_timbre.envelope_attack_duration_in_seconds,
													   default_instrument_timbre.envelope_decay_curve,
													   default_instrument_timbre.envelope_decay_duration_in_seconds,
													   default_instrument_timbre.envelope_note_on_sustain_level,
													   default_instrument_timbre.envelope_release_curve,
													   default_instrument_timbre.envelope_release_duration_in_seconds,
													   default_instrument_timbre.envelope_damper_sustain_level,
													   default_instrument_timbre.envelope_damper_sustain_curve,
													   default_instrument_timbre.envelope_damper_sustain_duration_in_seconds,
													   false);
		p_tune_manager->SetMelodicChannelTimbre((int8_t)channel_index,
												default_instrument_timbre.waveform,
												default_instrument_timbre.envelope_attack_curve,
												default_instrument_timbre.envelope_attack_duration_in_seconds,
												default_instrument_timbre.envelope_decay_curve,
												default_instrument_timbre.envelope_decay_duration_in_seconds,
												default_instrument_timbre.envelope_note_on_sustain_level,
												default_instrument_timbre.envelope_release_curve,
												default_instrument_timbre.envelope_release_duration_in_seconds,
												default_instrument_timbre.envelope_damper_sustain_level,
												default_instrument_timbre.envelope_damper_sustain_curve,
												default_instrument_timbre.envelope_damper_sustain_duration_in_seconds);
	}
}

/**********************************************************************************/
static inline bool IsInstrumentTimbreIdentical(instrument_timbre_t const * const p_original_timbre,
											   instrument_timbre_t const * const p_compared_timbre)
{
	return p_original_timbre->waveform == p_compared_timbre->waveform
			&& p_original_timbre->envelope_attack_curve == p_compared_timbre->envelope_attack_curve
			&& qRound(1000.0f * p_original_timbre->envelope_attack_duration_in_seconds)
				== qRound(1000.0f * p_compared_timbre->envelope_attack_duration_in_seconds)
			&& p_original_timbre->envelope_decay_curve == p_compared_timbre->envelope_decay_curve
			&& qRound(1000.0f * p_original_timbre->envelope_decay_duration_in_seconds)
				== qRound(1000.0f * p_compared_timbre->envelope_decay_duration_in_seconds)
			&& p_original_timbre->envelope_note_on_sustain_level
				== p_compared_timbre->envelope_note_on_sustain_level
			&& p_original_timbre->envelope_release_curve == p_compared_timbre->envelope_release_curve
			&& qRound(1000.0f * p_original_timbre->envelope_release_duration_in_seconds)
				== qRound(1000.0f * p_compared_timbre->envelope_release_duration_in_seconds)
			&& p_original_timbre->envelope_damper_sustain_level
				== p_compared_timbre->envelope_damper_sustain_level
			&& p_original_timbre->envelope_damper_sustain_curve
				== p_compared_timbre->envelope_damper_sustain_curve
			&& qRound(1000.0f * p_original_timbre->envelope_damper_sustain_duration_in_seconds)
				== qRound(1000.0f * p_compared_timbre->envelope_damper_sustain_duration_in_seconds);
}

/**********************************************************************************/
static instrument_timbre_t GetChannelInstrumentTimbre(ChannelListWidget * const p_channel_list_widget,
													  int const channel_index)
{
	int waveform;
	int envelope_attack_curve; double envelope_attack_duration_in_seconds;
	int envelope_decay_curve; double envelope_decay_duration_in_seconds;
	int envelope_note_on_sustain_level;
	int envelope_release_curve; double envelope_release_duration_in_seconds;
	int envelope_damper_sustain_level;
	int envelope_damper_sustain_curve;
	double envelope_damper_sustain_duration_in_seconds;
	p_channel_list_widget->GetMelodicChannelTimbre(channel_index,
							&waveform, &envelope_attack_curve, &envelope_attack_duration_in_seconds,
							&envelope_decay_curve, &envelope_decay_duration_in_seconds,
							&envelope_note_on_sustain_level,
							&envelope_release_curve, &envelope_release_duration_in_seconds,
							&envelope_damper_sustain_level,
							&envelope_damper_sustain_curve,
							&envelope_damper_sustain_duration_in_seconds);

	return GetInstrumentTimbre(waveform,
							   envelope_attack_curve,
							   (float)envelope_attack_duration_in_seconds,
							   envelope_decay_curve,
							   (float)envelope_decay_duration_in_seconds,
							   envelope_note_on_sustain_level,
							   envelope_release_curve,
							   (float)envelope_release_duration_in_seconds,
							   envelope_damper_sustain_level,
							   envelope_damper_sustain_curve,
							   (float)envelope_damper_sustain_duration_in_seconds);
}

/**********************************************************************************/
ChiptuneMidiSynthesizerWidget::ChiptuneMidiSynthesizerWidget(TuneManager * p_tune_manager, QWidget *parent)
	: QWidget(parent),
	  m_p_tune_manager(p_tune_manager),
	  m_p_audio_player(nullptr),
	  m_p_wave_chartview(nullptr),
	  m_p_midi_input_manager(nullptr),
	  m_audio_player_buffer_in_milliseconds(SYNTHESIZER_AUDIO_PLAYER_BUFFER_IN_MILLISECONDS),
	  m_inquiry_instrument_timer_id(-1),
	  ui(new Ui::ChiptuneMidiSynthesizerWidget)
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
	QObject::connect(m_p_audio_player, &AudioPlayer::StateChanged,
					 this, &ChiptuneMidiSynthesizerWidget::HandleAudioPlayerStateChanged, Qt::QueuedConnection);

	QThread *p_audio_player_working_thread = new QThread();
	QObject::connect(m_p_audio_player, &QObject::destroyed,
					 p_audio_player_working_thread, &QThread::quit);
	QObject::connect(p_audio_player_working_thread, &QThread::finished,
					 p_audio_player_working_thread, &QThread::deleteLater);
	m_p_audio_player->QObject::moveToThread(p_audio_player_working_thread);
	p_audio_player_working_thread->QThread::start(QThread::NormalPriority);

	QWidget::setFocusPolicy(Qt::StrongFocus);
	QWidget::setFixedSize(QWidget::size());

	ChannelListWidget * const p_channel_list_widget = new ChannelListWidget(ui->TimbreListWidget);
	QObject::connect(p_channel_list_widget, &ChannelListWidget::OutputEnabled,
					 this, &ChiptuneMidiSynthesizerWidget::HandleChannelOutputEnabled);
	QObject::connect(p_channel_list_widget, &ChannelListWidget::MelodicChannelTimbreChanged,
					 this, &ChiptuneMidiSynthesizerWidget::HandleMelodicChannelTimbreChanged);
	FillWidget(p_channel_list_widget, ui->TimbreListWidget);
	SetupChannelListWidget(p_channel_list_widget, m_p_tune_manager);
	m_inquiry_instrument_timer_id =
			QObject::startTimer(SYNTHESIZER_INQUIRY_INSTRUMENT_INTERVAL_IN_MILLISECONDS);

#define SYNTHESIZER_AUDIO_PLAYER_THREAD_STARTUP_DELAY_IN_MILLISECONDS	(50)
	QTimer::singleShot(SYNTHESIZER_AUDIO_PLAYER_THREAD_STARTUP_DELAY_IN_MILLISECONDS,
					   m_p_audio_player, &AudioPlayer::Play);

	m_p_midi_input_manager = new MidiInputManager(this);
	QObject::connect(m_p_midi_input_manager, &MidiInputManager::MidiMessageReceived,
					 m_p_tune_manager, &TuneManager::SendMidiMessage, Qt::QueuedConnection);

	QStringList const midi_input_port_name_list = m_p_midi_input_manager->GetPortNameList();
	qInfo() << "MIDI input ports:" << midi_input_port_name_list;
	if(false == midi_input_port_name_list.isEmpty()){
		m_p_midi_input_manager->OpenPort(0);
	}
}

/**********************************************************************************/
ChiptuneMidiSynthesizerWidget::~ChiptuneMidiSynthesizerWidget()
{
	if(-1 != m_inquiry_instrument_timer_id){
		QObject::killTimer(m_inquiry_instrument_timer_id);
		m_inquiry_instrument_timer_id = -1;
	}

	if(nullptr != m_p_midi_input_manager){
		m_p_midi_input_manager->ClosePort();
		//delete m_p_midi_input_manager;
		m_p_midi_input_manager = nullptr;
	}
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
void ChiptuneMidiSynthesizerWidget::timerEvent(QTimerEvent *event)
{
	QWidget::timerEvent(event);

	if(event->timerId() == m_inquiry_instrument_timer_id){
		do {
			ChannelListWidget * const p_channel_list_widget = ui->TimbreListWidget->findChild<ChannelListWidget*>();
			if(nullptr == p_channel_list_widget){
				break;
			}
			for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; ++channel_index){
				int const instrument_code = m_p_tune_manager->GetCurrentChannelInstrument(channel_index);
				p_channel_list_widget->SetMelodicChannelInstrument(channel_index, instrument_code);
			}
		}while(0);
	}
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::HandleAudioPlayerStateChanged(AudioPlayer::PlaybackState state)
{
	do
	{
		if(state != AudioPlayer::PlaybackStateIdle){
			break;
		}
		m_p_audio_player->Play();
	}while(0);
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::HandleChannelOutputEnabled(int channel_index, bool is_enabled)
{
	m_p_tune_manager->SetChannelOutputEnabled(channel_index, is_enabled);
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::HandleMelodicChannelTimbreChanged(int channel_index,
																	  int waveform,
																	  int envelope_attack_curve,
																	  double envelope_attack_duration_in_seconds,
																	  int envelope_decay_curve,
																	  double envelope_decay_duration_in_seconds,
																	  int envelope_note_on_sustain_level,
																	  int envelope_release_curve,
																	  double envelope_release_duration_in_seconds,
																	  int envelope_damper_sustain_level,
																	  int envelope_damper_sustain_curve,
																	  double envelope_damper_sustain_duration_in_seconds)
{
	m_p_tune_manager->SetMelodicChannelTimbre((int8_t)channel_index, (int8_t)waveform,
											  (int8_t)envelope_attack_curve,
											  (float)envelope_attack_duration_in_seconds,
											  (int8_t)envelope_decay_curve,
											  (float)envelope_decay_duration_in_seconds,
											  (uint8_t)envelope_note_on_sustain_level,
											  (int8_t)envelope_release_curve,
											  (float)envelope_release_duration_in_seconds,
											  (uint8_t)envelope_damper_sustain_level,
											  (int8_t)envelope_damper_sustain_curve,
											  (float)envelope_damper_sustain_duration_in_seconds);
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::on_LoadTimbresPushButton_released(void)
{
	InstrumentTimbreIniFile timbre_ini_file(INSTRUMENT_TIMBRES_INI_FILE_NAME_STRING);
	QMap<int8_t, instrument_timbre_t> ini_instrument_timbre_map;
	int const ret = timbre_ini_file.ReadTimbres(&ini_instrument_timbre_map);

	do {
		if(-1 == ret){
			QMessageBox::warning(this, QStringLiteral("Load Timbres"),
								 tr("%1 not found.").arg(INSTRUMENT_TIMBRES_INI_FILE_NAME_STRING));
			break;
		}

		ChannelListWidget * const p_channel_list_widget = ui->TimbreListWidget->findChild<ChannelListWidget*>();
		do{
			if(nullptr == p_channel_list_widget){
				break;
			}
			QList<QPair<int, int>> const channel_instrument_pair_list =
					m_p_tune_manager->GetChannelInstrumentPairList();
			for(int i = 0; i < channel_instrument_pair_list.size(); i++){
				int const channel_index = channel_instrument_pair_list.at(i).first;
				int const channel_instrument_code = channel_instrument_pair_list.at(i).second;
				do
				{
					if(MIDI_PERCUSSION_CHANNEL == channel_index){
						break;
					}
					if(false == ini_instrument_timbre_map.contains((int8_t)channel_instrument_code)){
						break;
					}

					instrument_timbre_t const loaded_instrument_timbre
							= ini_instrument_timbre_map.value((int8_t)channel_instrument_code);
					instrument_timbre_t const channel_instrument_timbre
							= GetChannelInstrumentTimbre(p_channel_list_widget, channel_index);
					if(true == IsInstrumentTimbreIdentical(&loaded_instrument_timbre, &channel_instrument_timbre)){
						break;
					}
					p_channel_list_widget->SetMelodicChannelTimbre(
								channel_index,
								loaded_instrument_timbre.waveform,
								loaded_instrument_timbre.envelope_attack_curve,
								loaded_instrument_timbre.envelope_attack_duration_in_seconds,
								loaded_instrument_timbre.envelope_decay_curve,
								loaded_instrument_timbre.envelope_decay_duration_in_seconds,
								loaded_instrument_timbre.envelope_note_on_sustain_level,
								loaded_instrument_timbre.envelope_release_curve,
								loaded_instrument_timbre.envelope_release_duration_in_seconds,
								loaded_instrument_timbre.envelope_damper_sustain_level,
								loaded_instrument_timbre.envelope_damper_sustain_curve,
								loaded_instrument_timbre.envelope_damper_sustain_duration_in_seconds,
								true);
					m_p_tune_manager->SetMelodicChannelTimbre(
								(int8_t)channel_index,
								loaded_instrument_timbre.waveform,
								loaded_instrument_timbre.envelope_attack_curve,
								loaded_instrument_timbre.envelope_attack_duration_in_seconds,
								loaded_instrument_timbre.envelope_decay_curve,
								loaded_instrument_timbre.envelope_decay_duration_in_seconds,
								loaded_instrument_timbre.envelope_note_on_sustain_level,
								loaded_instrument_timbre.envelope_release_curve,
								loaded_instrument_timbre.envelope_release_duration_in_seconds,
								loaded_instrument_timbre.envelope_damper_sustain_level,
								loaded_instrument_timbre.envelope_damper_sustain_curve,
								loaded_instrument_timbre.envelope_damper_sustain_duration_in_seconds);
					qInfo() << Q_FUNC_INFO
							<< "loaded timbre,"
							<< "channel =" << channel_index
							<< "instrument =" << GetInstrumentNameString(channel_instrument_code);
				}while(0);
			}
		}while(0);

	} while(0);
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::on_StoreTimbresPushButton_released(void)
{
	InstrumentTimbreIniFile timbre_ini_file(INSTRUMENT_TIMBRES_INI_FILE_NAME_STRING);

	QMap<int8_t, instrument_timbre_t> ini_instrument_timbre_map;
	timbre_ini_file.ReadTimbres(&ini_instrument_timbre_map);
	ChannelListWidget * const p_channel_list_widget = ui->TimbreListWidget->findChild<ChannelListWidget*>();
	do
	{
		if(nullptr == p_channel_list_widget){
			break;
		}
		bool is_changed = false;
		QList<QPair<int, int>> const channel_instrument_pair_list =
				m_p_tune_manager->GetChannelInstrumentPairList();
		for(int i = 0; i < channel_instrument_pair_list.size(); i++){
			int const channel_index = channel_instrument_pair_list.at(i).first;
			int const channel_instrument_code = channel_instrument_pair_list.at(i).second;
			if(MIDI_PERCUSSION_CHANNEL == channel_index){
				continue;
			}
			if(false == p_channel_list_widget->IsOutputEnabled(channel_index)){
				continue;
			}

			instrument_timbre_t const channel_instrument_timbre
					= GetChannelInstrumentTimbre(p_channel_list_widget, channel_index);

			if(false == ini_instrument_timbre_map.contains((int8_t)channel_instrument_code)){
				instrument_timbre_t const default_instrument_timbre = GetDefaultInstrumentTimbre();
				if(true == IsInstrumentTimbreIdentical(&default_instrument_timbre, &channel_instrument_timbre)){
					continue;
				}
				is_changed = true;
				break;
			}

			instrument_timbre_t const loaded_instrument_timbre
					= ini_instrument_timbre_map.value((int8_t)channel_instrument_code);
			if(false == IsInstrumentTimbreIdentical(&loaded_instrument_timbre, &channel_instrument_timbre)){
				is_changed = true;
				break;
			}
		}

		if(false == is_changed){
			break;
		}
		if(QMessageBox::No == QMessageBox::question(this, QStringLiteral("Store Timbres"),
													tr("Store changed timbres to the INI file?"))){
			break;
		}

		for(int i = 0; i < channel_instrument_pair_list.size(); i++){
			int const channel_index = channel_instrument_pair_list.at(i).first;
			int const channel_instrument_code = channel_instrument_pair_list.at(i).second;
			do
			{
				if(MIDI_PERCUSSION_CHANNEL == channel_index){
					break;
				}
				if(false == p_channel_list_widget->IsOutputEnabled(channel_index)){
					break;
				}

				instrument_timbre_t const channel_instrument_timbre
						= GetChannelInstrumentTimbre(p_channel_list_widget, channel_index);
				bool is_to_write = true;
				do
				{
					instrument_timbre_t const default_instrument_timbre = GetDefaultInstrumentTimbre();
					if(true == IsInstrumentTimbreIdentical(&default_instrument_timbre,
														   &channel_instrument_timbre)){
						is_to_write = false;
						if(true == ini_instrument_timbre_map.contains((int8_t)channel_instrument_code)){
							timbre_ini_file.RemoveTimbre((int8_t)channel_instrument_code);
							qInfo() << Q_FUNC_INFO
									<< "removed stored timbre,"
									<< "channel =" << channel_index
									<< "instrument =" << GetInstrumentNameString(channel_instrument_code)
									<< "reverted to default";
						}
						break;
					}

					if(true == ini_instrument_timbre_map.contains((int8_t)channel_instrument_code)){
						instrument_timbre_t const ini_instrument_timbre
								= ini_instrument_timbre_map.value((int8_t)channel_instrument_code);
						if(true == IsInstrumentTimbreIdentical(&ini_instrument_timbre,
															   &channel_instrument_timbre)){
							is_to_write = false;
						}
					}
				}while(0);
				if(false == is_to_write){
					break;
				}

				timbre_ini_file.WriteTimbre((int8_t)channel_instrument_code,
											channel_instrument_timbre.waveform,
											channel_instrument_timbre.envelope_attack_curve,
											channel_instrument_timbre.envelope_attack_duration_in_seconds,
											channel_instrument_timbre.envelope_decay_curve,
											channel_instrument_timbre.envelope_decay_duration_in_seconds,
											channel_instrument_timbre.envelope_note_on_sustain_level,
											channel_instrument_timbre.envelope_release_curve,
											channel_instrument_timbre.envelope_release_duration_in_seconds,
											channel_instrument_timbre.envelope_damper_sustain_level,
											channel_instrument_timbre.envelope_damper_sustain_curve,
											channel_instrument_timbre.envelope_damper_sustain_duration_in_seconds);
				qInfo() << Q_FUNC_INFO
						<< "stored timbre,"
						<< "channel =" << channel_index
						<< "instrument =" << GetInstrumentNameString(channel_instrument_code);
			}while(0);
		}
	} while(0);
}



#define SYNTHESIZER_NOTE_MIDI_CHANNEL				(0)
#define SYNTHESIZER_NOTE_NUMBER						(69)
#define SYNTHESIZER_NOTE_ON_VELOCITY				(127)
#define SYNTHESIZER_NOTE_OFF_VELOCITY				(64)
#define SYNTHESIZER_NOTE_SHORTCUT_KEY				(Qt::Key_H)

/**********************************************************************************/
static uint32_t MakeShortMidiMessage(uint8_t const status_byte, uint8_t const data_byte_1, uint8_t const data_byte_2)
{
	return (uint32_t)status_byte | ((uint32_t)data_byte_1 << 8) | ((uint32_t)data_byte_2 << 16);
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

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::keyPressEvent(QKeyEvent *event)
{
	do
	{
		if(SYNTHESIZER_NOTE_SHORTCUT_KEY != event->key()){
			break;
		}
		if(true == event->isAutoRepeat()){
			event->accept();
			return;
		}
		ui->NotePushButton->setDown(true);
		on_NotePushButton_pressed();
		event->accept();
		return;
	} while(0);
	QWidget::keyPressEvent(event);
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::keyReleaseEvent(QKeyEvent *event)
{
	do
	{
		if(SYNTHESIZER_NOTE_SHORTCUT_KEY != event->key()){
			break;
		}
		if(true == event->isAutoRepeat()){
			event->accept();
			return;
		}
		ui->NotePushButton->setDown(false);
		on_NotePushButton_released();
		event->accept();
		return;
	} while(0);
	QWidget::keyReleaseEvent(event);
}
