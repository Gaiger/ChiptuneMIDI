#ifndef PROGRESSSLIDER_H
#define PROGRESSSLIDER_H

#include <QSlider>
#include <QObject>

class ProgressSlider : public QSlider
{
	Q_OBJECT
public:
	ProgressSlider(QWidget *parent = nullptr);
	void SetPositionChangable(bool is_position_changabled);
public:
	signals:
	void PositionChanged(int value);
	void MouseRightReleased(QPoint position);
protected:
	virtual void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
	virtual void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
	virtual bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;
private:
	bool m_is_position_changable;
};

#endif // PROGRESSSLIDER_H
