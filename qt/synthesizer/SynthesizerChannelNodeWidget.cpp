#include <QLabel>
#include <QDebug>
#include <QColor>
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

	do
	{
		if(MIDI_PERCUSSION_CHANNEL == channel_index){
			ui->ExpandCollapsePushButton->setEnabled(false);
			break;
		}

		ChannelNodeWidget::SetupMelodicTimbreWidget(ui->MelodicTimbreWidget);
	} while(0);

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
int SynthesizerChannelNodeWidget::GetDisplayedChannelIndex(void) const
{
	return m_channel_index + (true == m_is_displayed_channel_index_start_from_one ? 1 : 0);
}

/**********************************************************************************/
void SynthesizerChannelNodeWidget::SetTitleText(QString const &text)
{
	ui->ExpandCollapsePushButton->setText(text);
}

/**********************************************************************************/
void SynthesizerChannelNodeWidget::SetTitleStyleSheet(QString const &style_sheet)
{
	ui->ExpandCollapsePushButton->setStyleSheet(style_sheet);
}

/**********************************************************************************/
QString SynthesizerChannelNodeWidget::GetTitleStyleSheet(void) const
{
	return ui->ExpandCollapsePushButton->styleSheet();
}
