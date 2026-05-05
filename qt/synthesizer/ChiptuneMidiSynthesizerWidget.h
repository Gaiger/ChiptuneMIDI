#ifndef _CHIPTUNEMIDISYNTHESIZERWIDGET_H_
#define _CHIPTUNEMIDISYNTHESIZERWIDGET_H_

#include <QWidget>

namespace Ui {
class ChiptuneMidiSynthesizerWidget;
}

class ChiptuneMidiSynthesizerWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ChiptuneMidiSynthesizerWidget(QWidget *parent = nullptr);
	~ChiptuneMidiSynthesizerWidget() Q_DECL_OVERRIDE;
private:
	Ui::ChiptuneMidiSynthesizerWidget *ui;
};

#endif // _CHIPTUNEMIDISYNTHESIZERWIDGET_H_
