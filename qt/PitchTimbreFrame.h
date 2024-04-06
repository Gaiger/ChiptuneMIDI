#ifndef PITCHTIMBREFRAME_H
#define PITCHTIMBREFRAME_H

namespace Ui {
class PitchTimbreFrame;
}

#include <QFrame>

class PitchTimbreFrame : public QFrame
{
	Q_OBJECT
public:
	explicit PitchTimbreFrame(int index, QWidget *parent = nullptr);
	~PitchTimbreFrame(void) Q_DECL_OVERRIDE;
public:
	enum WaveformType
	{
		WAVEFORM_SQUARE_DUDYCYCLE_50 = 0,
		WAVEFORM_SQUARE_DUDYCYCLE_25,
		WAVEFORM_SQUARE_DUDYCYCLE_125,
		WAVEFORM_SQUARE_DUDYCYCLE_75,
		WAVEFORM_TRIANGLE,
		WAVEFORM_SAW,
		WAVEFORM_NOISE,
	}; Q_ENUM(WaveformType)


	signals:
	void OutputEnabled(int index, bool is_enabled);
	void ValuesChanged(int index,
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

	void on_WaveFormComboBox_currentIndexChanged(int index);
	void on_DutyCycleComboBox_currentIndexChanged(int index);

	void on_AttackCurveComboBox_currentIndexChanged(int index);
	void on_AttackTimeSpinBox_valueChanged(int value);

	void on_DecayCurveComboBox_currentIndexChanged(int index);
	void on_DecayTimeSpinBox_valueChanged(int value);

	void on_SustainLevelSpinBox_valueChanged(int value);

	void on_ReleaseCurveComboBox_currentIndexChanged(int index);
	void on_ReleaseTimeSpinBox_valueChanged(int value);

	void on_DamperOnButNoteOffSustainTimeDoubleSpinBox_valueChanged(double value);
	void on_DamperOnButNoteOffSustainCurveComboBox_currentIndexChanged(int index);
	void on_DamperOnButNoteOffSustainSustainLevelSpinBox_valueChanged(int value);
private :
	void EmitValuesChanged(void);
private:
	int m_index;
	int m_previous_dutycycle;
	int m_previous_sustain_level;
private:
	Ui::PitchTimbreFrame *ui;
};

#endif // PITCHTIMBREFRAME_H
