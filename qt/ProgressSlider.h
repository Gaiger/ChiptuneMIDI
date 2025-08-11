#ifndef PROGRESSSLIDER_H
#define PROGRESSSLIDER_H

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

#endif // PROGRESSSLIDER_H
