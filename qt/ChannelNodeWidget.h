#ifndef _CHANNELNODEWIDGET_H_
#define _CHANNELNODEWIDGET_H_

#include <QWidget>

namespace Ui {
class ChannelNodeWidget;
}

class MelodicTimbreFrame;

class ChannelNodeWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ChannelNodeWidget(int channel_index, int instrument_index,
							   QWidget *parent = nullptr);
	~ChannelNodeWidget();

	void GetMelodicChannelTimbre(int *p_waveform,
				   int *p_envelope_attack_curve, double *p_envelope_attack_duration_in_seconds,
				   int *p_envelope_decay_curve, double *p_envelope_decay_duration_in_seconds,
				   int *p_envelope_note_on_sustain_level,
				   int *p_envelope_release_curve, double *p_envelope_release_duration_in_seconds,
				   int *p_envelope_damper_sustain_level,
				   int *p_envelope_damper_sustain_curve,
				   double *p_envelope_damper_sustain_duration_in_seconds);
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
	void setOutputEnabled(bool is_to_enabled);
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
	int const m_channel_index;
	QSize m_expanded_size;
	QSize m_collapsed_size;

	MelodicTimbreFrame *m_p_melodic_timbre_frame;
private:
	Ui::ChannelNodeWidget *ui;
};

#endif // _CHANNELNODEWIDGET_H_
