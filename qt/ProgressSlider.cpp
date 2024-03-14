#include <QDebug>

#include <QMouseEvent>
#include <QStyle>
#include "ProgressSlider.h"

ProgressSlider::ProgressSlider(QWidget *parent)
	: QSlider(parent)
{

}

/**********************************************************************************/

void ProgressSlider::mousePressEvent(QMouseEvent *ev)
{
	int value = QStyle::sliderValueFromPosition(QSlider::minimum(), QSlider::maximum(), ev->x(), QWidget::width());
	emit MousePressed(ev->button(), value);
	QSlider::mousePressEvent(ev);
}
