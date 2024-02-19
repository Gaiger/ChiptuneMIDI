#ifndef CHIPTUNEMIDIWIDGET_H
#define CHIPTUNEMIDIWIDGET_H
#include <QThread>
#include <QWidget>

#include "TuneManager.h"
#include "AudioPlayer.h"

namespace Ui {
class ChiptuneMidiWidget;
}


class ChiptuneMidiWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ChiptuneMidiWidget(TuneManager *const p_tune_manager, QWidget *parent = nullptr);
	~ChiptuneMidiWidget()  Q_DECL_OVERRIDE;
signals:

private :
	TuneManager *		m_p_tune_manager;
	QThread				m_tune_manager_working_thread;

	AudioPlayer *		m_p_audio_player;
private:
	Ui::ChiptuneMidiWidget *ui;
};

#endif // CHIPTUNEMIDIWIDGET_H
