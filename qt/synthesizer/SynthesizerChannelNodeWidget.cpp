#include <QLabel>
#include <QComboBox>
#include <QDebug>
#include <QColor>
#include <QSignalBlocker>
#include <QTimer>

#include "chiptune_midi_define.h"

#include "ChiptuneMidiValues.h"

#include "SynthesizerChannelNodeWidget.h"
#include "ui_SynthesizerChannelNodeWidgetForm.h"


/**********************************************************************************/
SynthesizerChannelNodeWidget::SynthesizerChannelNodeWidget(int const channel_index, int const instrument_code,
									 bool const is_displayed_channel_index_start_from_one,
									 QWidget *parent) :
	ChannelNodeWidget(channel_index, parent),
	m_is_displayed_channel_index_start_from_one(is_displayed_channel_index_start_from_one),
	m_p_percussion_name_label(nullptr),
	ui(new Ui::SynthesizerChannelNodeWidget)
{
	ui->setupUi(this);

	{
		QColor channel_color = GetChannelColor(channel_index);
		QString const background_color_string
				= QString::asprintf("rgba(%d, %d, %d, %f%%)",
									channel_color.red(), channel_color.green(), channel_color.blue(),
									channel_color.alpha() * 100/(double)UINT8_MAX);
		QString const style_sheet_string = "QWidget { "
										  "border-width: 1px;"
										  "border-style: solid;"
										  "border-color: rgba(255, 255, 255, 75%);"
										  "background-color: " + background_color_string + ";"
										"}";
		ui->ChannelColorWidget->setStyleSheet(style_sheet_string);
	}

	m_channel_indicator_widget_plain_style_sheet = ui->ChannelIndicatorWidget->styleSheet();
	{
		QColor const indicator_widget_color(0xA0, 0xA0, 0xA0);
		QString const indicator_widget_background_color_string
				= QString::asprintf("rgba(%d, %d, %d, %f%%)",
						indicator_widget_color.red(), indicator_widget_color.green(), indicator_widget_color.blue(),
						indicator_widget_color.alpha() * 100/(double)UINT8_MAX);
		m_channel_indicator_widget_highlight_style_sheet =
				"QWidget { "
				  "border-width: 0px;"
				  "background-color: " + indicator_widget_background_color_string + ";"
				"}";
	}

	ui->ChannelIndexLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	for(int instrument_code = 0; instrument_code <= MIDI_SEVEN_BITS_MAX_VALUE; instrument_code++){
		ui->InstrumentNameComboBox->addItem(GetInstrumentNameString(instrument_code), instrument_code);
	}

	if(MIDI_PERCUSSION_CHANNEL == channel_index){
		ui->ExpandCollapsePushButton->setEnabled(false);
		m_p_percussion_name_label = new QLabel(this);
		m_p_percussion_name_label->setGeometry(ui->InstrumentNameComboBox->geometry());
		m_p_percussion_name_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		m_p_percussion_name_label->hide();
	}
	ChannelNodeWidget::SetupMelodicTimbreWidget(ui->MelodicTimbreWidget);
	ChannelNodeWidget::SetupTitleWidget();
	ChannelNodeWidget::SetInstrument(instrument_code);
}

/**********************************************************************************/
SynthesizerChannelNodeWidget::~SynthesizerChannelNodeWidget()
{
	delete ui;
}

/**********************************************************************************/
void SynthesizerChannelNodeWidget::SetIndicator(bool const is_to_highlight)
{
	do
	{
		if(true == is_to_highlight){
			ui->ChannelIndicatorWidget->setStyleSheet(m_channel_indicator_widget_highlight_style_sheet);
			break;
		}
		ui->ChannelIndicatorWidget->setStyleSheet(m_channel_indicator_widget_plain_style_sheet);
	} while(0);
}

/**********************************************************************************/
void SynthesizerChannelNodeWidget::on_InstrumentNameComboBox_currentIndexChanged(int const index)
{
	do
	{
		if(index < 0){
			break;
		}
		emit MelodicChannelInstrumentChanged(m_channel_index, index);
	} while(0);
}

/**********************************************************************************/
int SynthesizerChannelNodeWidget::GetDisplayedChannelIndex(void) const
{
	return m_channel_index + (true == m_is_displayed_channel_index_start_from_one ? 1 : 0);
}

/**********************************************************************************/
void SynthesizerChannelNodeWidget::SetTitleText(QString const &text)
{
	int const title_separator_index = text.indexOf(' ');
	QString instrument_name = text;
	do
	{
		if(title_separator_index < 0){
			ui->ChannelIndexLabel->setText(QString());
			break;
		}
		ui->ChannelIndexLabel->setText(text.left(title_separator_index));
		instrument_name = text.mid(title_separator_index + 1);
	} while(0);

	int const instrument_name_combo_box_index = ui->InstrumentNameComboBox->findText(instrument_name);
	do
	{
		if(0 <= instrument_name_combo_box_index){
			ui->InstrumentNameComboBox->show();
			QSignalBlocker signal_blocker(ui->InstrumentNameComboBox);
			ui->InstrumentNameComboBox->setCurrentIndex(instrument_name_combo_box_index);
			break;
		}
		if(nullptr == m_p_percussion_name_label){
			break;
		}
		ui->InstrumentNameComboBox->hide();
		m_p_percussion_name_label->show();
		m_p_percussion_name_label->setText(instrument_name);
	} while(0);
}

/**********************************************************************************/
void SynthesizerChannelNodeWidget::SetTitleStyleSheet(QString const &style_sheet)
{
	QString const title_style_sheet = "color: rgb(208, 208, 208);" + style_sheet;
	ui->ChannelIndexLabel->setStyleSheet(title_style_sheet);
	ui->InstrumentNameComboBox->setStyleSheet(title_style_sheet);
	if(nullptr != m_p_percussion_name_label){
		m_p_percussion_name_label->setStyleSheet(title_style_sheet);
	}
}

/**********************************************************************************/
QString SynthesizerChannelNodeWidget::GetTitleStyleSheet(void) const
{
	return ui->InstrumentNameComboBox->styleSheet();
}
