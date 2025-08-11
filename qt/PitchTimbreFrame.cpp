#include <QDebug>
#include <QStandardItemModel>

#include "TuneManager.h"

#include "ui_PitchTimbreFrameForm.h"
#include "PitchTimbreFrame.h"

/**********************************************************************************/

PitchTimbreFrame::PitchTimbreFrame(int index, QWidget *parent)
	: QFrame(parent),
	  m_index(index),
	  ui(new Ui::PitchTimbreFrame)
{
	ui->setupUi(this);
	m_previous_dutycycle = 0;// dutycycle 50
	m_previous_sustain_level = ui->SustainLevelSpinBox->value();

#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL			(9)
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == index){
		ui->WaveFormComboBox->setEnabled(false);
		ui->AttackCurveComboBox->setEnabled(false);
		ui->AttackTimeSpinBox->setEnabled(false);
		ui->DecayCurveComboBox->setEnabled(false);
		ui->DecayTimeSpinBox->setEnabled(false);
		ui->SustainLevelSpinBox->setEnabled(false);
		ui->ReleaseCurveComboBox->setEnabled(false);
		ui->ReleaseTimeSpinBox->setEnabled(false);

		ui->DamperOnButNoteOffSustainSustainLevelSpinBox->setEnabled(false);
		ui->DamperOnButNoteOffSustainCurveComboBox->setEnabled(false);
		ui->DamperOnButNoteOffSustainTimeDoubleSpinBox->setEnabled(false);
	}
	QStandardItemModel *p_model =
		  qobject_cast<QStandardItemModel *>(ui->DutyCycleComboBox->model());
	QStandardItem *p_item = p_model->item(4); //no duty cycle
	p_item->setFlags(p_item->flags() & ~Qt::ItemIsEnabled);
}

/**********************************************************************************/

PitchTimbreFrame::~PitchTimbreFrame(void)
{
	delete ui;
}

/**********************************************************************************/

PitchTimbreFrame::WaveformType PitchTimbreFrame::GetWaveform(void)
{
	int waveform;
	switch(ui->WaveFormComboBox->currentIndex())
	{
	case 0: //square
		switch(ui->DutyCycleComboBox->currentIndex()){
		case 0://50
			waveform = TuneManager::WAVEFORM_SQUARE_DUDYCYCLE_50;
			break;
		case 1: //25
			waveform = TuneManager::WAVEFORM_SQUARE_DUDYCYCLE_25;
			break;
		case 2: //125
			waveform = TuneManager::WAVEFORM_SQUARE_DUDYCYCLE_125;
			break;
		case 3: //75
		default:
			waveform = TuneManager::WAVEFORM_SQUARE_DUDYCYCLE_75;
			break;
		}
		break;
	case 1: //triangle
		waveform = TuneManager::WAVEFORM_TRIANGLE;
		break;
	default:
	case 2:
		waveform =TuneManager::WAVEFORM_SAW;
		break;
	}
	return (PitchTimbreFrame::WaveformType)waveform;
}

/**********************************************************************************/

void PitchTimbreFrame::EmitTimbreChanged(void)
{
	emit TimbreChanged(m_index, GetWaveform(),
						ui->AttackCurveComboBox->currentIndex(), ui->AttackTimeSpinBox->value()/1000.0,
						ui->DecayCurveComboBox->currentIndex(), ui->DecayTimeSpinBox->value()/1000.0,
						ui->SustainLevelSpinBox->value(),
						ui->ReleaseCurveComboBox->currentIndex(), ui->ReleaseTimeSpinBox->value()/1000.0,
						ui->DamperOnButNoteOffSustainSustainLevelSpinBox->value(),
						ui->DamperOnButNoteOffSustainCurveComboBox->currentIndex(),
						ui->DamperOnButNoteOffSustainTimeDoubleSpinBox->value());
}

/**********************************************************************************/

void PitchTimbreFrame::GetTimbre(int *p_waveform,
			   int *p_envelope_attack_curve, double *p_envelope_attack_duration_in_seconds,
			   int *p_envelope_decay_curve, double *p_envelope_decay_duration_in_seconds,
			   int *p_envelope_sustain_level,
			   int *p_envelope_release_curve, double *p_envelope_release_duration_in_seconds,
			   int *p_envelope_damper_on_but_note_off_sustain_level,
			   int *p_envelope_damper_on_but_note_off_sustain_curve,
			   double *p_envelope_damper_on_but_note_off_sustain_duration_in_seconds)
{
	*p_waveform = GetWaveform();

	*p_envelope_attack_curve = ui->AttackCurveComboBox->currentIndex();
	*p_envelope_attack_duration_in_seconds = ui->AttackTimeSpinBox->value()/1000.0,

	*p_envelope_decay_curve = ui->DecayCurveComboBox->currentIndex();
	*p_envelope_decay_duration_in_seconds = ui->DecayTimeSpinBox->value()/1000.0;

	*p_envelope_sustain_level = ui->SustainLevelSpinBox->value();

	*p_envelope_release_curve = ui->ReleaseCurveComboBox->currentIndex();
	*p_envelope_release_duration_in_seconds = ui->ReleaseTimeSpinBox->value()/1000.0;

	*p_envelope_damper_on_but_note_off_sustain_level = ui->DamperOnButNoteOffSustainSustainLevelSpinBox->value();
	*p_envelope_damper_on_but_note_off_sustain_curve = ui->DamperOnButNoteOffSustainCurveComboBox->currentIndex();
	*p_envelope_damper_on_but_note_off_sustain_duration_in_seconds = ui->DamperOnButNoteOffSustainTimeDoubleSpinBox->value();
}

/**********************************************************************************/

void PitchTimbreFrame::on_WaveFormComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	do {
		if(QString("Square") == ui->WaveFormComboBox->currentText()){
			ui->DutyCycleComboBox->blockSignals(true);
			ui->DutyCycleComboBox->setCurrentIndex(m_previous_dutycycle);
			ui->DutyCycleComboBox->blockSignals(false);
			ui->DutyCycleComboBox->setEnabled(true);
			break;
		}
		ui->DutyCycleComboBox->setEnabled(false);
		QObject::blockSignals(true);
		ui->DutyCycleComboBox->setCurrentIndex(4);
		QObject::blockSignals(false);
	} while(0);
	EmitTimbreChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DutyCycleComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	if(4 != index){
		m_previous_dutycycle = ui->DutyCycleComboBox->currentIndex();
	}
	EmitTimbreChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_AttackCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	EmitTimbreChanged();
}
/**********************************************************************************/

void PitchTimbreFrame::on_AttackTimeSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	EmitTimbreChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DecayCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	EmitTimbreChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DecayTimeSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	do
	{
		if(0 == value){
			QObject::blockSignals(true);
			m_previous_sustain_level = ui->SustainLevelSpinBox->value();
			ui->SustainLevelSpinBox->setValue(128);
			ui->SustainLevelSpinBox->setEnabled(false);
			QObject::blockSignals(false);
			break;
		}

		if(false == ui->SustainLevelSpinBox->isEnabled()){
			QObject::blockSignals(true);
			ui->SustainLevelSpinBox->setValue(m_previous_sustain_level);
			ui->SustainLevelSpinBox->setEnabled(true);
			QObject::blockSignals(false);
		}
	} while(0);

	EmitTimbreChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_SustainLevelSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	EmitTimbreChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_ReleaseCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	EmitTimbreChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_ReleaseTimeSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	EmitTimbreChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DamperOnButNoteOffSustainTimeDoubleSpinBox_valueChanged(double value)
{
	Q_UNUSED(value);
	EmitTimbreChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DamperOnButNoteOffSustainCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	EmitTimbreChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DamperOnButNoteOffSustainSustainLevelSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	EmitTimbreChanged();
}
