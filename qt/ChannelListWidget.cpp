#include "ChannelNodeWidget.h"

#include "ChannelListWidget.h"
#include "ui_ChannelListWidgetForm.h"

ChannelListWidget::ChannelListWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ChannelListWidget)
{
	ui->setupUi(this);

	QWidget *p_widget = new QWidget(this);
	ui->scrollArea->setWidget(p_widget);
	ui->scrollArea->QAbstractScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	m_p_vboxlayout = new QVBoxLayout();
	m_p_vboxlayout->setContentsMargins(0, 0, 0, 0);
	m_p_vboxlayout->setSpacing(2);

	QVBoxLayout *p_appending_spacer_vBoxLayout = new QVBoxLayout(p_widget);
	p_appending_spacer_vBoxLayout->setContentsMargins(0, 0, 0, 0);
	p_appending_spacer_vBoxLayout->addLayout(m_p_vboxlayout);
	p_appending_spacer_vBoxLayout->addStretch(1);
}

/**********************************************************************************/

ChannelListWidget::~ChannelListWidget()
{
	delete ui;
}

/**********************************************************************************/
void ChannelListWidget::AddChannelNode(int channel_index, ChannelNodeWidget * const p_channel_node_widget)
{
	m_p_vboxlayout->addWidget(p_channel_node_widget);
	QObject::connect(p_channel_node_widget, &ChannelNodeWidget::OutputEnabled, this,
					 &ChannelListWidget::OutputEnabled);
	QObject::connect(p_channel_node_widget, &ChannelNodeWidget::MelodicChannelInstrumentChanged, this,
					 &ChannelListWidget::MelodicChannelInstrumentChanged);
	QObject::connect(p_channel_node_widget, &ChannelNodeWidget::MelodicChannelTimbreChanged, this,
					 &ChannelListWidget::MelodicChannelTimbreChanged);
	m_channel_position_map.insert(channel_index, m_p_vboxlayout->count() - 1);
}

/**********************************************************************************/
void ChannelListWidget::SetMelodicChannelInstrument(int channel_index, int instrument_code)
{
	ChannelNodeWidget *p_channel_node_widget
			= (ChannelNodeWidget*)m_p_vboxlayout->itemAt(m_channel_position_map.value(channel_index))->widget();
	p_channel_node_widget->SetInstrument(instrument_code);
}

/**********************************************************************************/
void ChannelListWidget::SetChannelNodeIndicator(int channel_index, bool is_to_highlight)
{
	ChannelNodeWidget *p_channel_node_widget
			= (ChannelNodeWidget*)m_p_vboxlayout->itemAt(m_channel_position_map.value(channel_index))->widget();
	p_channel_node_widget->SetIndicator(is_to_highlight);
}

/**********************************************************************************/

void ChannelListWidget::SetAllOutputEnabled(bool is_enabled)
{
	for (int i = 0; i < m_p_vboxlayout->count(); i++) {
		ChannelNodeWidget *p_channelnode_widget = (ChannelNodeWidget*)m_p_vboxlayout->itemAt(i)->widget();
		if(nullptr != p_channelnode_widget){
			p_channelnode_widget->SetOutputEnabled(is_enabled);
		}
	}

}

/**********************************************************************************/

bool ChannelListWidget::IsOutputEnabled(int channel_index)
{
	ChannelNodeWidget *p_channel_node_widget
			= (ChannelNodeWidget*)m_p_vboxlayout->itemAt(m_channel_position_map.value(channel_index))->widget();
	return p_channel_node_widget->IsOutputEnabled();
}

/**********************************************************************************/

void ChannelListWidget::GetMelodicChannelTimbre(int channel_index, int *p_waveform,
			   int *p_envelope_attack_curve, double *p_envelope_attack_duration_in_seconds,
			   int *p_envelope_decay_curve, double *p_envelope_decay_duration_in_seconds,
			   int *p_envelope_note_on_sustain_level,
			   int *p_envelope_release_curve, double *p_envelope_release_duration_in_seconds,
			   int *p_envelope_note_off_hold_sustain_level,
			   int *p_envelope_note_off_hold_sustain_curve,
			   double *p_envelope_note_off_hold_sustain_duration_in_seconds)
{
	ChannelNodeWidget *p_channel_node_widget
			= (ChannelNodeWidget*)m_p_vboxlayout->itemAt(m_channel_position_map.value(channel_index))->widget();

	p_channel_node_widget->GetMelodicChannelTimbre(p_waveform, p_envelope_attack_curve, p_envelope_attack_duration_in_seconds,
									p_envelope_decay_curve, p_envelope_decay_duration_in_seconds,
									p_envelope_note_on_sustain_level,
									p_envelope_release_curve, p_envelope_release_duration_in_seconds,
									p_envelope_note_off_hold_sustain_level,
									p_envelope_note_off_hold_sustain_curve,
									p_envelope_note_off_hold_sustain_duration_in_seconds);
}

/**********************************************************************************/

void ChannelListWidget::SetMelodicChannelTimbre(int channel_index, int waveform,
			   int envelope_attack_curve, double envelope_attack_duration_in_seconds,
			   int envelope_decay_curve, double envelope_decay_duration_in_seconds,
			   int envelope_note_on_sustain_level,
			   int envelope_release_curve, double envelope_release_duration_in_seconds,
			   int envelope_note_off_hold_sustain_level,
			   int envelope_note_off_hold_sustain_curve,
			   double envelope_note_off_hold_sustain_duration_in_seconds,
			   bool is_to_darker_title_for_a_while)
{
	ChannelNodeWidget *p_channel_node_widget
			= (ChannelNodeWidget*)m_p_vboxlayout->itemAt(m_channel_position_map.value(channel_index))->widget();

	p_channel_node_widget->SetMelodicChannelTimbre(waveform,
									envelope_attack_curve, envelope_attack_duration_in_seconds,
									envelope_decay_curve, envelope_decay_duration_in_seconds,
									envelope_note_on_sustain_level,
									envelope_release_curve, envelope_release_duration_in_seconds,
									envelope_note_off_hold_sustain_level,
									envelope_note_off_hold_sustain_curve,
									envelope_note_off_hold_sustain_duration_in_seconds,
									is_to_darker_title_for_a_while);
}
