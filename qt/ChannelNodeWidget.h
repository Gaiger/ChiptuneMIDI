#ifndef _CHANNELNODEWIDGET_H_
#define _CHANNELNODEWIDGET_H_

#include <QSize>
#include <QString>
#include <QWidget>

class MelodicTimbreFrame;

class ChannelNodeWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ChannelNodeWidget(int const channel_index, QWidget *parent = nullptr);
	~ChannelNodeWidget() Q_DECL_OVERRIDE = default;

	void SetInstrument(int const instrument_code);
	virtual void SetIndicator(bool const is_to_highlight);
	void GetMelodicChannelTimbre(int * const p_waveform,
				   int * const p_envelope_attack_curve, double * const p_envelope_attack_duration_in_seconds,
				   int * const p_envelope_decay_curve, double * const p_envelope_decay_duration_in_seconds,
				   int * const p_envelope_note_on_sustain_level,
				   int * const p_envelope_release_curve, double * const p_envelope_release_duration_in_seconds,
				   int * const p_envelope_note_off_hold_sustain_level,
				   int * const p_envelope_note_off_hold_sustain_curve,
				   double * const p_envelope_note_off_hold_sustain_duration_in_seconds);
	void SetMelodicChannelTimbre(int const waveform,
				   int const envelope_attack_curve, double const envelope_attack_duration_in_seconds,
				   int const envelope_decay_curve, double const envelope_decay_duration_in_seconds,
				   int const envelope_note_on_sustain_level,
				   int const envelope_release_curve, double const envelope_release_duration_in_seconds,
				   int const envelope_note_off_hold_sustain_level,
				   int const envelope_note_off_hold_sustain_curve,
				   double const envelope_note_off_hold_sustain_duration_in_seconds,
				   bool const is_to_darker_title_for_a_while);
	virtual void SetOutputEnabled(bool const is_to_enabled);
	virtual bool IsOutputEnabled(void) const;
signals:
	void OutputEnabled(int channel_index, bool is_enabled);
	void MelodicChannelInstrumentChanged(int channel_index, int instrument_code);
	void MelodicChannelTimbreChanged(int channel_index,
				   int waveform,
				   int envelope_attack_curve, double envelope_attack_duration_in_seconds,
				   int envelope_decay_curve, double envelope_decay_duration_in_seconds,
				   int envelope_note_on_sustain_level,
				   int envelope_release_curve, double envelope_release_duration_in_seconds,
				   int envelope_note_off_hold_sustain_level,
				   int envelope_note_off_hold_sustain_curve,
				   double envelope_note_off_hold_sustain_duration_in_seconds);
public slots:
	void on_ExpandCollapsePushButton_clicked(bool const checked);
	void HandleMelodicTimbreFrameTimbreChanged(int const waveform,
											   int const envelope_attack_curve, double const envelope_attack_duration_in_seconds,
											   int const envelope_decay_curve, double const envelope_decay_duration_in_seconds,
											   int const envelope_note_on_sustain_level,
											   int const envelope_release_curve, double const envelope_release_duration_in_seconds,
											   int const envelope_note_off_hold_sustain_level,
											   int const envelope_note_off_hold_sustain_curve,
											   double const envelope_note_off_hold_sustain_duration_in_seconds);
protected:
	virtual int GetDisplayedChannelIndex(void) const;
	virtual void SetTitleText(QString const &text) = 0;
	virtual void SetTitleStyleSheet(QString const &style_sheet) = 0;
	virtual QString GetTitleStyleSheet(void) const = 0;
	void SetupMelodicTimbreWidget(QWidget * const p_melodic_timbre_widget);
	void SetupTitleWidget(void);

	int const m_channel_index;
	int m_instrument_code;
	QSize m_expanded_size;
	QSize m_collapsed_size;
	QString m_title_widget_original_style_sheet;
	MelodicTimbreFrame *m_p_melodic_timbre_frame;
};

#endif // _CHANNELNODEWIDGET_H_
