#include <QThread>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QGridLayout>
#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QPair>
#include <QStringList>
#include <QTimer>
#include <QTimerEvent>
#include <functional>

#include "ui_ChiptuneMidiSynthesizerWidgetForm.h"

#include "chiptune_midi_define.h"

#include "ChannelListWidget.h"
#include "ChiptuneMidiValues.h"
#include "InstrumentTimbreIniFile.h"
#include "WaveChartView.h"
#include "MidiInputManager.h"
#include "SynthesizerSequencerWidget.h"

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

#define EXPAND_ENUM(ITEM, VAL)						ITEM = VAL,
enum MidiInstrumentCode
{
	MIDI_INSTRUMENT_CODE_LIST(EXPAND_ENUM)
};

#define SYNTHESIZER_AUDIO_BUFFER_UNIT_IN_MILLISECONDS					(30)
#define SYNTHESIZER_SEQUENCER_UPDATE_INTERVAL_IN_MILLISECONDS			(30)

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	#define SYNTHESIZER_AUDIO_BUFFER_TIME_FACTOR    					(2)
#else
	#define SYNTHESIZER_AUDIO_BUFFER_TIME_FACTOR    					(6)
#endif

#define SYNTHESIZER_AUDIO_PLAYER_BUFFER_IN_MILLISECONDS						\
	(SYNTHESIZER_AUDIO_BUFFER_TIME_FACTOR * SYNTHESIZER_AUDIO_BUFFER_UNIT_IN_MILLISECONDS)

#define SYNTHESIZER_MIDI_INPUT_PORT_INQUIRY_INTERVAL_IN_MILLISECONDS			(2000)
#define SYNTHESIZER_INSTRUMENT_CHANGED_INDICATOR_DURATION_IN_MILLISECONDS		(5000)

#define INSTRUMENT_TIMBRES_INI_FILE_NAME_STRING	QStringLiteral("instrument_timbres.ini")

/**********************************************************************************/
static void SetupChannelListWidget(ChannelListWidget * const p_channel_list_widget, TuneManager * const p_tune_manager)
{
	instrument_timbre_t const default_instrument_timbre = GetDefaultInstrumentTimbre();
	for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; ++channel_index){
		p_channel_list_widget->AddChannel(channel_index, AcousticGrandPiano, true, true);

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
ChiptuneMidiSynthesizerWidget::ChiptuneMidiSynthesizerWidget(TuneManager * p_tune_manager, QWidget *parent)
	: QWidget(parent),
	  m_p_tune_manager(p_tune_manager),
	  m_p_audio_player(nullptr),
	  m_p_wave_chartview(nullptr),
	  m_p_midi_input_manager(nullptr),
	  m_p_channel_list_widget(nullptr),
	  m_p_synthesizer_sequencer_widget(nullptr),
	  m_audio_player_buffer_in_milliseconds(SYNTHESIZER_AUDIO_PLAYER_BUFFER_IN_MILLISECONDS),
	  m_inquiry_midi_input_port_timer_id(-1),
	  m_synthesizer_sequencer_update_timer_id(-1),
	  ui(new Ui::ChiptuneMidiSynthesizerWidget)
{
	ui->setupUi(this);

	for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; channel_index++){
		m_channel_note_on_count_array[channel_index] = 0;
	}

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

	ui->SequencerWaterfallPushButton->setToolTip(tr("Show the live sequencer in waterfall mode"));
	ui->SequencerRollPushButton->setToolTip(tr("Show the live sequencer in roll mode"));
	ui->LoadTimbresPushButton->setToolTip(tr("Load instrument timbre parameters from %1")
										  .arg(INSTRUMENT_TIMBRES_INI_FILE_NAME_STRING));
	ui->StoreTimbresPushButton->setToolTip(tr("Store instrument timbre parameters for output-enabled channels to %1")
										   .arg(INSTRUMENT_TIMBRES_INI_FILE_NAME_STRING));

	m_p_channel_list_widget = new ChannelListWidget(ui->TimbreListWidget);
	QObject::connect(m_p_channel_list_widget, &ChannelListWidget::OutputEnabled,
					 this, &ChiptuneMidiSynthesizerWidget::HandleChannelOutputEnabled);
	QObject::connect(m_p_channel_list_widget, &ChannelListWidget::MelodicChannelTimbreChanged,
					 this, &ChiptuneMidiSynthesizerWidget::HandleMelodicChannelTimbreChanged);
	FillWidget(m_p_channel_list_widget, ui->TimbreListWidget);
	SetupChannelListWidget(m_p_channel_list_widget, m_p_tune_manager);

	m_p_synthesizer_sequencer_widget = new SynthesizerSequencerWidget(ui->SequencerScrollArea);
	SynthesizerSequencerWidget::ViewMode sequencer_view_mode =
			SynthesizerSequencerWidget::ViewModeWaterfall;
	if(true == ui->SequencerRollPushButton->isChecked()){
		sequencer_view_mode = SynthesizerSequencerWidget::ViewModeRoll;
	}
	m_p_synthesizer_sequencer_widget->SetViewMode(sequencer_view_mode);
	m_synthesizer_sequencer_update_timer_id =
			QObject::startTimer(SYNTHESIZER_SEQUENCER_UPDATE_INTERVAL_IN_MILLISECONDS);

#define SYNTHESIZER_AUDIO_PLAYER_THREAD_STARTUP_DELAY_IN_MILLISECONDS	(50)
	QTimer::singleShot(SYNTHESIZER_AUDIO_PLAYER_THREAD_STARTUP_DELAY_IN_MILLISECONDS,
					   m_p_audio_player, &AudioPlayer::Play);

	m_p_midi_input_manager = new MidiInputManager(this);
	QObject::connect(m_p_midi_input_manager, &MidiInputManager::MidiMessageDelivered,
					 m_p_tune_manager, &TuneManager::SendMidiMessage, Qt::QueuedConnection);
	QObject::connect(m_p_midi_input_manager, &MidiInputManager::MidiMessageDelivered,
					 this, &ChiptuneMidiSynthesizerWidget::HandleMidiMessageDelivered, Qt::QueuedConnection);

	UpdateInputPortComboBoxItems();
	if(false == m_p_midi_input_manager->IsPortOpen()){
		m_inquiry_midi_input_port_timer_id =
				QObject::startTimer(SYNTHESIZER_MIDI_INPUT_PORT_INQUIRY_INTERVAL_IN_MILLISECONDS);
	}
}

/**********************************************************************************/
ChiptuneMidiSynthesizerWidget::~ChiptuneMidiSynthesizerWidget()
{
	if(-1 != m_synthesizer_sequencer_update_timer_id){
		QObject::killTimer(m_synthesizer_sequencer_update_timer_id);
		m_synthesizer_sequencer_update_timer_id = -1;
	}

	if(-1 != m_inquiry_midi_input_port_timer_id){
		QObject::killTimer(m_inquiry_midi_input_port_timer_id);
		m_inquiry_midi_input_port_timer_id = -1;
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

	//delete m_p_synthesizer_sequencer_widget
	m_p_synthesizer_sequencer_widget = nullptr;

	delete ui;
}

/**********************************************************************************/
static QStringList GetComboBoxItemList(QComboBox const * const p_combo_box)
{
	QStringList item_string_list;
	for(int i = 0; i < p_combo_box->count(); i++){
		item_string_list << p_combo_box->itemText(i);
	}
	return item_string_list;
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::UpdateInputPortComboBoxItems(void)
{
	do{
		if(nullptr == m_p_midi_input_manager){
			break;
		}
		QStringList const midi_input_port_name_list = m_p_midi_input_manager->GetPortNameList();
		if(true == midi_input_port_name_list.isEmpty()){
			bool const is_input_port_still_checked =
					ui->OpenCloseInputPortPushButton->isChecked();
			if(true == is_input_port_still_checked){
				for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; channel_index++){
					uint32_t const midi_message =
							(uint32_t)(MIDI_MESSAGE_CONTROL_CHANGE | channel_index)
							| ((uint32_t)MIDI_CC_ALL_SOUND_OFF << 8);
					m_p_tune_manager->SendMidiMessage(midi_message);
					UpdateIndicatorsAndSequencerByMidiMessage(midi_message);
				}
				m_p_midi_input_manager->ClosePort();
			}
			ui->OpenCloseInputPortPushButton->setChecked(false);
			ui->OpenCloseInputPortPushButton->setText("Open");
			ui->OpenCloseInputPortPushButton->setEnabled(false);
			ui->InputPortComboBox->setEnabled(false);
			break;
		}
		if(GetComboBoxItemList(ui->InputPortComboBox) != midi_input_port_name_list){
			qInfo() << "MIDI input ports:" << midi_input_port_name_list;
			QString const selected_port_name_string = ui->InputPortComboBox->currentText();
			ui->InputPortComboBox->clear();
			ui->InputPortComboBox->addItems(midi_input_port_name_list);
			int const selected_port_index = midi_input_port_name_list.indexOf(selected_port_name_string);
			if(0 <= selected_port_index){
				ui->InputPortComboBox->setCurrentIndex(selected_port_index);
			}
		}
		ui->OpenCloseInputPortPushButton->setEnabled(true);
	}while(0);
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
void ChiptuneMidiSynthesizerWidget::HandleMidiMessageDelivered(uint32_t midi_message)
{
	UpdateIndicatorsAndSequencerByMidiMessage(midi_message);
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::UpdateIndicatorsAndSequencerByMidiMessage(
		uint32_t const midi_message)
{
	qint64 const current_timestamp_in_ms = QDateTime::currentMSecsSinceEpoch();
	uint8_t const status_byte = (uint8_t)(midi_message & 0xFF);
	uint8_t const midi_message_type = (uint8_t)(status_byte & 0xF0);
	int const channel_index = status_byte & 0x0F;

	bool const is_previous_to_highlight
			= (0 != m_channel_note_on_count_array[channel_index]) ? true : false;
	do
	{
		if(MIDI_MESSAGE_NOTE_OFF == midi_message_type
				|| MIDI_MESSAGE_NOTE_ON == midi_message_type){
			do
			{
				if(MIDI_MESSAGE_NOTE_OFF == midi_message_type){
					if(m_channel_note_on_count_array[channel_index] > 0){
						m_channel_note_on_count_array[channel_index] -= 1;
					}
					break;
				}
				m_channel_note_on_count_array[channel_index] += 1;
			}while(0);

			int const note_number = (uint8_t)((midi_message >> 8) & 0xFF);
			m_p_synthesizer_sequencer_widget->SendNoteEvent(
						SynthesizerSequencerNoteEvent(
							(MIDI_MESSAGE_NOTE_OFF == midi_message_type)
							? SynthesizerSequencerNoteEvent::NoteOff
							: SynthesizerSequencerNoteEvent::NoteOn,
							channel_index,
							note_number,
							current_timestamp_in_ms));
			break;
		}

		if(MIDI_MESSAGE_CONTROL_CHANGE == midi_message_type){
			uint8_t const control_code = (uint8_t)((midi_message >> 8) & 0xFF);
			if(MIDI_CC_ALL_SOUND_OFF == control_code
					|| MIDI_CC_ALL_NOTES_OFF == control_code){
				m_channel_note_on_count_array[channel_index] = 0;
				m_p_synthesizer_sequencer_widget->SendAllNotesOffEvent(
							channel_index,
							current_timestamp_in_ms);
			}
			break;
		}

		if(MIDI_MESSAGE_PROGRAM_CHANGE == midi_message_type){
			int const instrument_code = (uint8_t)((midi_message >> 8) & 0xFF);
			if(nullptr != m_p_channel_list_widget){
				m_p_channel_list_widget->SetMelodicChannelInstrument(channel_index, instrument_code);
				ApplyMelodicChannelInstrumentTimbre(channel_index, instrument_code, true);

				m_p_channel_list_widget->SetChannelNodeIndicator(channel_index, true);
				std::function<void()> set_indicator_not_highlight_function = [this, channel_index](){
					do
					{
						if(0 != m_channel_note_on_count_array[channel_index]){
							break;
						}
						if(nullptr == m_p_channel_list_widget){
							break;
						}
						m_p_channel_list_widget->SetChannelNodeIndicator(channel_index, false);
					}while(0);
				};
				QTimer::singleShot(SYNTHESIZER_INSTRUMENT_CHANGED_INDICATOR_DURATION_IN_MILLISECONDS,
								   this,
								   set_indicator_not_highlight_function);
			}
			break;
		}
	} while(0);

	bool const is_current_to_highlight
			= (0 != m_channel_note_on_count_array[channel_index]) ? true : false;
	if(is_previous_to_highlight != is_current_to_highlight){
		if(nullptr != m_p_channel_list_widget){
			m_p_channel_list_widget->SetChannelNodeIndicator(channel_index,
															   is_current_to_highlight);
		}
	}
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::timerEvent(QTimerEvent *event)
{
	QWidget::timerEvent(event);

	if(event->timerId() == m_inquiry_midi_input_port_timer_id){
		UpdateInputPortComboBoxItems();
	}

	if(event->timerId() == m_synthesizer_sequencer_update_timer_id){
		if(nullptr != m_p_synthesizer_sequencer_widget){
			qint64 const current_timestamp_in_ms = QDateTime::currentMSecsSinceEpoch();
			m_p_synthesizer_sequencer_widget->Update();
			m_p_synthesizer_sequencer_widget->Prepare(current_timestamp_in_ms);
		}
	}
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
void ChiptuneMidiSynthesizerWidget::ApplyMelodicChannelInstrumentTimbre(
		int channel_index, int instrument_code, bool is_to_darker_title_for_a_while)
{
	do
	{
		if(nullptr == m_p_channel_list_widget){
			break;
		}
		if(MIDI_PERCUSSION_CHANNEL == channel_index){
			break;
		}

		instrument_timbre_t instrument_timbre = GetDefaultInstrumentTimbre();
		if(true == m_ini_instrument_timbre_map.contains((int8_t)instrument_code)){
			instrument_timbre = m_ini_instrument_timbre_map.value((int8_t)instrument_code);
		}

		instrument_timbre_t const channel_instrument_timbre
				= GetChannelInstrumentTimbre(m_p_channel_list_widget, channel_index);
		if(true == IsInstrumentTimbreIdentical(&instrument_timbre, &channel_instrument_timbre)){
			break;
		}

		m_p_channel_list_widget->SetMelodicChannelTimbre(
					channel_index,
					instrument_timbre.waveform,
					instrument_timbre.envelope_attack_curve,
					instrument_timbre.envelope_attack_duration_in_seconds,
					instrument_timbre.envelope_decay_curve,
					instrument_timbre.envelope_decay_duration_in_seconds,
					instrument_timbre.envelope_note_on_sustain_level,
					instrument_timbre.envelope_release_curve,
					instrument_timbre.envelope_release_duration_in_seconds,
					instrument_timbre.envelope_damper_sustain_level,
					instrument_timbre.envelope_damper_sustain_curve,
					instrument_timbre.envelope_damper_sustain_duration_in_seconds,
					is_to_darker_title_for_a_while);
		m_p_tune_manager->SetMelodicChannelTimbre(
					(int8_t)channel_index,
					instrument_timbre.waveform,
					instrument_timbre.envelope_attack_curve,
					instrument_timbre.envelope_attack_duration_in_seconds,
					instrument_timbre.envelope_decay_curve,
					instrument_timbre.envelope_decay_duration_in_seconds,
					instrument_timbre.envelope_note_on_sustain_level,
					instrument_timbre.envelope_release_curve,
					instrument_timbre.envelope_release_duration_in_seconds,
					instrument_timbre.envelope_damper_sustain_level,
					instrument_timbre.envelope_damper_sustain_curve,
					instrument_timbre.envelope_damper_sustain_duration_in_seconds);
	}while(0);
}

/**********************************************************************************/
int ChiptuneMidiSynthesizerWidget::LoadAndApplyTimbres(void)
{
	InstrumentTimbreIniFile timbre_ini_file(INSTRUMENT_TIMBRES_INI_FILE_NAME_STRING);
	int const ret = timbre_ini_file.ReadTimbres(&m_ini_instrument_timbre_map);

	do{
		if(-1 == ret){
			break;
		}

		if(nullptr == m_p_channel_list_widget){
			break;
		}

		for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; channel_index++){
			int const channel_instrument_code =
					m_p_tune_manager->GetCurrentChannelInstrument(channel_index);
			do
			{
				if(MIDI_PERCUSSION_CHANNEL == channel_index){
					break;
				}

				instrument_timbre_t const channel_instrument_timbre
						= GetChannelInstrumentTimbre(m_p_channel_list_widget, channel_index);
				ApplyMelodicChannelInstrumentTimbre(channel_index, channel_instrument_code, true);
				instrument_timbre_t const applied_instrument_timbre
						= GetChannelInstrumentTimbre(m_p_channel_list_widget, channel_index);
				if(true == IsInstrumentTimbreIdentical(&applied_instrument_timbre, &channel_instrument_timbre)){
					break;
				}
				qInfo() << Q_FUNC_INFO
						<< "applied timbre,"
						<< "channel =" << channel_index
						<< "instrument =" << GetInstrumentNameString(channel_instrument_code);
			}while(0);
		}
	}while(0);

	return ret;
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::on_LoadTimbresPushButton_toggled(bool is_checked)
{
	do {
		if(false == is_checked){
			m_ini_instrument_timbre_map.clear();
			do{
				if(nullptr == m_p_channel_list_widget){
					break;
				}
				for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; channel_index++){
					int const channel_instrument_code =
							m_p_tune_manager->GetCurrentChannelInstrument(channel_index);
					ApplyMelodicChannelInstrumentTimbre(channel_index, channel_instrument_code, true);
				}
			}while(0);
			break;
		}

		int const ret = LoadAndApplyTimbres();
		if(-1 == ret){
			m_ini_instrument_timbre_map.clear();
			ui->LoadTimbresPushButton->setChecked(false);
			QMessageBox::warning(this, QStringLiteral("Load Timbres"),
								 tr("%1 not found.").arg(INSTRUMENT_TIMBRES_INI_FILE_NAME_STRING));
			break;
		}

	} while(0);
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::on_StoreTimbresPushButton_released(void)
{
	InstrumentTimbreIniFile timbre_ini_file(INSTRUMENT_TIMBRES_INI_FILE_NAME_STRING);

	QMap<int8_t, instrument_timbre_t> ini_instrument_timbre_map;
	timbre_ini_file.ReadTimbres(&ini_instrument_timbre_map);
	do
	{
		if(nullptr == m_p_channel_list_widget){
			break;
		}
		bool is_changed = false;
		for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; channel_index++){
			int const channel_instrument_code =
					m_p_tune_manager->GetCurrentChannelInstrument(channel_index);
			if(MIDI_PERCUSSION_CHANNEL == channel_index){
				continue;
			}
			if(false == m_p_channel_list_widget->IsOutputEnabled(channel_index)){
				continue;
			}

			instrument_timbre_t const channel_instrument_timbre
					= GetChannelInstrumentTimbre(m_p_channel_list_widget, channel_index);

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

		for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; channel_index++){
			int const channel_instrument_code =
					m_p_tune_manager->GetCurrentChannelInstrument(channel_index);
			do
			{
				if(MIDI_PERCUSSION_CHANNEL == channel_index){
					break;
				}
				if(false == m_p_channel_list_widget->IsOutputEnabled(channel_index)){
					break;
				}

				instrument_timbre_t const channel_instrument_timbre
						= GetChannelInstrumentTimbre(m_p_channel_list_widget, channel_index);
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

		if(true == ui->LoadTimbresPushButton->isChecked()){
			LoadAndApplyTimbres();
		}
	} while(0);
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::on_OpenCloseInputPortPushButton_toggled(bool is_checked)
{
	do
	{
		if(nullptr == m_p_midi_input_manager){
			break;
		}

		if(false == is_checked){
			m_p_midi_input_manager->ClosePort();
			break;
		}

		if(0 > ui->InputPortComboBox->currentIndex()){
			break;
		}

		m_p_midi_input_manager->OpenPort((unsigned int)ui->InputPortComboBox->currentIndex());
	} while(0);

	bool is_port_opened = false;
	if(nullptr != m_p_midi_input_manager){
		if(true == m_p_midi_input_manager->IsPortOpen()){
			is_port_opened = true;
		}
	}

	if(true == is_port_opened){
		if(-1 != m_inquiry_midi_input_port_timer_id){
			QObject::killTimer(m_inquiry_midi_input_port_timer_id);
			m_inquiry_midi_input_port_timer_id = -1;
		}
	}
	else {
		if(-1 == m_inquiry_midi_input_port_timer_id){
			m_inquiry_midi_input_port_timer_id =
					QObject::startTimer(SYNTHESIZER_MIDI_INPUT_PORT_INQUIRY_INTERVAL_IN_MILLISECONDS);
		}
	}

	ui->OpenCloseInputPortPushButton->setChecked(is_port_opened);
	ui->InputPortComboBox->setEnabled(false == is_port_opened);
	ui->OpenCloseInputPortPushButton->setText((true == is_port_opened) ? "Close" : "Open");
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::on_SequencerRollPushButton_toggled(bool is_checked)
{
	do
	{
		if(false == is_checked){
			break;
		}
		m_p_synthesizer_sequencer_widget->SetViewMode(
					SynthesizerSequencerWidget::ViewModeRoll);
	}while(0);
}

/**********************************************************************************/
void ChiptuneMidiSynthesizerWidget::on_SequencerWaterfallPushButton_toggled(bool is_checked)
{
	do
	{
		if(false == is_checked){
			break;
		}
		m_p_synthesizer_sequencer_widget->SetViewMode(
					SynthesizerSequencerWidget::ViewModeWaterfall);
	}while(0);
}
