#ifndef CHIPTUNEMIDIWIDGET_H
#define CHIPTUNEMIDIWIDGET_H
#include <QTimer>
#include <QThread>
#include <QWidget>
#include <QToolBox>
#include <QFileInfo>

#include "TuneManager.h"
#include "AudioPlayer.h"

#include "WaveChartView.h"

namespace Ui {
class ChiptuneMidiWidget;
}

class SequencerWidget;

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
	void on_AllOutputDisabledPushButton_released(void);
	void on_AllOutputEnabledPushButton_released(void);
	void on_PitchShiftSpinBox_valueChanged(int i);
	void on_PlayingSpeedRatioComboBox_currentIndexChanged(int i);
private slots:
	void on_PlayProgressSlider_sliderMoved(int value);
private slots:
	void HandleWaveFetched(const QByteArray wave_bytearray);
	void HandlePlayProgressSliderPositionChanged(int value);
	void HandlePlayProgressSliderMouseRightReleased(QPoint position);
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
	bool IsPlayPausePushButtonPlayIcon(void);
	void SetPlayPausePushButtonAsPlayIcon(bool is_play_icon);

private :
	TuneManager *		m_p_tune_manager;
	QThread				m_tune_manager_working_thread;

	AudioPlayer *		m_p_audio_player;
	QThread				m_audio_player_working_thread;

	QFileInfo			m_opened_file_info;
	uint32_t			m_midi_file_duration_in_milliseconds;
	int					m_inquiring_playback_status_timer_id;
	int					m_inquiring_playback_tick_timer_id;
	QString				m_midi_file_duration_time_string;

	QTimer				m_set_start_time_postpone_timer;

	int					m_audio_player_buffer_in_milliseconds;
private:
	SequencerWidget	*	m_p_sequencer_widget;
	WaveChartView *		m_p_wave_chartview;
private:
	Ui::ChiptuneMidiWidget *ui;
};

#endif // CHIPTUNEMIDIWIDGET_H
