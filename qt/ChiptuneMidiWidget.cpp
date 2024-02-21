#include <QDebug>

#include <QGridLayout>
#include <QElapsedTimer>
#include <QDateTime>
#include <QTimerEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QFile>

#include "ui_ChiptuneMidiWidgetForm.h"
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
	p_tune_manager->InitializeTune();
	int data_buffer_size = p_tune_manager->GetNumberOfChannels()
			* p_tune_manager->GetSamplingRate() * p_tune_manager->GetSamplingSize()/8;
	QByteArray wave_data;
	while(1)
	{
		wave_data += p_tune_manager->FetchWave(data_buffer_size);
		if(true == p_tune_manager->IsTuneEnding()){
			break;
		}
	}
	qDebug() << "Generate wave data elpased" << elasped_timer.elapsed() << "ms";
	int header_size = 0;
	char *p_wav_header = (char*)wav_file_header(
				p_tune_manager->GetNumberOfChannels(), p_tune_manager->GetSamplingRate(),
				p_tune_manager->GetSamplingSize(), wave_data.size(), &header_size);

	QFile file(filename);
	if(false == file.open(QIODevice::ReadWrite | QIODevice::Truncate)){
		qDebug() << "open file fail";
		return -2;
	}

	int file_size = file.write(QByteArray(p_wav_header, header_size) + wave_data);
	file.close();

	qDebug() << "file saved, size = " << file_size << " bytes";
	return 0;
}

/**********************************************************************************/

void ReplaceWidget(QWidget *p_widget, QWidget *p_replaced_widget)
{
	QGridLayout *p_layout = new QGridLayout(p_replaced_widget);
	p_layout->addWidget(p_widget, 0, 0);
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->setSpacing(0);
}

/**********************************************************************************/

ChiptuneMidiWidget::ChiptuneMidiWidget(TuneManager *const p_tune_manager, QWidget *parent)
	: QWidget(parent),
	m_p_tune_manager(p_tune_manager),
	ui(new Ui::ChiptuneMidiWidget)
{
	ui->setupUi(this);

	QFont font20("Monospace");
	font20.setStyleHint(QFont::TypeWriter);
	font20.setPixelSize(20);
	ui->MessageLabel->setFont(font20);
	ui->ElapsedLabel->setFont(font20);
	do
	{
		m_p_wave_chartview = new WaveChartView(
					p_tune_manager->GetNumberOfChannels(),
					p_tune_manager->GetSamplingRate(), p_tune_manager->GetSamplingSize(), this);
		ReplaceWidget(m_p_wave_chartview, ui->WaveWidget);
	}while(0);

	QWidget::setAcceptDrops(true);

	m_p_tune_manager->moveToThread(&m_tune_manager_working_thread);
	m_tune_manager_working_thread.start(QThread::HighPriority);
	m_p_audio_player = new AudioPlayer(m_p_tune_manager, this);

	QObject::connect(p_tune_manager, &TuneManager::WaveFetched,
					 this, &ChiptuneMidiWidget::HandleWaveFetched, Qt::QueuedConnection);

	ui->OpenMidiFilePushButton->setToolTip(tr("Open MIDI File"));
	ui->SaveSaveFilePushButton->setToolTip(tr("Save as .wav file"));
}

/**********************************************************************************/

ChiptuneMidiWidget::~ChiptuneMidiWidget()
{
	m_tune_manager_working_thread.quit();
	while( false == m_tune_manager_working_thread.isFinished())
	{
		QThread::msleep(10);
	}

	if(nullptr != m_p_audio_player){
		m_p_audio_player->Stop();
	}
	delete m_p_audio_player; m_p_audio_player = nullptr;
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

int ChiptuneMidiWidget::PlayMidiFile(QString filename_string)
{
	m_p_audio_player->Stop();
	QObject::killTimer(m_inquiring_elapsed_time_timer);
	m_inquiring_elapsed_time_timer = -1;

	ui->MessageLabel->setText("");
	QThread::msleep(10);
	QString message_string;
	m_opened_file_info = QFileInfo(filename_string);
	int ret = 0;
	do
	{
		if(0 > m_p_tune_manager->SetMidiFile(filename_string)){
			message_string = QString::asprintf("Not a MIDI File");
			ui->MessageLabel->setText(message_string);
			ret = -1;
			break;
		}

		m_midi_file_duration_in_milliseconds = (int)(1000 * m_p_tune_manager->GetMidiFileDurationInSeconds());
		m_midi_file_duration_time_string = FormatTimeString(m_midi_file_duration_in_milliseconds);
		ui->ElapsedLabel->setText(FormatTimeString(0) + " / " + m_midi_file_duration_time_string);
		ui->ElapsedSlider->setRange(0, m_midi_file_duration_in_milliseconds);
		ui->ElapsedSlider->setValue(0);
		m_p_wave_chartview->Reset();
		m_p_audio_player->Play();
		ui->SaveSaveFilePushButton->setEnabled(true);
		message_string = QString::asprintf("Playing file");
		ui->MessageLabel->setText(message_string);
		m_inquiring_elapsed_time_timer = QObject::startTimer(500);
	}while(0);

	message_string += QString::asprintf(" :: <b>%s</b>", m_opened_file_info.fileName().toUtf8().data());
	ui->MessageLabel->setText(message_string);
	return ret;
}

/**********************************************************************************/

void ChiptuneMidiWidget::HandleWaveFetched(const QByteArray wave_bytearray)
{
	m_p_wave_chartview->GiveWave(wave_bytearray);
}

/**********************************************************************************/

void ChiptuneMidiWidget::timerEvent(QTimerEvent *event)
{
	QWidget::timerEvent(event);
	do
	{
		if(event->timerId() == m_inquiring_elapsed_time_timer){
			int elapsed_time_in_milliseconds =
					(int)(m_p_tune_manager->GetCurrentElapsedTimeInSeconds() * 1000);
			ui->ElapsedLabel->setText(FormatTimeString(elapsed_time_in_milliseconds) + " / "
									  + m_midi_file_duration_time_string);
			ui->ElapsedSlider->setValue(elapsed_time_in_milliseconds);
			break;
		}
	}while(0);
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
	do
	{
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
}

/**********************************************************************************/

void ChiptuneMidiWidget::on_OpenMidiFilePushButton_released(void)
{

	QString open_filename_string = QFileDialog::getOpenFileName(this, QString("Open the MIDI File"),
											   QString(),
											   QString("MIDI File (*.mid);;"
											   "All file (*)")
																);

	do
	{
		if(true == open_filename_string.isNull()){
			break;
		}
		PlayMidiFile(open_filename_string);
	}while(0);
}

/**********************************************************************************/

void ChiptuneMidiWidget::on_SaveSaveFilePushButton_released(void)
{
	QString suggested_filename_string = QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
	suggested_filename_string += m_opened_file_info.baseName();
	suggested_filename_string += ".wav";
	QString save_filename_string = QFileDialog::getSaveFileName(this, QString("Save the Wave File"),
											   suggested_filename_string,
											   QString("Wave File (*.wav);;"
											   "All file (*)")
																);

	do
	{
		if(true == save_filename_string.isNull()){
			break;
		}
		m_p_audio_player->Stop();
		SaveAsWavFile(m_p_tune_manager, save_filename_string);
	}while(0);
}
