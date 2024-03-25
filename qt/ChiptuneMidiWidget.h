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

	void on_StopPushButton_released(void);
	void on_PlayPausePushButton_released(void);
	void on_AmplitudeGainSlider_sliderMoved(int value);
private slots:
	void HandleWaveFetched(const QByteArray wave_bytearray);

	void on_PlayProgressSlider_sliderMoved(int value);
	void HandlePlayProgressSliderMousePressed(Qt::MouseButton button, int value);

private:
	int PlayMidiFile(QString filename_string);
	void SetTuneStartTimeAndCheckPlayPausePushButtonIconToPlay(int start_time_in_milliseconds);
private:
	bool IsPlayPausePushButtonPlayIcon(void);
	void SetPlayPausePushButtonAsPlayIcon(bool is_play_icon);
private:
	WaveChartView *		m_p_wave_chartview;

private :
	TuneManager *		m_p_tune_manager;
	QThread				m_tune_manager_working_thread;

	AudioPlayer *		m_p_audio_player;
	QFileInfo			m_opened_file_info;
	uint32_t			m_midi_file_duration_in_milliseconds;
	int					m_inquiring_play_progress_timer_id;
	QString				m_midi_file_duration_time_string;

	QTimer				m_set_start_time_postpone_timer;
private:
	Ui::ChiptuneMidiWidget *ui;
};

#endif // CHIPTUNEMIDIWIDGET_H
