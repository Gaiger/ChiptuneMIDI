#include <QGridLayout>
#include <QLabel>
#include <QDebug>
#include <QColor>
#include <QRect>
#include <QTimer>

#include "chiptune_midi_define.h"

#include "ChiptuneMidiValues.h"
#include "MelodicTimbreFrame.h"

#include "ChannelNodeWidget.h"
#include "ui_ChannelNodeWidgetForm.h"


/**********************************************************************************/
ChannelNodeWidget::ChannelNodeWidget(int channel_index, int instrument_code,
									 bool is_displayed_channel_index_start_from_one,
									 bool is_channel_indicator_enabled, QWidget *parent) :
	QWidget(parent),
	m_channel_index(channel_index),
	m_instrument_code(instrument_code),
	m_is_displayed_channel_index_start_from_one(is_displayed_channel_index_start_from_one),
	m_p_melodic_timbre_frame(nullptr),
	m_p_channel_indicator_widget(nullptr),
	ui(new Ui::ChannelNodeWidget)
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

	if(true == is_channel_indicator_enabled){
		SetupChannelIndicatorLayout();
	}

	m_expanded_size = QWidget::size();
	m_collapsed_size = QSize(m_expanded_size.width(), m_expanded_size.height() - ui->MelodicTimbreWidget->height());
	QWidget::setFixedSize(m_collapsed_size);

	do
	{
		if(MIDI_PERCUSSION_CHANNEL == channel_index){
			ui->ExpandCollapsePushButton->setEnabled(false);
			break;
		}

		m_p_melodic_timbre_frame = new MelodicTimbreFrame(this);
		QGridLayout *p_layout = new QGridLayout(ui->MelodicTimbreWidget);
		p_layout->addWidget(m_p_melodic_timbre_frame, 0, 0);
		p_layout->setContentsMargins(0, 0, 0, 0);
		p_layout->setSpacing(0);

		QObject::connect(m_p_melodic_timbre_frame, &MelodicTimbreFrame::TimbreChanged, this,
						 &ChannelNodeWidget::HandleMelodicTimbreFrameTimbreChanged);
	} while(0);

	ui->ExpandCollapsePushButton->setStyleSheet("text-align:left;");
	m_expand_collapse_push_button_original_style_sheet = ui->ExpandCollapsePushButton->styleSheet();
	SetInstrument(instrument_code);
}

/**********************************************************************************/


ChannelNodeWidget::~ChannelNodeWidget()
{
	delete ui;
}

/**********************************************************************************/
void ChannelNodeWidget::SetupChannelIndicatorLayout(void)
{
	ui->OutputEnabledCheckBox->setVisible(false);

	QRect const original_output_enabled_check_box_geometry = ui->OutputEnabledCheckBox->geometry();

	QRect const original_channel_color_widget_geometry = ui->ChannelColorWidget->geometry();
	QRect const original_expand_collapse_push_button_geometry = ui->ExpandCollapsePushButton->geometry();
#define CHANNEL_INDICATOR_WIDGET_SPACING				(4)

	ui->ChannelColorWidget->setGeometry(original_output_enabled_check_box_geometry.x(),
										original_output_enabled_check_box_geometry.y(),
										original_channel_color_widget_geometry.width(),
										original_channel_color_widget_geometry.height());

	m_p_channel_indicator_widget = new QWidget(this);
	m_p_channel_indicator_widget->setObjectName(QStringLiteral("ChannelIndicatorWidget"));
	int const channel_indicator_widget_x = original_output_enabled_check_box_geometry.x()
			+ original_channel_color_widget_geometry.width() + CHANNEL_INDICATOR_WIDGET_SPACING;
	m_p_channel_indicator_widget->setGeometry(channel_indicator_widget_x,
											 original_output_enabled_check_box_geometry.y(),
											 original_channel_color_widget_geometry.width(),
											 original_channel_color_widget_geometry.height());
	{
		int const changed_expand_collapse_push_button_x = channel_indicator_widget_x
				+ original_channel_color_widget_geometry.width() + CHANNEL_INDICATOR_WIDGET_SPACING;

		int const changed_expand_collapse_push_button_width =
				original_expand_collapse_push_button_geometry.right() - changed_expand_collapse_push_button_x + 1;
		ui->ExpandCollapsePushButton->setGeometry(changed_expand_collapse_push_button_x,
												  original_expand_collapse_push_button_geometry.y(),
												  changed_expand_collapse_push_button_width,
												  original_expand_collapse_push_button_geometry.height());
	}

	m_channel_indicator_widget_plain_style_sheet = m_p_channel_indicator_widget->styleSheet();
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

/**********************************************************************************/
void ChannelNodeWidget::SetInstrument(int instrument_code)
{
	m_instrument_code = instrument_code;

	QString instrument_name = QString("Unknown");
	do
	{
		if(MIDI_PERCUSSION_CHANNEL == m_channel_index){
			instrument_name = QString("Percussion");
			break;
		}
		instrument_name = GetInstrumentNameString(instrument_code);
	} while(0);

	int const displayed_channel_index = m_channel_index
			+ (true == m_is_displayed_channel_index_start_from_one ? 1 : 0);
	ui->ExpandCollapsePushButton->setText("#"+ QString::number(displayed_channel_index) +" " + instrument_name);
}

/**********************************************************************************/
int ChannelNodeWidget::GetInstrument(void)
{
	return m_instrument_code;
}

/**********************************************************************************/
void ChannelNodeWidget::SetIndicator(bool is_to_highlight)
{
	do
	{
		if(nullptr == m_p_channel_indicator_widget){
			break;
		}

		if(true == is_to_highlight){
			m_p_channel_indicator_widget->setStyleSheet(m_channel_indicator_widget_highlight_style_sheet);
			break;
		}
		m_p_channel_indicator_widget->setStyleSheet(m_channel_indicator_widget_plain_style_sheet);
	} while(0);
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

void ChannelNodeWidget::SetMelodicChannelTimbre(int waveform,
			   int envelope_attack_curve, double envelope_attack_duration_in_seconds,
			   int envelope_decay_curve, double envelope_decay_duration_in_seconds,
			   int envelope_note_on_sustain_level,
			   int envelope_release_curve, double envelope_release_duration_in_seconds,
			   int envelope_damper_sustain_level,
			   int envelope_damper_sustain_curve,
			   double envelope_damper_sustain_duration_in_seconds,
			   bool is_to_darker_title_for_a_while)
{
	do
	{
		if(MIDI_PERCUSSION_CHANNEL == m_channel_index){
			qDebug() << Q_FUNC_INFO << "WARNING :: ignore for MIDI_PERCUSSION_CHANNEL";
			break;
		}
		m_p_melodic_timbre_frame->SetTimbre(waveform,
											envelope_attack_curve, envelope_attack_duration_in_seconds,
											envelope_decay_curve, envelope_decay_duration_in_seconds,
											envelope_note_on_sustain_level,
											envelope_release_curve, envelope_release_duration_in_seconds,
											envelope_damper_sustain_level,
											envelope_damper_sustain_curve,
											envelope_damper_sustain_duration_in_seconds);

		if(false == is_to_darker_title_for_a_while){
			break;
		}

		QColor const dark24_color = QColor(24, 24, 24);
		ui->ExpandCollapsePushButton->setStyleSheet(m_expand_collapse_push_button_original_style_sheet
				+ QString::asprintf("background-color: rgb(%d, %d, %d);",
									dark24_color.red(),
									dark24_color.green(),
									dark24_color.blue()));
		QTimer::singleShot(5000, this, [this]() {
			ui->ExpandCollapsePushButton->setStyleSheet(m_expand_collapse_push_button_original_style_sheet);
		});
	}while(0);
}

/**********************************************************************************/

void ChannelNodeWidget::SetOutputEnabled(bool is_to_enabled)
{
	ui->OutputEnabledCheckBox->setChecked(is_to_enabled);
}

/**********************************************************************************/

bool ChannelNodeWidget::IsOutputEnabled(void)
{
	return ui->OutputEnabledCheckBox->isChecked();
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
	ui->ChannelColorWidget->setStyleSheet(style_sheet_string);
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
