#include <QGridLayout>
#include <QLabel>
#include <QDebug>

#include "GetInstrumentNameString.h"
#include "PitchTimbreFrame.h"

#include "ChannelNodeWidget.h"
#include "ui_ChannelNodeWidgetForm.h"

/**********************************************************************************/

ChannelNodeWidget::ChannelNodeWidget(int channel_index, int instrument_index, QWidget *parent) :
	QWidget(parent),
	m_channel_index(channel_index),
	ui(new Ui::ChannelNodeWidget)
{
	ui->setupUi(this);

	QColor color = GetChannelColor(channel_index);
	QString color_string = QString::asprintf("#%02x%02x%02x", color.red(), color.green(), color.blue() );
	QString background_color_string = "background-color: " + color_string + ";";
	ui->ColorWidget->setStyleSheet(background_color_string);
	m_expanded_size = QWidget::size();
	m_collapsed_size = QSize(m_expanded_size.width(), m_expanded_size.height() - ui->PitchTimbreWidget->height());
	QWidget::setFixedSize(m_collapsed_size);
	QString instrument_name = GetInstrumentNameString(instrument_index);
#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL			(9)
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == channel_index){
		instrument_name = QString("Percussion");
	}
	QString string = "#"+ QString::number(channel_index) +" " + instrument_name;
	ui->CollapsePushButton->setStyleSheet("text-align:left;");
	ui->CollapsePushButton->setText(string);
	PitchTimbreFrame *p_pitchtimbre_frame = new PitchTimbreFrame(channel_index, this);

	QGridLayout *p_layout = new QGridLayout(ui->PitchTimbreWidget);
	p_layout->addWidget(p_pitchtimbre_frame, 0, 0);
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->setSpacing(0);

	QObject::connect(p_pitchtimbre_frame, &PitchTimbreFrame::TimbreChanged, this,
					 &ChannelNodeWidget::TimbreChanged);
}

/**********************************************************************************/


ChannelNodeWidget::~ChannelNodeWidget()
{
	delete ui;
}

/**********************************************************************************/

void ChannelNodeWidget::setOutputEnabled(bool is_to_enabled)
{
	ui->OutputEnabledCheckBox->setChecked(is_to_enabled);
}

/**********************************************************************************/

void ChannelNodeWidget::on_OutputEnabledCheckBox_stateChanged(int state)
{
	emit OutputEnabled(m_channel_index, (bool)state);
}

/**********************************************************************************/

void ChannelNodeWidget::on_CollapsePushButton_clicked(bool checked)
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
