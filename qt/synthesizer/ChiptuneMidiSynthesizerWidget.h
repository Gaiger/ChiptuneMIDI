#ifndef _CHIPTUNEMIDISYNTHESIZERWIDGET_H_
#define _CHIPTUNEMIDISYNTHESIZERWIDGET_H_

#include <QWidget>

#include "TuneManager.h"
#include "AudioPlayer.h"

class WaveChartView;
class QKeyEvent;
class MidiInputManager;

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
	void on_LoadTimbresPushButton_released(void);
	void on_StoreTimbresPushButton_released(void);
	void HandleAudioPlayerStateChanged(AudioPlayer::PlaybackState state);
	void HandleChannelOutputEnabled(int channel_index, bool is_enabled);
	void HandleMelodicChannelTimbreChanged(int channel_index,
										   int waveform,
										   int envelope_attack_curve, double envelope_attack_duration_in_seconds,
										   int envelope_decay_curve, double envelope_decay_duration_in_seconds,
										   int envelope_note_on_sustain_level,
										   int envelope_release_curve, double envelope_release_duration_in_seconds,
										   int envelope_damper_sustain_level,
										   int envelope_damper_sustain_curve,
										   double envelope_damper_sustain_duration_in_seconds);
private:
	void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
	void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
private:
	TuneManager * const	m_p_tune_manager;
	AudioPlayer *		m_p_audio_player;
	WaveChartView *		m_p_wave_chartview;
	MidiInputManager *	m_p_midi_input_manager;

	int					m_audio_player_buffer_in_milliseconds;
private:
	Ui::ChiptuneMidiSynthesizerWidget *ui;
};

#endif // _CHIPTUNEMIDISYNTHESIZERWIDGET_H_
