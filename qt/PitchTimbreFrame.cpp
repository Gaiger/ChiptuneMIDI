#include <QDebug>

#include "PitchTimbreFrame.h"
#include "TuneManager.h"
#include "ui_PitchTimbreFrameForm.h"

#include <QStandardItemModel>

static QString GetInstrumentNameString(int instrument)
{
	QString instrument_name_string = QString("Not Specified");
	switch(instrument){
	case 0:
		instrument_name_string = QString("Acoustic Grand Piano");
		break;
	case 1:
		instrument_name_string = QString("Bright Acoustic Piano");
		break;
	case 2:
		instrument_name_string = QString("Electric Grand Piano");
		break;
	case 3:
		instrument_name_string = QString("Honky-tonk Piano");
		break;
	case 4:
		instrument_name_string = QString("Rhodes Piano");
		break;
	case 5:
		instrument_name_string = QString("Chorused Piano");
		break;
	case 6:
		instrument_name_string = QString("Harpsichord");
		break;
	case 7:
		instrument_name_string = QString("Clavinet");
		break;

	case 8:
		instrument_name_string = QString("Celesta");
		break;
	case 9:
		instrument_name_string = QString("Glockenspiel");
		break;
	case 10:
		instrument_name_string = QString("Music box");
		break;
	case 11:
		instrument_name_string = QString("Vibraphone");
		break;
	case 12:
		instrument_name_string = QString("Marimba");
		break;
	case 13:
		instrument_name_string = QString("Xylophone");
		break;
	case 14:
		instrument_name_string = QString("Tubular Bells");
		break;
	case 15:
		instrument_name_string = QString("Dulcimer");
		break;

	case 16:
		instrument_name_string = QString("Hammond Organ");
		break;
	case 17:
		instrument_name_string = QString("Percussive Organ");
		break;
	case 18:
		instrument_name_string = QString("	Rock Organ");
		break;
	case 19:
		instrument_name_string = QString("Church Organ");
		break;
	case 20:
		instrument_name_string = QString("Reed Organ");
		break;
	case 21:
		instrument_name_string = QString("Accordion");
		break;
	case 22:
		instrument_name_string = QString("Harmonica");
		break;
	case 23:
		instrument_name_string = QString("Tango Accordion");
		break;

	case 24:
		instrument_name_string = QString("Acoustic Guitar (nylon)");
		break;
	case 25:
		instrument_name_string = QString("Acoustic Guitar (steel)");
		break;
	case 26:
		instrument_name_string = QString("Electric Guitar (jazz)");
		break;
	case 27:
		instrument_name_string = QString("Electric Guitar (clean)");
		break;
	case 28:
		instrument_name_string = QString("Electric Guitar (muted)");
		break;
	case 29:
		instrument_name_string = QString("Overdriven Guitar");
		break;
	case 30:
		instrument_name_string = QString("Distortion Guitar");
		break;
	case 31:
		instrument_name_string = QString("Guitar Harmonics");
		break;

	case 32:
		instrument_name_string = QString("Acoustic Bass");
		break;
	case 33:
		instrument_name_string = QString("Electric Bass (finger)");
		break;
	case 34:
		instrument_name_string = QString("Electric Bass (pick)");
		break;
	case 35:
		instrument_name_string = QString("Fretless Bass");
		break;
	case 36:
		instrument_name_string = QString("Slap Bass 1");
		break;
	case 37:
		instrument_name_string = QString("Slap Bass 2");
		break;
	case 38:
		instrument_name_string = QString("Synth Bass 1");
		break;
	case 39:
		instrument_name_string = QString("Synth Bass 2");
		break;

	case 40:
		instrument_name_string = QString("Violin");
		break;
	case 41:
		instrument_name_string = QString("Viola");
		break;
	case 42:
		instrument_name_string = QString("Cello");
		break;
	case 43:
		instrument_name_string = QString("Contrabass");
		break;
	case 44:
		instrument_name_string = QString("Tremolo Strings");
		break;
	case 45:
		instrument_name_string = QString("Pizzicato Strings");
		break;
	case 46:
		instrument_name_string = QString("Orchestral Harp");
		break;
	case 47:
		instrument_name_string = QString("Timpani");
		break;

	case 48:
		instrument_name_string = QString("String Ensemble 1");
		break;
	case 49:
		instrument_name_string = QString("String Ensemble 2");
		break;
	case 50:
		instrument_name_string = QString("Synth Strings 1");
		break;
	case 51:
		instrument_name_string = QString("Synth Strings 2");
		break;
	case 52:
		instrument_name_string = QString("Choir Aahs");
		break;
	case 53:
		instrument_name_string = QString("Voice Oohs");
		break;
	case 54:
		instrument_name_string = QString("Synth Voice");
		break;
	case 55:
		instrument_name_string = QString("Orchestra Hit");
		break;

	case 56:
		instrument_name_string = QString("Trumpet");
		break;
	case 57:
		instrument_name_string = QString("Trombone");
		break;
	case 58:
		instrument_name_string = QString("Tuba");
		break;
	case 59:
		instrument_name_string = QString("Muted Trumpet");
		break;
	case 60:
		instrument_name_string = QString("French Horn");
		break;
	case 61:
		instrument_name_string = QString("Brass Section");
		break;
	case 62:
		instrument_name_string = QString("Synth Brass 1");
		break;
	case 63:
		instrument_name_string = QString("Synth Brass 2");
		break;

	case 64:
		instrument_name_string = QString("Soprano Sax");
		break;
	case 65:
		instrument_name_string = QString("Alto Sax");
		break;
	case 66:
		instrument_name_string = QString("Tenor Sax");
		break;
	case 67:
		instrument_name_string = QString("Baritone Sax");
		break;
	case 68:
		instrument_name_string = QString("Oboe");
		break;
	case 69:
		instrument_name_string = QString("English Horn");
		break;
	case 70:
		instrument_name_string = QString("Bassoon");
		break;
	case 71:
		instrument_name_string = QString("Clarinet");
		break;


	case 72:
		instrument_name_string = QString("Piccolo");
		break;
	case 73:
		instrument_name_string = QString("Flute");
		break;
	case 74:
		instrument_name_string = QString("Recorder");
		break;
	case 75:
		instrument_name_string = QString("Pan Flute");
		break;
	case 76:
		instrument_name_string = QString("Bottle Blow");
		break;
	case 77:
		instrument_name_string = QString("Shakuhachi");
		break;
	case 78:
		instrument_name_string = QString("Ocarina");
		break;
	case 79:
		instrument_name_string = QString("Harmonica");
		break;

	case 80:
		instrument_name_string = QString("Lead 1 (square)");
		break;
	case 81:
		instrument_name_string = QString("Lead 2 (sawtooth)");
		break;
	case 82:
		instrument_name_string = QString("Lead 3 (calliope lead)");
		break;
	case 83:
		instrument_name_string = QString("Lead 4 (chiffer lead)");
		break;
	case 84:
		instrument_name_string = QString("Lead 5 (charang)");
		break;
	case 85:
		instrument_name_string = QString("Lead 6 (voice)");
		break;
	case 86:
		instrument_name_string = QString("Lead 7 (fifths)");
		break;
	case 87:
		instrument_name_string = QString("Lead 8 (brass + lead)");
		break;

	case 88:
		instrument_name_string = QString("Pad 1 (new age)");
		break;
	case 89:
		instrument_name_string = QString("Pad 2 (warm)");
		break;
	case 90:
		instrument_name_string = QString("Pad 3 (polysynth)");
		break;
	case 91:
		instrument_name_string = QString("Pad 4 (choir)");
		break;
	case 92:
		instrument_name_string = QString("Pad 5 (bowed)");
		break;
	case 93:
		instrument_name_string = QString("Pad 6 (metallic)");
		break;
	case 94:
		instrument_name_string = QString("Pad 7 (halo)");
		break;
	case 95:
		instrument_name_string = QString("Pad 8 (sweep)");
		break;

	case 96:
		instrument_name_string = QString("FX 1 (rain)");
		break;
	case 97:
		instrument_name_string = QString("FX 2 (soundtrack)");
		break;
	case 98:
		instrument_name_string = QString("FX 3 (crystal)");
		break;
	case 99:
		instrument_name_string = QString("FX 4 (atmosphere)");
		break;
	case 100:
		instrument_name_string = QString("FX 5 (brightness)");
		break;
	case 101:
		instrument_name_string = QString("FX 6 (goblins)");
		break;
	case 102:
		instrument_name_string = QString("FX 7 (echoes)");
		break;
	case 103:
		instrument_name_string = QString("FX 8 (sci-fi)");
		break;

	case 104:
		instrument_name_string = QString("Sitar");
		break;
	case 105:
		instrument_name_string = QString("Banjo");
		break;
	case 106:
		instrument_name_string = QString("Shamisen");
		break;
	case 107:
		instrument_name_string = QString("Koto");
		break;
	case 108:
		instrument_name_string = QString("Kalimba");
		break;
	case 109:
		instrument_name_string = QString("Bagpipe");
		break;
	case 110:
		instrument_name_string = QString("Fiddle");
		break;
	case 111:
		instrument_name_string = QString("Shana");
		break;

	case 112:
		instrument_name_string = QString("Tinkle Bell");
		break;
	case 113:
		instrument_name_string = QString("Agogo");
		break;
	case 114:
		instrument_name_string = QString("Steel Drums");
		break;
	case 115:
		instrument_name_string = QString("Woodblock");
		break;
	case 116:
		instrument_name_string = QString("Taiko Drum");
		break;
	case 117:
		instrument_name_string = QString("Melodic Tom");
		break;
	case 118:
		instrument_name_string = QString("Synth Drum");
		break;
	case 119:
		instrument_name_string = QString("Reverse Cymbal");
		break;

	case 120:
		instrument_name_string = QString("Guitar Fret Noise");
		break;
	case 121:
		instrument_name_string = QString("Breath Noise");
		break;
	case 122:
		instrument_name_string = QString("Seashore");
		break;
	case 123:
		instrument_name_string = QString("Bird Tweet");
		break;
	case 124:
		instrument_name_string = QString("Telephone Ring");
		break;
	case 125:
		instrument_name_string = QString("Helicopter");
		break;
	case 126:
		instrument_name_string = QString("Applause");
		break;
	case 127:
		instrument_name_string = QString("Gunshot");
		break;

	default:
		break;
	}

	return instrument_name_string;
}

/**********************************************************************************/

PitchTimbreFrame::PitchTimbreFrame(int index, int inustrument, QWidget *parent)
	: QFrame(parent),
	  m_index(index),
	  ui(new Ui::PitchTimbreFrame)
{
	ui->setupUi(this);
	m_previous_dutycycle = 0;// dutycycle 50
	m_previous_sustain_level = ui->SustainLevelSpinBox->value();

	ui->IndexLabel->setText( "#" + QString::number(index) + " " + GetInstrumentNameString(inustrument));
#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL			(9)
	if(MIDI_PERCUSSION_INSTRUMENT_CHANNEL == index){
		ui->IndexLabel->setText( "#" + QString::number(index) + QString(" Percussion"));
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

void PitchTimbreFrame::setOutputEnabled(bool is_to_enabled)
{
	ui->OutputEnabledCheckBox->setChecked(is_to_enabled);
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
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DutyCycleComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	if(4 != index){
		m_previous_dutycycle = ui->DutyCycleComboBox->currentIndex();
	}
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_AttackCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	EmitValuesChanged();
}
/**********************************************************************************/

void PitchTimbreFrame::on_AttackTimeSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DecayCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	EmitValuesChanged();
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

	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_SustainLevelSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_ReleaseCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_ReleaseTimeSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DamperOnButNoteOffSustainTimeDoubleSpinBox_valueChanged(double value)
{
	Q_UNUSED(value);
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DamperOnButNoteOffSustainCurveComboBox_currentIndexChanged(int index)
{
	Q_UNUSED(index);
	EmitValuesChanged();
}

/**********************************************************************************/

void PitchTimbreFrame::on_DamperOnButNoteOffSustainSustainLevelSpinBox_valueChanged(int value)
{
	Q_UNUSED(value);
	EmitValuesChanged();
}
