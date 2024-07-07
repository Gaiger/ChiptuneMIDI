#include <QDebug>

#include <QMouseEvent>
#include <QStyle>
#include "ProgressSlider.h"

ProgressSlider::ProgressSlider(QWidget *parent)
	: QSlider(parent),
	  m_is_position_changable(false)
{
	QSlider::installEventFilter(this);

}

/**********************************************************************************/

void ProgressSlider::SetPositionChangable(bool is_position_changabled)
{
	m_is_position_changable = is_position_changabled;
}

/**********************************************************************************/

void ProgressSlider::mousePressEvent(QMouseEvent *ev)
{
	int value = QStyle::sliderValueFromPosition(QSlider::minimum(), QSlider::maximum(), ev->x(), QWidget::width());
	emit MousePressed(ev->button(), value);
	QSlider::mousePressEvent(ev);
}

/**********************************************************************************/

bool ProgressSlider::eventFilter(QObject *obj, QEvent *event)
{
	if(false == m_is_position_changable){
		return true;
	}
	return QSlider::eventFilter(obj, event);
}
