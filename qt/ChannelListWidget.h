#ifndef _CHANNELLISTWIDGET_H_
#define _CHANNELLISTWIDGET_H_

#include <QWidget>
#include <QVBoxLayout>
#include <QMap>

namespace Ui {
class ChannelListWidget;
}

class ChannelListWidget : public QWidget
{
	Q_OBJECT

public :
	explicit ChannelListWidget(QWidget *parent = nullptr);
	~ChannelListWidget();

	void AddChannel(int channel_index, int instrument_index);
	void SetAllOutputEnabled(bool is_enabled);

	void GetTimbre(int channel_index, int *p_waveform,
				   int *p_envelope_attack_curve, double *p_envelope_attack_duration_in_seconds,
				   int *p_envelope_decay_curve, double *p_envelope_decay_duration_in_seconds,
				   int *p_envelope_note_on_sustain_level,
				   int *p_envelope_release_curve, double *p_envelope_release_duration_in_seconds,
				   int *p_envelope_damper_sustain_level,
				   int *p_envelope_damper_sustain_curve,
				   double *p_envelope_damper_sustain_duration_in_seconds);
public:
	signals:
void OutputEnabled(int index, bool is_enabled);
void TimbreChanged(int index,
				   int waveform,
				   int envelope_attack_curve, double envelope_attack_duration_in_seconds,
				   int envelope_decay_curve, double envelope_decay_duration_in_seconds,
				   int envelope_note_on_sustain_level,
				   int envelope_release_curve, double envelope_release_duration_in_seconds,
				   int envelope_damper_sustain_level,
				   int envelope_damper_sustain_curve,
				   double envelope_damper_sustain_duration_in_seconds);
private:
	QVBoxLayout *m_p_vboxlayout;
	QMap<int, int> m_channel_position_map;
private:
	Ui::ChannelListWidget *ui;
};

#endif // _CHANNELLISTWIDGET_H_
