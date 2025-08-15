#include <QDebug>

#include <QMouseEvent>
#include <QStyle>
#include "ProgressSlider.h"

ProgressSlider::ProgressSlider(QWidget *parent): QSlider(parent){}

/**********************************************************************************/

void ProgressSlider::mousePressEvent(QMouseEvent *ev)
{
	QSlider::mousePressEvent(ev);

	int position;
#if QT_VERSION_CHECK(6, 0, 0) > QT_VERSION
	position = ev->x();
#endif
#if QT_VERSION_CHECK(6, 0, 0) <= QT_VERSION
	position = ev->position().x();
#endif
	int value = QStyle::sliderValueFromPosition(QSlider::minimum(), QSlider::maximum(), position, QWidget::width());
	emit MousePressed(ev->button(), value);
}
