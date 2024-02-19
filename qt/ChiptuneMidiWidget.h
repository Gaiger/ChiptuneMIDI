#ifndef CHIPTUNEMIDIWIDGET_H
#define CHIPTUNEMIDIWIDGET_H
#include <QThread>
#include <QWidget>
#include <QFileInfo>

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

private:
	virtual void dragEnterEvent(QDragEnterEvent* event) Q_DECL_OVERRIDE;
	virtual void dragMoveEvent(QDragMoveEvent* event) Q_DECL_OVERRIDE;
	virtual void dragLeaveEvent(QDragLeaveEvent* event) Q_DECL_OVERRIDE;
	virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
private slots:
	void on_OpenMidiFilePushButton_released(void);
	void on_SaveSaveFilePushButton_released(void);

private:
	void PlayMidiFile(QString midi_filename_string);

private :
	TuneManager *		m_p_tune_manager;
	QThread				m_tune_manager_working_thread;

	AudioPlayer *		m_p_audio_player;

	QFileInfo				m_opened_file_info;
private:
	Ui::ChiptuneMidiWidget *ui;
};

#endif // CHIPTUNEMIDIWIDGET_H
