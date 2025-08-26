#include <QDebug>

#include <QThread>
#include <QGridLayout>
#include <QElapsedTimer>
#include <QDateTime>
#include <QTimerEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QFile>
#include <QLineEdit>

#if QT_VERSION_CHECK(6, 0, 0) > QT_VERSION
#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif
#endif

#include "ui_ChiptuneMidiWidgetForm.h"
#include "ProgressSlider.h"

#include "WaveChartView.h"
#include "SequencerWidget.h"

#include "ChannelListWidget.h"

#include "ChiptuneMidiWidget.h"

struct wav_header_t
{
	char chunk_id[4];							// RIFF string
	unsigned int chunk_size;					// overall size of file in bytes
	char format[4];								// WAVE string
	char subchunk1_id[4];						// fmt string with trailing null char
	unsigned int subchunk1_size;				// length of the format data as listed above
	unsigned short audio_format;					// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	unsigned short number_of_channels;			// no.of channels
	unsigned int sampling_rate;					// sampling rate (blocks per second)
	unsigned int byte_rate;						// SampleRate * NumChannels * BitsPerSample/8
	unsigned short block_align;					// NumChannels * BitsPerSample/8
	unsigned short bits_per_sample;				// bits per sample, 8- 8bits, 16- 16 bits etc
	char subchunk2_id[4];						// DATA string or FLLR string
	unsigned int subchunk2_size;				// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
};

static struct wav_header_t s_wave_header;

unsigned char const * const wav_file_header(unsigned int const number_of_channels, unsigned int const sampling_rate,
											unsigned int bits_per_sample, unsigned int wave_data_length,
											int * const p_this_header_size)
{
	s_wave_header.chunk_id[0] = 'R';
	s_wave_header.chunk_id[1] = 'I';
	s_wave_header.chunk_id[2] = 'F';
	s_wave_header.chunk_id[3] = 'F';

	s_wave_header.format[0] = 'W';
	s_wave_header.format[1] = 'A';
	s_wave_header.format[2] = 'V';
	s_wave_header.format[3] = 'E';

	s_wave_header.subchunk1_id[0] = 'f';
	s_wave_header.subchunk1_id[1] = 'm';
	s_wave_header.subchunk1_id[2] = 't';
	s_wave_header.subchunk1_id[3] = ' ';

	s_wave_header.subchunk2_id[0] = 'd';
	s_wave_header.subchunk2_id[1] = 'a';
	s_wave_header.subchunk2_id[2] = 't';
	s_wave_header.subchunk2_id[3] = 'a';

	s_wave_header.subchunk1_size = 16;
	s_wave_header.audio_format = 1;
	s_wave_header.number_of_channels = number_of_channels;
	s_wave_header.sampling_rate = sampling_rate;
	s_wave_header.byte_rate = number_of_channels * sampling_rate * bits_per_sample / 8;
	s_wave_header.block_align = number_of_channels * bits_per_sample / 8;
	s_wave_header.bits_per_sample = bits_per_sample;

	s_wave_header.subchunk2_size = wave_data_length;
	s_wave_header.chunk_size = wave_data_length + (sizeof(struct wav_header_t) - 8);

	*p_this_header_size = sizeof(struct wav_header_t);
	return (unsigned char const * const)&s_wave_header;
}

/**********************************************************************************/

int SaveAsWavFile(TuneManager *p_tune_manager, QString filename)
{
	if(false == filename.endsWith(".wav", Qt::CaseInsensitive)){
		filename.append(".wav");
	}
	qDebug() << "saving as wav file " << filename;

	QElapsedTimer elasped_timer;

	elasped_timer.start();
	p_tune_manager->SetStartTimeInSeconds(0);
	int data_buffer_size = p_tune_manager->GetNumberOfChannels()
			* p_tune_manager->GetSamplingRate() * p_tune_manager->GetSamplingSize()/8;
	QByteArray wave_bytearray;
	while(1)
	{
		wave_bytearray += p_tune_manager->FetchWave(data_buffer_size);
		if(true == p_tune_manager->IsTuneEnding()){
			break;
		}
	}
	qDebug() << "Generate wave data elpased" << elasped_timer.elapsed() << "ms";
	int header_size = 0;
	char *p_wav_header = (char*)wav_file_header(
				p_tune_manager->GetNumberOfChannels(), p_tune_manager->GetSamplingRate(),
				p_tune_manager->GetSamplingSize(), wave_bytearray.size(), &header_size);

	QFile file(filename);
	if(false == file.open(QIODevice::ReadWrite | QIODevice::Truncate)){
		qDebug() << "open file fail";
		return -2;
	}

	int file_size = file.write(QByteArray(p_wav_header, header_size) + wave_bytearray);
	file.close();

	qDebug() << "file saved, size = " << file_size << " bytes";
	return 0;
}

class SaveAsWavFileThread: public QThread {
public :
	SaveAsWavFileThread(TuneManager *p_tune_manager, QString filename)
		: m_p_tune_manager(p_tune_manager), m_filename(filename){}
protected:
	void run(void) Q_DECL_OVERRIDE { SaveAsWavFile(m_p_tune_manager, m_filename); }
private:
	TuneManager *m_p_tune_manager;
	QString m_filename;
};

/**********************************************************************************/

void FillWidget(QWidget *p_widget, QWidget *p_filled_widget)
{
	QGridLayout *p_layout = new QGridLayout(p_filled_widget);
	p_layout->addWidget(p_widget, 0, 0);
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->setSpacing(0);
}

#if QT_VERSION_CHECK(6, 0, 0) > QT_VERSION
/**********************************************************************************/

void SetFontSizeForWidgetSubtree(QWidget * const p_root_widget, int const target_point_size,
								 bool const is_to_set_only_if_point_size_matches_root)
{
	int const original_font_point_size = p_root_widget->QWidget::font().pointSize();

	{
		QFont font = p_root_widget->QWidget::font();
		font.setPointSize(target_point_size);
	}
	QList<QWidget*> widget_pointer_list = p_root_widget->QObject::findChildren<QWidget*>();
	QListIterator<QWidget*> widget_pointer_iterator(widget_pointer_list);
	while(widget_pointer_iterator.hasNext())
	{
		QWidget *p_widget = widget_pointer_iterator.next();

		bool is_to_set = true;
		do
		{
			if(false == is_to_set_only_if_point_size_matches_root){
				break;
			}
			if(p_widget->QWidget::font().pointSize() != original_font_point_size){
				is_to_set = false;
			}
		} while (0);
		if(true == is_to_set){
			QFont font = p_widget->QWidget::font();
			font.setPointSize(target_point_size);
			p_widget->QWidget::setFont(font);
		}
	}
}
#endif

/**********************************************************************************/

ChiptuneMidiWidget::ChiptuneMidiWidget(TuneManager *const p_tune_manager, QWidget *parent)
	: QWidget(parent),
	m_p_tune_manager(p_tune_manager),

	m_inquiring_playback_status_timer_id(-1),
	m_inquiring_playback_tick_timer_id(-1),
	m_audio_player_buffer_in_milliseconds(200),
	m_p_sequencer_widget(nullptr),
	ui(new Ui::ChiptuneMidiWidget)
{
	ui->setupUi(this);
#if QT_VERSION_CHECK(6, 0, 0) > QT_VERSION
	SetFontSizeForWidgetSubtree(this, QWidget::font().pointSize() + 2, true);
#endif
	{
		QFont font20("Monospace");
		font20.setStyleHint(QFont::TypeWriter);
		font20.setPixelSize(20);
		ui->PlayPositionLabel->setFont(font20);

		font20 = ui->MessageLabel->font();
		font20.setStyleHint(QFont::SansSerif);
		font20.setPixelSize(20);
		ui->MessageLabel->setFont(font20);
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
									   m_audio_player_buffer_in_milliseconds/2, nullptr);
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

	QObject::connect(ui->PlayProgressSlider, &ProgressSlider::MousePressed, this,
						 &ChiptuneMidiWidget::HandlePlayProgressSliderMousePressed);
	QObject::connect(m_p_audio_player, &AudioPlayer::StateChanged,
					 this, &ChiptuneMidiWidget::HandleAudioPlayerStateChanged, Qt::QueuedConnection);

	ui->OpenMidiFilePushButton->setToolTip(tr("Open MIDI File"));
	ui->SaveSaveFilePushButton->setToolTip(tr("Save as .wav file"));

	ui->PitchShiftSpinBox->setToolTip(tr("Shift the pitch in the unit of semitone"));
	QLineEdit *p_lineEdit = ui->PitchShiftSpinBox->findChild<QLineEdit *>();
	p_lineEdit->setReadOnly(true);

	QWidget::setFocusPolicy(Qt::StrongFocus);
	QWidget::setFixedSize(QWidget::size());
	QWidget::setAcceptDrops(true);

	qApp->installEventFilter(this);
}

/**********************************************************************************/

ChiptuneMidiWidget::~ChiptuneMidiWidget()
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
}

/**********************************************************************************/

static QString FormatTimeString(qint64 timeMilliSeconds)
{
	qint64 seconds = timeMilliSeconds / 1000;
	const qint64 minutes = seconds / 60;
	seconds -= minutes * 60;
	return QStringLiteral("%1:%2")
		.arg(minutes, 2, 10, QLatin1Char('0'))
		.arg(seconds, 2, 10, QLatin1Char('0'));
}

/**********************************************************************************/
#define INQUIRING_PLACKBACK_TICK_INTERVAL_IN_MILLISECONDS		(35)

int ChiptuneMidiWidget::PlayMidiFile(QString filename_string)
{
	StopMidiFile();

	QThread::msleep(10);
	QString message_string;
	m_opened_file_info = QFileInfo(filename_string);
	int ret = 0;
	do {
		if(0 > m_p_tune_manager->LoadMidiFile(filename_string)){
			message_string = QString::asprintf("Not a MIDI File");
			ui->MessageLabel->setText(message_string);
			ret = -1;
			break;
		}

		ChannelListWidget *p_channellist_widget = new ChannelListWidget(ui->TimbreListWidget);
		FillWidget(p_channellist_widget, ui->TimbreListWidget);
		int channel_number = m_p_tune_manager->GetChannelInstrumentPairList().size();
		for(int i = 0; i < channel_number; i++){
			int channel_index = m_p_tune_manager->GetChannelInstrumentPairList().at(i).first;
			int instrument = m_p_tune_manager->GetChannelInstrumentPairList().at(i).second;
			p_channellist_widget->AddChannel(channel_index, instrument);

			QObject::connect(p_channellist_widget, &ChannelListWidget::OutputEnabled,
							 this, &ChiptuneMidiWidget::HandleChannelOutputEnabled);
			QObject::connect(p_channellist_widget, &ChannelListWidget::TimbreChanged,
							 this, &ChiptuneMidiWidget::HandlePitchTimbreValueFrameChanged);

			int waveform;
			int envelope_attack_curve; double envelope_attack_duration_in_seconds;
			int envelope_decay_curve; double envelope_decay_duration_in_seconds;
			int envelope_sustain_level;
			int envelope_release_curve; double envelope_release_duration_in_seconds;
			int envelope_damper_on_but_note_off_sustain_level;
			int envelope_damper_on_but_note_off_sustain_curve;
			double envelope_damper_on_but_note_off_sustain_duration_in_seconds;

			p_channellist_widget->GetTimbre(channel_index,
											&waveform, &envelope_attack_curve, &envelope_attack_duration_in_seconds,
											&envelope_decay_curve, &envelope_decay_duration_in_seconds,
											&envelope_sustain_level,
											&envelope_release_curve, &envelope_release_duration_in_seconds,
											&envelope_damper_on_but_note_off_sustain_level,
											&envelope_damper_on_but_note_off_sustain_curve,
											&envelope_damper_on_but_note_off_sustain_duration_in_seconds);
			m_p_tune_manager->SetPitchChannelTimbre((int8_t)channel_index, (int8_t)waveform,
													(int8_t)envelope_attack_curve, (float)envelope_attack_duration_in_seconds,
													(int8_t)envelope_decay_curve, (float)envelope_decay_duration_in_seconds,
													(uint8_t)envelope_sustain_level,
													(int8_t)envelope_release_curve, (float)envelope_release_duration_in_seconds,
													(uint8_t)envelope_damper_on_but_note_off_sustain_level,
													(int8_t)envelope_damper_on_but_note_off_sustain_curve,
													(float)envelope_damper_on_but_note_off_sustain_duration_in_seconds);
		}
#if QT_VERSION_CHECK(6, 0, 0) > QT_VERSION
		SetFontSizeForWidgetSubtree(p_channellist_widget,
										p_channellist_widget->QWidget::font().pointSize() + 3, false);
#endif
		m_p_sequencer_widget = new SequencerWidget(m_p_tune_manager,
											   m_audio_player_buffer_in_milliseconds/1000.0,
											   ui->SequencerScrollArea);

		ui->AmplitudeGainSlider->setValue(UINT16_MAX - m_p_tune_manager->GetAmplitudeGain());
		m_midi_file_duration_in_milliseconds = (int)(1000 * m_p_tune_manager->GetMidiFileDurationInSeconds());
		m_midi_file_duration_time_string = FormatTimeString(m_midi_file_duration_in_milliseconds);
		ui->PlayPositionLabel->setText(FormatTimeString(0) + " / " + m_midi_file_duration_time_string);
		ui->PlayProgressSlider->setRange(0, m_midi_file_duration_in_milliseconds);
		ui->PlayProgressSlider->setValue(0);
		m_p_wave_chartview->Reset();

		ui->SaveSaveFilePushButton->setEnabled(true);
		message_string = QString::asprintf("Playing file");
		ui->MessageLabel->setText(message_string);
		m_inquiring_playback_status_timer_id = QObject::startTimer(500);
		m_inquiring_playback_tick_timer_id = QObject::startTimer(INQUIRING_PLACKBACK_TICK_INTERVAL_IN_MILLISECONDS);

		ui->PlayPausePushButton->setEnabled(true);
		SetPlayPauseButtonInPlayState(true);
		ui->SaveSaveFilePushButton->setEnabled(true);

		m_p_audio_player->Prime();
		SetTuneStartTimeAndCheckPlayPausePushButtonIconToPlay(0);
	}while(0);

	message_string += QString::asprintf(" :: <b>%s</b>", m_opened_file_info.fileName().toUtf8().data());
	ui->MessageLabel->setText(message_string);
	return ret;
}

/**********************************************************************************/

void ChiptuneMidiWidget::StopMidiFile(void)
{
	if(nullptr != m_p_sequencer_widget){
		delete m_p_sequencer_widget;
	}
	m_p_sequencer_widget = nullptr;

	m_p_audio_player->Stop();
	m_p_tune_manager->ClearOutMidiFile();

	if(-1 != m_inquiring_playback_status_timer_id){
		QObject::killTimer(m_inquiring_playback_status_timer_id);
		m_inquiring_playback_status_timer_id = -1;
	}

	if(-1 != m_inquiring_playback_tick_timer_id){
		QObject::killTimer(m_inquiring_playback_tick_timer_id);
		m_inquiring_playback_tick_timer_id = -1;
	}

	ui->PlayPositionLabel->setText("00:00 / 00:00");
	ui->PlayProgressSlider->setValue(0);

	ui->MessageLabel->setText("");

	ui->PlayPausePushButton->setEnabled(false);
	SetPlayPauseButtonInPlayState(false);

	m_p_wave_chartview->Reset();

	do{
		if(nullptr == ui->SequencerScrollArea->widget()){
			break;
		}

		QWidget *p_widget = ui->SequencerScrollArea->takeWidget();
		delete p_widget->layout();
		delete p_widget;
	} while(0);

	do {
		if(nullptr == ui->TimbreListWidget->layout()){
			break;
		}
		QLayoutItem *p_layoutitem = ui->TimbreListWidget->layout()->takeAt(0);
		if(nullptr == p_layoutitem){
			break;
		}
		QWidget *p_widget = p_layoutitem->widget();
		if(nullptr == p_widget){
			break;
		}
		delete p_widget;
		delete ui->TimbreListWidget->layout();
	} while(0);

	ui->SaveSaveFilePushButton->setEnabled(false);
}

/**********************************************************************************/

#define UNICODE_QUARTER_NOTE_ICON						u8"\u2669"

void ChiptuneMidiWidget::UpdateTempoLabelText(void)
{
	double tempo = m_p_tune_manager->GetTempo();
	QString tempo_string = QString::asprintf(" = %2.0f", tempo);
	do{
		if( 0.1 < abs(tempo - (int)(tempo + 0.5))){
			tempo_string = QString::asprintf(" = %2.1f", tempo);
			break;
		}
	} while(0);

	double playing_tempo = m_p_tune_manager->GetPlayingTempo();
	do{
		if(abs(playing_tempo - tempo) < 0.1){
			break;
		}

		QString playing_tempo_string = QString::asprintf(" (%2.0f)", playing_tempo);
		if( 0.1 < abs(playing_tempo - (int)(playing_tempo + 0.5))){
			playing_tempo_string = QString::asprintf(" (%2.1f)", playing_tempo);
		}
		tempo_string += playing_tempo_string;
	}while(0);
	ui->TempoLabel->setText(UNICODE_QUARTER_NOTE_ICON + tempo_string);
}

/**********************************************************************************/

void ChiptuneMidiWidget::SetTuneStartTimeAndCheckPlayPausePushButtonIconToPlay(int start_time_in_milliseconds)
{
	if(-1 != m_inquiring_playback_status_timer_id){
		QObject::killTimer(m_inquiring_playback_status_timer_id);
		m_inquiring_playback_status_timer_id = -1;
	}

	if(true == m_defer_start_play_timer.isActive()){
		m_defer_start_play_timer.stop();
	}
	QObject::disconnect(&m_defer_start_play_timer, nullptr , nullptr, nullptr);
	ui->PlayPositionLabel->setText(FormatTimeString(start_time_in_milliseconds) + " / "
							  + m_midi_file_duration_time_string);
	m_p_tune_manager->SetStartTimeInSeconds(start_time_in_milliseconds/1000.0);
	do
	{
		if(false == IsPlayPauseButtonInPlayState()){
			break;
		}

		std::function<void()> ensure_playing_function = [this]() {
			if(AudioPlayer::PlaybackStatePlaying != m_p_audio_player->GetState()){
				//qDebug() << m_p_audio_player->GetState();
				m_p_audio_player->Play();
			}
		};

		std::function<void()> defer_start_play_function = [this, ensure_playing_function](){
			m_inquiring_playback_status_timer_id = QObject::startTimer(500);
				//qDebug() << m_p_audio_player->GetState();
				if(AudioPlayer::PlaybackStatePlaying != m_p_audio_player->GetState()){
					m_p_audio_player->Play();
				}

				m_defer_start_play_timer.setInterval(30);
				QObject::disconnect(&m_defer_start_play_timer, nullptr , nullptr, nullptr);
				QObject::connect(&m_defer_start_play_timer, &QTimer::timeout, this, ensure_playing_function);
				m_defer_start_play_timer.start();
		};

		QObject::connect(&m_defer_start_play_timer, &QTimer::timeout, this, defer_start_play_function);
		m_defer_start_play_timer.setInterval(100);
		m_defer_start_play_timer.setSingleShot(true);
		m_defer_start_play_timer.start();
	} while(0);
	UpdateTempoLabelText();
	QWidget::activateWindow();
	QWidget::setFocus();
}

/**********************************************************************************/

void ChiptuneMidiWidget::on_PlayProgressSlider_sliderMoved(int value)
{
	SetTuneStartTimeAndCheckPlayPausePushButtonIconToPlay(value);
}

/**********************************************************************************/

void ChiptuneMidiWidget::HandleAudioPlayerStateChanged(AudioPlayer::PlaybackState state)
{
	do
	{
		if(state != AudioPlayer::PlaybackStateIdle){
			break;
		}
		if(true == m_p_tune_manager->IsTuneEnding()){
			break;
		}
		if(false == IsPlayPauseButtonInPlayState()){
			break;
		}
		m_p_audio_player->Play();
	}while(0);
}

/**********************************************************************************/

void ChiptuneMidiWidget::HandlePlayProgressSliderMousePressed(Qt::MouseButton button, int value)
{
	qDebug() << Q_FUNC_INFO;
	do
	{
		if(Qt::LeftButton != button){
			break;
		}
		ui->PlayProgressSlider->setValue(value);
		SetTuneStartTimeAndCheckPlayPausePushButtonIconToPlay(value);
	}while(0);
}

/**********************************************************************************/

void ChiptuneMidiWidget::HandleChannelOutputEnabled(int index, bool is_enabled)
{
	m_p_tune_manager->SetChannelOutputEnabled(index, is_enabled);
	m_p_sequencer_widget->SetChannelToDrawEnabled(index, is_enabled);
}

/**********************************************************************************/

void ChiptuneMidiWidget::HandlePitchTimbreValueFrameChanged(int index,
										int waveform,
										int envelope_attack_curve, double envelope_attack_duration_in_seconds,
										int envelope_decay_curve, double envelope_decay_duration_in_seconds,
										int envelope_sustain_level,
										int envelope_release_curve, double envelope_release_duration_in_seconds,
										int envelope_damper_on_but_note_off_sustain_level,
										int envelope_damper_on_but_note_off_sustain_curve,
										double envelope_damper_on_but_note_off_sustain_duration_in_seconds)
{
	m_p_tune_manager->SetPitchChannelTimbre((int8_t)index, (int8_t)waveform,
											(int8_t)envelope_attack_curve, (float)envelope_attack_duration_in_seconds,
											(int8_t)envelope_decay_curve, (float)envelope_decay_duration_in_seconds,
											(uint8_t)envelope_sustain_level,
											(int8_t)envelope_release_curve, (float)envelope_release_duration_in_seconds,
											(uint8_t)envelope_damper_on_but_note_off_sustain_level,
											(int8_t)envelope_damper_on_but_note_off_sustain_curve,
											(float)envelope_damper_on_but_note_off_sustain_duration_in_seconds);
}

/**********************************************************************************/

bool ChiptuneMidiWidget::eventFilter(QObject *watched, QEvent *event)
{
	bool is_handled = false;
	bool ret = false;
	do
	{
		if(nullptr == QApplication::focusWidget()){
			break;
		}
		if(this == watched){
			break;
		}
		if(false == watched->isWidgetType()){
			break;
		}
	#if 0
		QWidget *p_event_widget = (QWidget*)watched;
		if (false == (p_event_widget == this
				   || QWidget::isAncestorOf(p_event_widget)) ) {
			break;
		}
	#endif

		if(true == watched->QObject::inherits("QAbstractSpinBox")){
			break;
		}

		//QComboBoxListView has triggered, avoid QComboBoxPrivateContainer triggers once
		if(0 == QString::compare(watched->metaObject()->className(), "QComboBoxPrivateContainer") ){
			break;
		}

		if(QEvent::KeyPress != event->type()){
			break;
		}
		QKeyEvent *p_key_event = (QKeyEvent*)event;
		if( false == (Qt::Key_Left == p_key_event->key()
					  || Qt::Key_Right == p_key_event->key()) ){
			break;
		}
		qDebug() << QApplication::focusWidget()->metaObject()->className();
		qDebug() << watched->metaObject()->className();
#if 1
		QCoreApplication::sendEvent(this, p_key_event);
#else
		QCoreApplication::postEvent(this, new QKeyEvent(p_key_event->type(), p_key_event->key(),
														p_key_event->modifiers(), p_key_event->text(),
														p_key_event->isAutoRepeat(), p_key_event->count()),
									Qt::HighEventPriority);
#endif
		ret = true;
	} while(0);

	if(false == is_handled){
		ret = QWidget::eventFilter(watched, event);
	}
	return ret;
}

/**********************************************************************************/

void ChiptuneMidiWidget::timerEvent(QTimerEvent *event)
{
	QWidget::timerEvent(event);
	do {
		if(event->timerId() == m_inquiring_playback_status_timer_id){
			if(true == m_p_tune_manager->IsTuneEnding())
			{
				if(AudioPlayer::PlaybackStateIdle == m_p_audio_player->GetState()){
					m_p_tune_manager->SetStartTimeInSeconds(0);
					ui->PlayProgressSlider->setValue(0);
					ui->PlayPositionLabel->setText("00:00 / 00:00");
					SetPlayPauseButtonInPlayState(false);
					m_p_wave_chartview->Reset();
				}
				break;
			}

			UpdateTempoLabelText();

			int elapsed_time_in_milliseconds =
					(int)(m_p_tune_manager->GetCurrentElapsedTimeInSeconds() * 1000);
			ui->PlayPositionLabel->setText(FormatTimeString(elapsed_time_in_milliseconds) + " / "
									  + m_midi_file_duration_time_string);
			ui->PlayProgressSlider->setValue(elapsed_time_in_milliseconds);

			int const amplitude_gain = UINT16_MAX - m_p_tune_manager->GetAmplitudeGain();
			if(ui->AmplitudeGainSlider->value() != amplitude_gain){
				ui->AmplitudeGainSlider->setValue(amplitude_gain);
			}
			break;
		}
	}while(0);

	if(event->timerId() == m_inquiring_playback_tick_timer_id){
		m_p_sequencer_widget->Update();
		m_p_sequencer_widget->Prepare(
					m_p_tune_manager->GetMidiFilePointer()->tickFromTime(INQUIRING_PLACKBACK_TICK_INTERVAL_IN_MILLISECONDS/1000.0f)
					+ m_p_tune_manager->GetCurrentTick());
	}
}

/**********************************************************************************/

void ChiptuneMidiWidget::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
#if QT_VERSION_CHECK(6, 0, 0) > QT_VERSION
#ifdef Q_OS_WIN
	QWinTaskbarButton *p_win_taskbar_button = new QWinTaskbarButton(this);
	p_win_taskbar_button->setWindow(QWidget::windowHandle());
	QWinTaskbarProgress *p_win_taskbar_progress = p_win_taskbar_button->progress();
	p_win_taskbar_progress->setVisible(true);
	QObject::connect(ui->PlayProgressSlider, &QAbstractSlider::valueChanged,
					 p_win_taskbar_progress, &QWinTaskbarProgress::setValue);
	QObject::connect(ui->PlayProgressSlider, &QAbstractSlider::rangeChanged,
					 p_win_taskbar_progress, &QWinTaskbarProgress::setRange);
#endif
#endif
}

/**********************************************************************************/

void ChiptuneMidiWidget::dragEnterEvent(QDragEnterEvent* event) { event->acceptProposedAction(); }

/**********************************************************************************/

void ChiptuneMidiWidget::dragMoveEvent(QDragMoveEvent* event) { event->acceptProposedAction(); }

/**********************************************************************************/

void ChiptuneMidiWidget::dragLeaveEvent(QDragLeaveEvent* event) { event->accept();}

/**********************************************************************************/

void ChiptuneMidiWidget::dropEvent(QDropEvent *event)
{
	QWidget::dropEvent(event);

	QString dropped_filename_string = event->mimeData()->urls().at(0).toLocalFile();
	QFileInfo file_info;
	do {
		if(false == event->mimeData()->hasUrls()){
			ui->MessageLabel->setText("No dropped file");
			break;
		}
		if(false == event->mimeData()->urls().at(0).isLocalFile()){
			ui->MessageLabel->setText("Not a local file");
			break;
		}

		file_info = QFileInfo(event->mimeData()->urls().at(0).toLocalFile());
		if(false == file_info.isFile()){
			QString message_string = QString::asprintf("Not a file :: <b>%s<b>", file_info.fileName().toUtf8().data());
			ui->MessageLabel->setText(message_string);
			break;
		}

		PlayMidiFile(dropped_filename_string);
	}while(0);
	QWidget::activateWindow();
	QWidget::setFocus();
}

/**********************************************************************************/

void ChiptuneMidiWidget::keyPressEvent(QKeyEvent *event)
{
	QWidget:: keyPressEvent(event);

	do {
		if(false == m_p_tune_manager->IsFileLoaded()){
			break;
		}

		if(false == (Qt::Key_Left == event->key() || Qt::Key_Right == event->key()) ){
			break;
		}

		int start_time = ui->PlayProgressSlider->value();
#define KEY_LEFT_RIGHT_DELTA_TIME_IN_SECONDS		(5)
		if(Qt::Key_Left == event->key()){
			start_time -= KEY_LEFT_RIGHT_DELTA_TIME_IN_SECONDS * 1000;
			if(start_time < 0){
				start_time = 0;
			}
		}

		if(Qt::Key_Right == event->key()){
			start_time += KEY_LEFT_RIGHT_DELTA_TIME_IN_SECONDS * 1000;
			if(start_time > m_p_tune_manager->GetMidiFileDurationInSeconds() * 1000){
				start_time = m_p_tune_manager->GetMidiFileDurationInSeconds() * 1000;
			}
		}

		ui->PlayProgressSlider->setValue(start_time);
		SetTuneStartTimeAndCheckPlayPausePushButtonIconToPlay(start_time);
	} while(0);
}

/**********************************************************************************/

void ChiptuneMidiWidget::on_OpenMidiFilePushButton_released(void)
{
	QString open_filename_string = QFileDialog::getOpenFileName(this, QString("Open the MIDI File"),
											   QString(),
											   QString("MIDI File (*.mid);;"
											   "All file (*)")
																);

	do {
		if(true == open_filename_string.isNull()){
			break;
		}
		PlayMidiFile(open_filename_string);
	} while(0);
}

/**********************************************************************************/

void ChiptuneMidiWidget::on_SaveSaveFilePushButton_released(void)
{
	m_p_audio_player->Stop();
	int playing_value = ui->PlayProgressSlider->value();

	do {
		QString suggested_filename_string = QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
		suggested_filename_string += QString(" ") + m_opened_file_info.baseName();
		suggested_filename_string += ".wav";
		QString save_filename_string = QFileDialog::getSaveFileName(this, QString("Save the Wave File"),
												   suggested_filename_string,
												   QString("Wave File (*.wav);;"
												   "All file (*)")
																	);

		if(true == save_filename_string.isNull()){
			break;
		}

		QString original_text_string = ui->MessageLabel->text();
		ui->MessageLabel->setText(QString("saving file :: ") + QFileInfo(save_filename_string).fileName());
		QWidget::setEnabled(false);
		SaveAsWavFileThread save_as_wav_file_thread(m_p_tune_manager, save_filename_string);
		save_as_wav_file_thread.start(QThread::HighPriority);

		QEventLoop loop;
		QObject::connect(&save_as_wav_file_thread, &QThread::finished, &loop, &QEventLoop::quit);
		do
		{
			loop.exec();
		} while(false == save_as_wav_file_thread.isFinished());

		ui->MessageLabel->setText(original_text_string);
		QWidget::setEnabled(true);
	} while(0);

	SetTuneStartTimeAndCheckPlayPausePushButtonIconToPlay(playing_value);
}

/**********************************************************************************/

void ChiptuneMidiWidget::on_StopPushButton_released(void)
{
	StopMidiFile();
	ui->TempoLabel->setText("");
}

/**********************************************************************************/

#define UNICODE_PLAY_ICON						u8"\u25B7"
#define UNICODE_PAUSE_ICON						u8"\u2016"

bool ChiptuneMidiWidget::IsPlayPauseButtonInPlayState(void)
{
	bool is_play_state = false;
	do
	{
		if(ui->PlayPausePushButton->text() == QString(UNICODE_PLAY_ICON)){
				break;
		}
		is_play_state = true;
	}while(0);
	return is_play_state;
}

/**********************************************************************************/

void ChiptuneMidiWidget::SetPlayPauseButtonInPlayState(bool is_play_state)
{
	do {
		if(true == is_play_state){
			ui->PlayPausePushButton->setText(UNICODE_PAUSE_ICON);
			break;
		}
		ui->PlayPausePushButton->setText(UNICODE_PLAY_ICON);
	} while(0);
}

/**********************************************************************************/

void ChiptuneMidiWidget::on_PlayPausePushButton_released(void)
{
	do {
		if(false == IsPlayPauseButtonInPlayState()){
			SetPlayPauseButtonInPlayState(true);
			m_p_audio_player->Play();
			break;
		}

		SetPlayPauseButtonInPlayState(false);
		m_p_audio_player->Pause();
	} while(0);

	QWidget::setFocus();
}

/**********************************************************************************/

void ChiptuneMidiWidget::on_AmplitudeGainSlider_sliderMoved(int value)
{
	m_p_tune_manager->SetAmplitudeGain(UINT16_MAX - value);
}

/**********************************************************************************/

void ChiptuneMidiWidget::on_AllOutputDisabledPushButton_released(void)
{
	do {
		if(nullptr == ui->TimbreListWidget->layout()){
			break;
		}
		ChannelListWidget *p_channellist_widget = (ChannelListWidget*)ui->TimbreListWidget->layout()->itemAt(0)->widget();
		p_channellist_widget->SetAllOutputEnabled(false);
	} while(0);

}

/**********************************************************************************/

void ChiptuneMidiWidget::on_AllOutputEnabledPushButton_released(void)
{
	do {
		if(nullptr == ui->TimbreListWidget->layout()){
			break;
		}
		ChannelListWidget *p_channellist_widget = (ChannelListWidget*)ui->TimbreListWidget->layout()->itemAt(0)->widget();
		p_channellist_widget->SetAllOutputEnabled(true);
	} while(0);
}

/**********************************************************************************/

void ChiptuneMidiWidget::on_PitchShiftSpinBox_valueChanged(int i)
{
	QLineEdit *p_lineEdit = ui->PitchShiftSpinBox->findChild<QLineEdit *>();
	if(i > 0){
		p_lineEdit->setText("+" + p_lineEdit->text());
	}

	m_p_tune_manager->SetPitchShift((int8_t)i);
}

/**********************************************************************************/

void ChiptuneMidiWidget::on_PlayingSpeedRatioComboBox_currentIndexChanged(int i)
{
	Q_UNUSED(i);
	m_p_tune_manager->SetPlayingSpeedRatio(ui->PlayingSpeedRatioComboBox->currentText().split("x").at(1).toDouble());
}
