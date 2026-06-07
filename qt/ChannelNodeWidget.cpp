#include "ChannelNodeWidget.h"

#include <QColor>
#include <QDebug>
#include <QGridLayout>
#include <QTimer>
#include <functional>

#include "chiptune_midi_define.h"
#include "ChiptuneMidiValues.h"
#include "MelodicTimbreFrame.h"

/**********************************************************************************/
ChannelNodeWidget::ChannelNodeWidget(int const channel_index, QWidget *parent) :
	QWidget(parent),
	m_channel_index(channel_index),
	m_instrument_code(0),
	m_p_melodic_timbre_frame(nullptr)
{
}

/**********************************************************************************/
void ChannelNodeWidget::SetIndicator(bool const is_to_highlight){ (void)is_to_highlight;}

/**********************************************************************************/
void ChannelNodeWidget::SetOutputEnabled(bool const is_to_enabled){ (void)is_to_enabled;}

/**********************************************************************************/
bool ChannelNodeWidget::IsOutputEnabled(void) const
{
	return true;
}

/**********************************************************************************/
void ChannelNodeWidget::SetInstrument(int const instrument_code)
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

	SetTitleText("#"+ QString::number(GetDisplayedChannelIndex()) +" " + instrument_name);
}

/**********************************************************************************/
int ChannelNodeWidget::GetDisplayedChannelIndex(void) const
{
	return m_channel_index;
}

/**********************************************************************************/
void ChannelNodeWidget::SetupMelodicTimbreWidget(QWidget * const p_melodic_timbre_widget)
{
	m_expanded_size = QWidget::size();
	m_collapsed_size = QSize(m_expanded_size.width(), m_expanded_size.height() - p_melodic_timbre_widget->height());
	QWidget::setFixedSize(m_collapsed_size);

	m_p_melodic_timbre_frame = new MelodicTimbreFrame(this);
	QGridLayout *p_layout = new QGridLayout(p_melodic_timbre_widget);
	p_layout->addWidget(m_p_melodic_timbre_frame, 0, 0);
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->setSpacing(0);

	QObject::connect(m_p_melodic_timbre_frame, &MelodicTimbreFrame::TimbreChanged, this,
					 &ChannelNodeWidget::HandleMelodicTimbreFrameTimbreChanged);
}

/**********************************************************************************/
void ChannelNodeWidget::SetupTitleWidget(void)
{
	SetTitleStyleSheet("text-align:left;");
	m_title_widget_original_style_sheet = GetTitleStyleSheet();
}

/**********************************************************************************/
void ChannelNodeWidget::GetMelodicChannelTimbre(int * const p_waveform,
			   int * const p_envelope_attack_curve, double * const p_envelope_attack_duration_in_seconds,
			   int * const p_envelope_decay_curve, double * const p_envelope_decay_duration_in_seconds,
			   int * const p_envelope_note_on_sustain_level,
			   int * const p_envelope_release_curve, double * const p_envelope_release_duration_in_seconds,
			   int * const p_envelope_note_off_hold_sustain_level,
			   int * const p_envelope_note_off_hold_sustain_curve,
			   double * const p_envelope_note_off_hold_sustain_duration_in_seconds)
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
										 p_envelope_note_off_hold_sustain_level,
										 p_envelope_note_off_hold_sustain_curve,
										 p_envelope_note_off_hold_sustain_duration_in_seconds);

	}while(0);
}

/**********************************************************************************/
void ChannelNodeWidget::SetMelodicChannelTimbre(int const waveform,
			   int const envelope_attack_curve, double const envelope_attack_duration_in_seconds,
			   int const envelope_decay_curve, double const envelope_decay_duration_in_seconds,
			   int const envelope_note_on_sustain_level,
			   int const envelope_release_curve, double const envelope_release_duration_in_seconds,
			   int const envelope_note_off_hold_sustain_level,
			   int const envelope_note_off_hold_sustain_curve,
			   double const envelope_note_off_hold_sustain_duration_in_seconds,
			   bool const is_to_darker_title_for_a_while)
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
											envelope_note_off_hold_sustain_level,
											envelope_note_off_hold_sustain_curve,
											envelope_note_off_hold_sustain_duration_in_seconds);

		if(false == is_to_darker_title_for_a_while){
			break;
		}

		QColor const dark24_color = QColor(24, 24, 24);
		SetTitleStyleSheet(m_title_widget_original_style_sheet
				+ QString::asprintf("background-color: rgb(%d, %d, %d);",
									dark24_color.red(),
									dark24_color.green(),
									dark24_color.blue()));
		std::function<void()> recover_background_color_function = [this]() {
			SetTitleStyleSheet(m_title_widget_original_style_sheet);
		};
		QTimer::singleShot(5000, this, recover_background_color_function);
	}while(0);
}

/**********************************************************************************/
void ChannelNodeWidget::HandleMelodicTimbreFrameTimbreChanged(int const waveform,
										   int const envelope_attack_curve, double const envelope_attack_duration_in_seconds,
										   int const envelope_decay_curve, double const envelope_decay_duration_in_seconds,
										   int const envelope_note_on_sustain_level,
										   int const envelope_release_curve, double const envelope_release_duration_in_seconds,
										   int const envelope_note_off_hold_sustain_level,
										   int const envelope_note_off_hold_sustain_curve,
										   double const envelope_note_off_hold_sustain_duration_in_seconds)
{
	emit MelodicChannelTimbreChanged(m_channel_index,
									 waveform,
									 envelope_attack_curve, envelope_attack_duration_in_seconds,
									 envelope_decay_curve, envelope_decay_duration_in_seconds,
									 envelope_note_on_sustain_level,
									 envelope_release_curve, envelope_release_duration_in_seconds,
									 envelope_note_off_hold_sustain_level,
									 envelope_note_off_hold_sustain_curve,
									 envelope_note_off_hold_sustain_duration_in_seconds);
}

/**********************************************************************************/
void ChannelNodeWidget::on_ExpandCollapsePushButton_clicked(bool const checked)
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
