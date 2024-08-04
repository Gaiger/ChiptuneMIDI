#include "ChannelNodeWidget.h"

#include "ChannelListWidget.h"
#include "ui_ChannelListWidgetForm.h"

ChannelListWidget::ChannelListWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ChannelListWidget)
{
	ui->setupUi(this);

	QWidget *p_widget = new QWidget(this);
	m_p_vboxlayout = new QVBoxLayout;
	m_p_vboxlayout->setContentsMargins(0, 0, 0, 0);
	m_p_vboxlayout->setSpacing(2);

	QVBoxLayout *vBoxLayout = new QVBoxLayout(p_widget);
	vBoxLayout->setContentsMargins(0, 0, 0, 0);
	vBoxLayout->addLayout(m_p_vboxlayout);
	vBoxLayout->addStretch(1);

	ui->scrollArea->setWidget(p_widget);
}

/**********************************************************************************/

ChannelListWidget::~ChannelListWidget()
{
	delete ui;
}

/**********************************************************************************/

void ChannelListWidget::AddChannel(int channel_index, int instrument_index)
{
	ChannelNodeWidget *p_channelnode_widget = new ChannelNodeWidget(channel_index, instrument_index, this);
	m_p_vboxlayout->addWidget(p_channelnode_widget);
	QObject::connect(p_channelnode_widget, &ChannelNodeWidget::OutputEnabled, this,
					 &ChannelListWidget::OutputEnabled);
	QObject::connect(p_channelnode_widget, &ChannelNodeWidget::TimbreChanged, this,
					 &ChannelListWidget::TimbreChanged);
	m_channel_position_map.insert(channel_index, m_p_vboxlayout->count() - 1);
}

/**********************************************************************************/

void ChannelListWidget::SetAllOutputEnabled(bool is_enabled)
{
	for (int i = 0; i < m_p_vboxlayout->count(); i++) {
		ChannelNodeWidget *p_channelnode_widget = (ChannelNodeWidget*)m_p_vboxlayout->itemAt(i)->widget();
		if(nullptr != p_channelnode_widget){
			p_channelnode_widget->setOutputEnabled(is_enabled);
		}
	}

}

/**********************************************************************************/

void ChannelListWidget::GetTimbre(int channel_index, int *p_waveform,
			   int *p_envelope_attack_curve, double *p_envelope_attack_duration_in_seconds,
			   int *p_envelope_decay_curve, double *p_envelope_decay_duration_in_seconds,
			   int *p_envelope_sustain_level,
			   int *p_envelope_release_curve, double *p_envelope_release_duration_in_seconds,
			   int *p_envelope_damper_on_but_note_off_sustain_level,
			   int *p_envelope_damper_on_but_note_off_sustain_curve,
			   double *p_envelope_damper_on_but_note_off_sustain_duration_in_seconds)
{
	ChannelNodeWidget *p_channelnode_widget
			= (ChannelNodeWidget*)m_p_vboxlayout->itemAt(m_channel_position_map.value(channel_index))->widget();

	p_channelnode_widget->GetTimbre(p_waveform, p_envelope_attack_curve, p_envelope_attack_duration_in_seconds,
									p_envelope_decay_curve, p_envelope_decay_duration_in_seconds,
									p_envelope_sustain_level,
									p_envelope_release_curve, p_envelope_release_duration_in_seconds,
									p_envelope_damper_on_but_note_off_sustain_level,
									p_envelope_damper_on_but_note_off_sustain_curve,
									p_envelope_damper_on_but_note_off_sustain_duration_in_seconds);
}
