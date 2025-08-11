#ifndef _PROGRESSSLIDER_H_
#define _PROGRESSSLIDER_H_

#include <QSlider>
#include <QObject>

class ProgressSlider : public QSlider
{
	Q_OBJECT
public:
	ProgressSlider(QWidget *parent = nullptr);
public:
	signals:
	void MousePressed(Qt::MouseButton button, int value);
protected:
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
};

#endif // _PROGRESSSLIDER_H_
