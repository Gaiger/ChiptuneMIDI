#include <QDebug>

#include "PitchTimbreFrame.h"
#include "TuneManager.h"
#include "ui_PitchTimbreFrameForm.h"

PitchTimbreFrame::PitchTimbreFrame(int index, QWidget *parent)
	: QFrame(parent),
	  m_index(index),
	  ui(new Ui::PitchTimbreFrame)
{
	ui->setupUi(this);
	ui->IndexLabel->setText( "#" + QString::number(index));
#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL			(9)
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == index){
		ui->IndexLabel->setText( "#" + QString::number(index) + QString(" (Percussion)"));
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
}

/**********************************************************************************/

PitchTimbreFrame::~PitchTimbreFrame(void) { }

/**********************************************************************************/

void PitchTimbreFrame::EmitValuesChanged(void)
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

	emit ValuesChanged(m_index, waveform,
						ui->AttackCurveComboBox->currentIndex(), ui->AttackTimeSpinBox->value()/1000.0,
						ui->DecayCurveComboBox->currentIndex(), ui->DecayTimeSpinBox->value()/1000.0,
						ui->SustainLevelSpinBox->value(),
						ui->ReleaseCurveComboBox->currentIndex(), ui->ReleaseTimeSpinBox->value()/1000.0,
						ui->DamperOnButNoteOffSustainSustainLevelSpinBox->value(),
						ui->DamperOnButNoteOffSustainCurveComboBox->currentIndex(),
						ui->DamperOnButNoteOffSustainTimeDoubleSpinBox->value());
}
/**********************************************************************************/

void PitchTimbreFrame::on_OutputEnabledCheckBox_stateChanged(int state)
{
	emit OutputEnabled(m_index, (bool)state);
}

/**********************************************************************************/

void PitchTimbreFrame::on_WaveFormComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	qDebug() << Q_FUNC_INFO;
	do {
		if(QString("Square") == ui->WaveFormComboBox->currentText()){
			ui->DutyCycleComboBox->setEnabled(true);
			break;
		}
		ui->DutyCycleComboBox->setEnabled(false);
	} while(0);
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DutyCycleComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	qDebug() << Q_FUNC_INFO;
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_AttackCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	qDebug() << Q_FUNC_INFO;
	EmitValuesChanged();
}
/**********************************************************************************/

void PitchTimbreFrame::on_AttackTimeSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	qDebug() << Q_FUNC_INFO;
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DecayCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	qDebug() << Q_FUNC_INFO;
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DecayTimeSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	qDebug() << Q_FUNC_INFO;
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_SustainLevelSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	qDebug() << Q_FUNC_INFO;
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_ReleaseCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	qDebug() << Q_FUNC_INFO;
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_ReleaseTimeSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	qDebug() << Q_FUNC_INFO;
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DamperOnButNoteOffSustainTimeDoubleSpinBox_valueChanged(double value)
{
	Q_UNUSED(value);
	qDebug() << Q_FUNC_INFO;
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DamperOnButNoteOffSustainCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	qDebug() << Q_FUNC_INFO;
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DamperOnButNoteOffSustainSustainLevelSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	qDebug() << Q_FUNC_INFO;
	EmitValuesChanged();
}
