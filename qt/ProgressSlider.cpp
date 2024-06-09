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
	QSlider::mousePressEvent(ev);
	if(Qt::LeftButton != ev->button()){
		return;
	}

	int value = QStyle::sliderValueFromPosition(QSlider::minimum(), QSlider::maximum(), ev->x(), QWidget::width());
	emit PositionChanged(value);
}

/**********************************************************************************/

void ProgressSlider::mouseReleaseEvent(QMouseEvent *ev)
{
	QSlider::mousePressEvent(ev);
	int value = QStyle::sliderValueFromPosition(QSlider::minimum(), QSlider::maximum(), ev->x(), QWidget::width());
	emit MouseRightReleased(ev->pos());
}

/**********************************************************************************/

bool ProgressSlider::eventFilter(QObject *obj, QEvent *event)
{
	do {
		if(true == m_is_position_changable){
			return QSlider::eventFilter(obj, event);
		}

		if(event->type() != QEvent::MouseButtonRelease){
			return true;
		}

		if( Qt::RightButton != ((QMouseEvent*)event)->button() ){
			return true;
		}
	} while(0);

	return QSlider::eventFilter(obj, event);;
}
