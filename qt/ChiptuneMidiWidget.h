#ifndef _CHIPTUNEMIDIWIDGET_H_
#define _CHIPTUNEMIDIWIDGET_H_
#include <QTimer>
#include <QWidget>
#include <QToolBox>
#include <QFileInfo>

#include "TuneManager.h"
#include "AudioPlayer.h"


namespace Ui {
class ChiptuneMidiWidget;
}

class WaveChartView;
class SequencerWidget;

class ChiptuneMidiWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ChiptuneMidiWidget(TuneManager *const p_tune_manager, QWidget *parent = nullptr);
	~ChiptuneMidiWidget()  Q_DECL_OVERRIDE;
signals:

private:
	bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE;
	void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
	void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

	void dragEnterEvent(QDragEnterEvent* event) Q_DECL_OVERRIDE;
	void dragMoveEvent(QDragMoveEvent* event) Q_DECL_OVERRIDE;
	void dragLeaveEvent(QDragLeaveEvent* event) Q_DECL_OVERRIDE;
	void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

	void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
private slots:
	void on_OpenMidiFilePushButton_released(void);
	void on_SaveSaveFilePushButton_released(void);

	void on_StopPushButton_released(void);
	void on_PlayPausePushButton_released(void);
	void on_AmplitudeGainSlider_sliderMoved(int value);
	void on_AllOutputDisabledPushButton_released(void);
	void on_AllOutputEnabledPushButton_released(void);
	void on_PitchShiftSpinBox_valueChanged(int i);
	void on_PlayingSpeedRatioComboBox_currentIndexChanged(int i);
private slots:
	void on_PlayProgressSlider_sliderMoved(int value);
private slots:
	void HandlePlayProgressSliderMousePressed(Qt::MouseButton button, int value);
	void HandleAudioPlayerStateChanged(AudioPlayer::PlaybackState state);

	void HandleChannelOutputEnabled(int index, bool is_enabled);
	void HandlePitchTimbreValueFrameChanged(int index,
											int waveform,
											int envelope_attack_curve, double envelope_attack_duration_in_seconds,
											int envelope_decay_curve, double envelope_decay_duration_in_seconds,
											int envelope_sustain_level,
											int envelope_release_curve, double envelope_release_duration_in_seconds,
											int envelope_damper_on_but_note_off_sustain_level,
											int envelope_damper_on_but_note_off_sustain_curve,
											double envelope_damper_on_but_note_off_sustain_duration_in_seconds);
private:
	int PlayMidiFile(QString filename_string);
	void SetTuneStartTimeAndCheckPlayPausePushButtonIconToPlay(int start_time_in_milliseconds);
	void StopMidiFile(void);
	void UpdateTempoLabelText(void);

private:
	bool IsPlayPauseButtonInPlayState(void);
	void SetPlayPauseButtonInPlayState(bool is_play_state);

private :
	TuneManager *		m_p_tune_manager;
	AudioPlayer *		m_p_audio_player;

	QFileInfo			m_opened_file_info;
	uint32_t			m_midi_file_duration_in_milliseconds;
	int					m_inquiring_playback_state_timer_id;
	int					m_inquiring_playback_tick_timer_id;
	QString				m_midi_file_duration_time_string;

	QTimer				m_defer_start_play_timer;

	int					m_audio_player_buffer_in_milliseconds;
private:
	WaveChartView *		m_p_wave_chartview;
	SequencerWidget *	m_p_sequencer_widget;
private:
	Ui::ChiptuneMidiWidget *ui;
};

#endif // _CHIPTUNEMIDIWIDGET_H_
