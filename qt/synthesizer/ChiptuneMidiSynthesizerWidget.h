#ifndef _CHIPTUNEMIDISYNTHESIZERWIDGET_H_
#define _CHIPTUNEMIDISYNTHESIZERWIDGET_H_

#include <QMap>
#include <QWidget>

#include "chiptune_midi_define.h"

#include "TuneManager.h"
#include "AudioPlayer.h"
#include "ChiptuneMidiValues.h"
#include "SynthesizerSequencerWidget.h"

class WaveChartView;
class QTimerEvent;
class MidiInputManager;
class ChannelListWidget;

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
	void on_LoadTimbresPushButton_toggled(bool is_checked);
	void on_StoreTimbresPushButton_released(void);
	void on_OpenCloseInputPortPushButton_toggled(bool is_checked);
	void on_SequencerRollPushButton_toggled(bool is_checked);
	void on_SequencerWaterfallPushButton_toggled(bool is_checked);
	void HandleMidiMessageDelivered(uint32_t midi_message);
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
	void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
	void UpdateInputPortComboBoxItems(void);
	void UpdateIndicatorsAndSequencerByMidiMessage(uint32_t const midi_message);
	int LoadAndApplyTimbres(void);
	void ApplyMelodicChannelInstrumentTimbre(int channel_index, int instrument_code,
											 bool is_to_darker_title_for_a_while);
private:
	TuneManager * const	m_p_tune_manager;
	AudioPlayer *		m_p_audio_player;
	WaveChartView *		m_p_wave_chartview;
	MidiInputManager *	m_p_midi_input_manager;
	ChannelListWidget *	m_p_channel_list_widget;
	SynthesizerSequencerWidget *m_p_synthesizer_sequencer_widget;

	int					m_audio_player_buffer_in_milliseconds;
	int					m_inquiry_midi_input_port_timer_id;
	int					m_synthesizer_sequencer_update_timer_id;
	int					m_channel_note_on_count_array[MIDI_MAX_CHANNEL_NUMBER];
	QMap<int8_t, instrument_timbre_t> m_ini_instrument_timbre_map;
private:
	Ui::ChiptuneMidiSynthesizerWidget *ui;
};

#endif // _CHIPTUNEMIDISYNTHESIZERWIDGET_H_
