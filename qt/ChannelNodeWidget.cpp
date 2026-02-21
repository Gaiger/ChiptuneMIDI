#include <QGridLayout>
#include <QLabel>
#include <QDebug>

#include "chiptune_midi_define.h"

#include "GetInstrumentNameString.h"
#include "MelodicTimbreFrame.h"

#include "ChannelNodeWidget.h"
#include "ui_ChannelNodeWidgetForm.h"

/**********************************************************************************/

ChannelNodeWidget::ChannelNodeWidget(int channel_index, int instrument_index, QWidget *parent) :
	QWidget(parent),
	m_channel_index(channel_index),
	m_p_melodic_timbre_frame(nullptr),
	ui(new Ui::ChannelNodeWidget)
{
	ui->setupUi(this);

	QColor color = GetChannelColor(channel_index);
	QString background_color_string = QString::asprintf("rgba(%d, %d, %d, %f%%)", color.red(), color.green(), color.blue(),
											 color.alpha() * 100/(double)UINT8_MAX);
	QString style_sheet_string = "QWidget { "
									  "border-width: 1px;"
									  "border-style: solid;"
									  "border-color: rgba(255, 255, 255, 75%);"
									  "background-color: " + background_color_string + ";"
									"}";

	ui->ColorWidget->setStyleSheet(style_sheet_string);
	m_expanded_size = QWidget::size();
	m_collapsed_size = QSize(m_expanded_size.width(), m_expanded_size.height() - ui->MelodicTimbreWidget->height());
	QWidget::setFixedSize(m_collapsed_size);

	QString instrument_name = QString("Unknown");
	do
	{
		if(MIDI_PERCUSSION_CHANNEL == channel_index){
			instrument_name = QString("Percussion");
			ui->ExpandCollapsePushButton->setEnabled(false);
			break;
		}

#define CHIPTUNE_INSTRUMENT_NOT_SPECIFIED			(-1)
#define CHIPTUNE_INSTRUMENT_UNUSED_CHANNEL			(-2)
		do
		{
			if(CHIPTUNE_INSTRUMENT_NOT_SPECIFIED == instrument_index){
				instrument_name = QString("Not Specified");
				break;
			}
			if(CHIPTUNE_INSTRUMENT_UNUSED_CHANNEL == instrument_index){
				instrument_name = QString("Unused channel");
				break;
			}

			instrument_name = GetInstrumentNameString(instrument_index);
		}while(0);

		m_p_melodic_timbre_frame = new MelodicTimbreFrame(this);
		QGridLayout *p_layout = new QGridLayout(ui->MelodicTimbreWidget);
		p_layout->addWidget(m_p_melodic_timbre_frame, 0, 0);
		p_layout->setContentsMargins(0, 0, 0, 0);
		p_layout->setSpacing(0);

		QObject::connect(m_p_melodic_timbre_frame, &MelodicTimbreFrame::TimbreChanged, this,
						 &ChannelNodeWidget::HandleMelodicTimbreFrameTimbreChanged);
	} while(0);

	QString string = "#"+ QString::number(channel_index) +" " + instrument_name;
	ui->ExpandCollapsePushButton->setStyleSheet("text-align:left;");
	ui->ExpandCollapsePushButton->setText(string);
}

/**********************************************************************************/


ChannelNodeWidget::~ChannelNodeWidget()
{
	delete ui;
}

/**********************************************************************************/

void ChannelNodeWidget::HandleMelodicTimbreFrameTimbreChanged(int waveform,
										   int envelope_attack_curve, double envelope_attack_duration_in_seconds,
										   int envelope_decay_curve, double envelope_decay_duration_in_seconds,
										   int envelope_note_on_sustain_level,
										   int envelope_release_curve, double envelope_release_duration_in_seconds,
										   int envelope_damper_sustain_level,
										   int envelope_damper_sustain_curve,
										   double envelope_damper_sustain_duration_in_seconds)
{
	emit MelodicChannelTimbreChanged(m_channel_index,
									 waveform,
									 envelope_attack_curve, envelope_attack_duration_in_seconds,
									 envelope_decay_curve, envelope_decay_duration_in_seconds,
									 envelope_note_on_sustain_level,
									 envelope_release_curve, envelope_release_duration_in_seconds,
									 envelope_damper_sustain_level,
									 envelope_damper_sustain_curve,
									 envelope_damper_sustain_duration_in_seconds);
}

/**********************************************************************************/

void ChannelNodeWidget::GetMelodicChannelTimbre(int *p_waveform,
			   int *p_envelope_attack_curve, double *p_envelope_attack_duration_in_seconds,
			   int *p_envelope_decay_curve, double *p_envelope_decay_duration_in_seconds,
			   int *p_envelope_note_on_sustain_level,
			   int *p_envelope_release_curve, double *p_envelope_release_duration_in_seconds,
			   int *p_envelope_damper_sustain_level,
			   int *p_envelope_damper_sustain_curve,
			   double *p_envelope_damper_sustain_duration_in_seconds)
{
	do
	{
		if(MIDI_PERCUSSION_CHANNEL == m_channel_index){
			qDebug() << Q_FUNC_INFO << "WARNING :: ignore for MIDI_PERCUSSION_CHANNEL";
			break;
		}
		m_p_melodic_timbre_frame->GetTimbre(p_waveform, p_envelope_attack_curve, p_envelope_attack_duration_in_seconds,
										 p_envelope_decay_curve, p_envelope_decay_duration_in_seconds,
										 p_envelope_note_on_sustain_level,
										 p_envelope_release_curve, p_envelope_release_duration_in_seconds,
										 p_envelope_damper_sustain_level,
										 p_envelope_damper_sustain_curve,
										 p_envelope_damper_sustain_duration_in_seconds);

	}while(0);
}

/**********************************************************************************/

void ChannelNodeWidget::setOutputEnabled(bool is_to_enabled)
{
	ui->OutputEnabledCheckBox->setChecked(is_to_enabled);
}

/**********************************************************************************/

void ChannelNodeWidget::on_OutputEnabledCheckBox_stateChanged(int state)
{
#if(0)
	QColor color = GetChannelColor(m_channel_index);
	QString style_sheet_string;
	do {
		if(true == (bool)state){
			QString background_color_string = QString::asprintf("rgba(%d, %d, %d, %f%%)", color.red(), color.green(), color.blue(),
													 color.alpha() * 100/(double)UINT8_MAX);
			style_sheet_string = "QWidget { "
										  "border-width: 1px;"
										  "border-style: solid;"
										  "border-color: rgba(255, 255, 255, 75%);"
										  "background-color: " + background_color_string + ";"
										"}";
			break;
		}

		color.setAlpha(0x30);
		//QPen pen(color.lighter(75));
		//pen.setWidth(4);
		//painter.setPen(pen);

		QString background_color_string = QString::asprintf("rgba(%d, %d, %d, %f%%)", color.red(), color.green(), color.blue(),
												 color.alpha() * 100/(double)UINT8_MAX);
		color = color.lighter(75);
		QString border_color_string = QString::asprintf("rgba(%d, %d, %d, %f%%)", color.red(), color.green(), color.blue(),
														color.alpha() * 100/(double)UINT8_MAX);

		style_sheet_string = "QWidget { "
								  "border-width: 4px;"
								  "border-style: solid;"
								  "border-color:" + border_color_string +";"
								  "background-color: " + background_color_string + ";"
								"}";
	}while(0);
	ui->ColorWidget->setStyleSheet(style_sheet_string);
#endif
	emit OutputEnabled(m_channel_index, (bool)state);
}

/**********************************************************************************/

void ChannelNodeWidget::on_ExpandCollapsePushButton_clicked(bool checked)
{
	do
	{
		if(true == checked){
			QWidget::setFixedSize(m_expanded_size);
			break;
		}
		QWidget::setFixedSize(m_collapsed_size);
	} while(0);
}
