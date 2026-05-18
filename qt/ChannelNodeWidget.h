#ifndef _CHANNELNODEWIDGET_H_
#define _CHANNELNODEWIDGET_H_

#include <QString>
#include <QWidget>

namespace Ui {
class ChannelNodeWidget;
}

class MelodicTimbreFrame;

class ChannelNodeWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ChannelNodeWidget(int channel_index, int instrument_code,
							   bool is_displayed_channel_index_start_from_one = false,
							   bool is_channel_indicator_enabled = false,
							   QWidget *parent = nullptr);
	~ChannelNodeWidget();

	void SetInstrument(int instrument_code);
	void SetIndicator(bool is_to_highlight);
	void GetMelodicChannelTimbre(int *p_waveform,
				   int *p_envelope_attack_curve, double *p_envelope_attack_duration_in_seconds,
				   int *p_envelope_decay_curve, double *p_envelope_decay_duration_in_seconds,
				   int *p_envelope_note_on_sustain_level,
				   int *p_envelope_release_curve, double *p_envelope_release_duration_in_seconds,
				   int *p_envelope_damper_sustain_level,
				   int *p_envelope_damper_sustain_curve,
				   double *p_envelope_damper_sustain_duration_in_seconds);
	void SetMelodicChannelTimbre(int waveform,
				   int envelope_attack_curve, double envelope_attack_duration_in_seconds,
				   int envelope_decay_curve, double envelope_decay_duration_in_seconds,
				   int envelope_note_on_sustain_level,
				   int envelope_release_curve, double envelope_release_duration_in_seconds,
				   int envelope_damper_sustain_level,
				   int envelope_damper_sustain_curve,
				   double envelope_damper_sustain_duration_in_seconds,
				   bool is_to_darker_title_for_a_while);
public :
	signals:
void OutputEnabled(int channel_index, bool is_enabled);
void MelodicChannelTimbreChanged(int channel_index,
				   int waveform,
				   int envelope_attack_curve, double envelope_attack_duration_in_seconds,
				   int envelope_decay_curve, double envelope_decay_duration_in_seconds,
				   int envelope_note_on_sustain_level,
				   int envelope_release_curve, double envelope_release_duration_in_seconds,
				   int envelope_damper_sustain_level,
				   int envelope_damper_sustain_curve,
				   double envelope_damper_sustain_duration_in_seconds);
public :
	void SetOutputEnabled(bool is_to_enabled);
	bool IsOutputEnabled(void);
private slots:
	void on_OutputEnabledCheckBox_stateChanged(int state);
	void on_ExpandCollapsePushButton_clicked(bool checked);
private slots:
	void HandleMelodicTimbreFrameTimbreChanged(int waveform,
											   int envelope_attack_curve, double envelope_attack_duration_in_seconds,
											   int envelope_decay_curve, double envelope_decay_duration_in_seconds,
											   int envelope_note_on_sustain_level,
											   int envelope_release_curve, double envelope_release_duration_in_seconds,
											   int envelope_damper_sustain_level,
											   int envelope_damper_sustain_curve,
											   double envelope_damper_sustain_duration_in_seconds);
private:
	void SetupChannelIndicatorLayout(void);

	int const m_channel_index;
	int m_instrument_code;
	bool const m_is_displayed_channel_index_start_from_one;
	QSize m_expanded_size;
	QSize m_collapsed_size;
	QString m_expand_collapse_push_button_original_style_sheet;
	QString m_channel_indicator_widget_plain_style_sheet;
	QString m_channel_indicator_widget_highlight_style_sheet;

	MelodicTimbreFrame *m_p_melodic_timbre_frame;
	QWidget *m_p_channel_indicator_widget;
private:
	Ui::ChannelNodeWidget *ui;
};

#endif // _CHANNELNODEWIDGET_H_
