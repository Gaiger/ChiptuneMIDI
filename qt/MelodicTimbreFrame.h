#ifndef _MELODICTIMBREFRAME_H_
#define _MELODICTIMBREFRAME_H_

namespace Ui {
class MelodicTimbreFrame;
}

#include <QFrame>

class MelodicTimbreFrame : public QFrame
{
	Q_OBJECT
public:
	explicit MelodicTimbreFrame(QWidget *parent = nullptr);
	~MelodicTimbreFrame(void) Q_DECL_OVERRIDE;
public:
	enum WaveformType
	{
		WaveformSquareDutyCycle50	= 0,
		WaveformSquareDutyCycle25,
		WaveformSquareDutyCycle12_5,
		WaveformSquareDutyCycle75,
		WaveformTriangle,
		WaveformSaw,
		WaveformNoise,
	}; Q_ENUM(WaveformType)

	void GetTimbre(int *p_waveform,
				   int *p_envelope_attack_curve, double *p_envelope_attack_duration_in_seconds,
				   int *p_envelope_decay_curve, double *p_envelope_decay_duration_in_seconds,
				   int *p_envelope_note_on_sustain_level,
				   int *p_envelope_release_curve, double *p_envelope_release_duration_in_seconds,
				   int *p_envelope_damper_sustain_level,
				   int *p_envelope_damper_sustain_curve,
				   double *p_envelope_damper_sustain_duration_in_seconds);

public:
	signals:
	void TimbreChanged(int waveform,
					   int envelope_attack_curve, double envelope_attack_duration_in_seconds,
					   int envelope_decay_curve, double envelope_decay_duration_in_seconds,
					   int envelope_note_on_sustain_level,
					   int envelope_release_curve, double envelope_release_duration_in_seconds,
					   int envelope_damper_sustain_level,
					   int envelope_damper_sustain_curve,
					   double envelope_damper_sustain_duration_in_seconds);

private slots:
	void on_WaveFormComboBox_currentIndexChanged(int index);
	void on_DutyCycleComboBox_currentIndexChanged(int index);

	void on_AttackCurveComboBox_currentIndexChanged(int index);
	void on_AttackTimeSpinBox_valueChanged(int value);

	void on_DecayCurveComboBox_currentIndexChanged(int index);
	void on_DecayTimeSpinBox_valueChanged(int value);

	void on_SustainLevelSpinBox_valueChanged(int value);

	void on_ReleaseCurveComboBox_currentIndexChanged(int index);
	void on_ReleaseTimeSpinBox_valueChanged(int value);

	void on_DamperSustainTimeDoubleSpinBox_valueChanged(double value);
	void on_DamperSustainCurveComboBox_currentIndexChanged(int index);
	void on_DamperSustainLevelSpinBox_valueChanged(int value);
private :
	WaveformType GetWaveform(void);
	void EmitMelodicChannelTimbreChanged(void);
private:
	int m_previous_dutycycle;
	int m_previous_sustain_level;
private:
	Ui::MelodicTimbreFrame *ui;
};

#endif // _MELODICTIMBREFRAME_H_
