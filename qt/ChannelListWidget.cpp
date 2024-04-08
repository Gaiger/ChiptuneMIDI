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
	ChannelNodeWidget *p_channelnote_widget = new ChannelNodeWidget(channel_index, instrument_index, this);
	m_p_vboxlayout->addWidget(p_channelnote_widget);
	QObject::connect(p_channelnote_widget, &ChannelNodeWidget::OutputEnabled, this,
					 &ChannelListWidget::OutputEnabled);
	QObject::connect(p_channelnote_widget, &ChannelNodeWidget::TimbreChanged, this,
					 &ChannelListWidget::TimbreChanged);
}

/**********************************************************************************/

void ChannelListWidget::SetAllOutputEnabled(bool is_enabled)
{
	for (int i = 0; i < m_p_vboxlayout->count(); ++i) {
		ChannelNodeWidget *p_channelnote_widget = (ChannelNodeWidget*)m_p_vboxlayout->itemAt(i)->widget();
		if(nullptr != p_channelnote_widget){
			p_channelnote_widget->setOutputEnabled(is_enabled);
		}
	}

}
