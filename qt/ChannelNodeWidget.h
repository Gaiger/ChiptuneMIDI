#ifndef CHANNELNODEWIDGET_H
#define CHANNELNODEWIDGET_H

#include <QWidget>

namespace Ui {
class ChannelNodeWidget;
}

class ChannelNodeWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ChannelNodeWidget(int channel_index, int instrument_index,
							   QWidget *parent = nullptr);
	~ChannelNodeWidget();

public :
	signals:
void OutputEnabled(int index, bool is_enabled);
void TimbreChanged(int index,
				   int waveform,
				   int envelope_attack_curve, double envelope_attack_duration_in_seconds,
				   int envelope_decay_curve, double envelope_decay_duration_in_seconds,
				   int envelope_sustain_level,
				   int envelope_release_curve, double envelope_release_duration_in_seconds,
				   int envelope_damper_on_but_note_off_sustain_level,
				   int envelope_damper_on_but_note_off_sustain_curve,
				   double envelope_damper_on_but_note_off_sustain_duration_in_seconds);
public :
	void setOutputEnabled(bool is_to_enabled);
private slots:
	void on_OutputEnabledCheckBox_stateChanged(int state);
	void on_ExpandCollapsePushButton_clicked(bool checked);

private:
	int m_channel_index;
	QSize m_expanded_size;
	QSize m_collapsed_size;

private:
	Ui::ChannelNodeWidget *ui;
};

#endif // CHANNELNODEWIDGET_H
