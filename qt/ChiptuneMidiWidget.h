#ifndef CHIPTUNEMIDIWIDGET_H
#define CHIPTUNEMIDIWIDGET_H
#include <QTimer>
#include <QThread>
#include <QWidget>
#include <QFileInfo>

#include "TuneManager.h"
#include "AudioPlayer.h"

#include "WaveChartView.h"

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
	virtual void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
	virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

	virtual void dragEnterEvent(QDragEnterEvent* event) Q_DECL_OVERRIDE;
	virtual void dragMoveEvent(QDragMoveEvent* event) Q_DECL_OVERRIDE;
	virtual void dragLeaveEvent(QDragLeaveEvent* event) Q_DECL_OVERRIDE;
	virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

	virtual void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
private slots:
	void on_OpenMidiFilePushButton_released(void);
	void on_SaveSaveFilePushButton_released(void);

private slots:
	void HandleWaveFetched(const QByteArray wave_bytearray);
	void HandlePlayPositionSliderMoved(int value);
	void HandlePlayPositionSliderMousePressed(Qt::MouseButton button, int value);

private:
	int PlayMidiFile(QString filename_string);
	void PlayTune(int start_time_in_milliseconds);
private:
	WaveChartView *		m_p_wave_chartview;

private :
	TuneManager *		m_p_tune_manager;
	QThread				m_tune_manager_working_thread;

	AudioPlayer *		m_p_audio_player;
	QFileInfo			m_opened_file_info;
	uint32_t			m_midi_file_duration_in_milliseconds;
	int					m_inquiring_elapsed_time_timer;
	QString				m_midi_file_duration_time_string;

	QTimer				m_set_start_time_postpone_timer;
private:
	Ui::ChiptuneMidiWidget *ui;
};

#endif // CHIPTUNEMIDIWIDGET_H
