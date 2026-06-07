#include <QLabel>
#include <QDebug>
#include <QColor>
#include <QRect>
#include <QTimer>

#include "chiptune_midi_define.h"

#include "ChiptuneMidiValues.h"

#include "PlayerChannelNodeWidget.h"
#include "ui_PlayerChannelNodeWidgetForm.h"


/**********************************************************************************/
PlayerChannelNodeWidget::PlayerChannelNodeWidget(int const channel_index, int const instrument_code, QWidget *parent) :
	ChannelNodeWidget(channel_index, parent),
	ui(new Ui::PlayerChannelNodeWidget)
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
PlayerChannelNodeWidget::~PlayerChannelNodeWidget()
{
	delete ui;
}

/**********************************************************************************/
void PlayerChannelNodeWidget::SetOutputEnabled(bool const is_to_enabled)
{
	ui->OutputEnabledCheckBox->setChecked(is_to_enabled);
}

/**********************************************************************************/
bool PlayerChannelNodeWidget::IsOutputEnabled(void) const
{
	return ui->OutputEnabledCheckBox->isChecked();
}

/**********************************************************************************/
void PlayerChannelNodeWidget::on_OutputEnabledCheckBox_stateChanged(int const state)
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
	ui->ChannelColorWidget->setStyleSheet(style_sheet_string);
#endif
	emit OutputEnabled(m_channel_index, (bool)state);
}

/**********************************************************************************/
void PlayerChannelNodeWidget::SetTitleText(QString const &text)
{
	ui->ExpandCollapsePushButton->setText(text);
}

/**********************************************************************************/
void PlayerChannelNodeWidget::SetTitleStyleSheet(QString const &style_sheet)
{
	ui->ExpandCollapsePushButton->setStyleSheet(style_sheet);
}

/**********************************************************************************/
QString PlayerChannelNodeWidget::GetTitleStyleSheet(void) const
{
	return ui->ExpandCollapsePushButton->styleSheet();
}
