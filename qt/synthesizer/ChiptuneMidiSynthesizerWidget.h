#ifndef _CHIPTUNEMIDISYNTHESIZERWIDGET_H_
#define _CHIPTUNEMIDISYNTHESIZERWIDGET_H_

#include <QWidget>

#include "TuneManager.h"
#include "AudioPlayer.h"

class WaveChartView;

namespace Ui {
class ChiptuneMidiSynthesizerWidget;
}

class ChiptuneMidiSynthesizerWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ChiptuneMidiSynthesizerWidget(TuneManager * p_tune_manager, QWidget *parent = nullptr);
	~ChiptuneMidiSynthesizerWidget() Q_DECL_OVERRIDE;
private slots:
	void on_NotePushButton_pressed(void);
	void on_NotePushButton_released(void);
private:
	TuneManager * const	m_p_tune_manager;
	AudioPlayer *		m_p_audio_player;
	WaveChartView *		m_p_wave_chartview;

	int					m_audio_player_buffer_in_milliseconds;
private:
	Ui::ChiptuneMidiSynthesizerWidget *ui;
};

#endif // _CHIPTUNEMIDISYNTHESIZERWIDGET_H_
