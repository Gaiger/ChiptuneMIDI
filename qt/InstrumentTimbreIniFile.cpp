#include "InstrumentTimbreIniFile.h"

#include "../chiptune.h"
#include "../chiptune_midi_define.h"

#include <QDebug>
#include <QFileInfo>
#include <QString>
#include <QStringList>

#define EXPAND_CASE_TO_STR(ENUMS_ELEMENT, VAL)					case VAL:	return #ENUMS_ELEMENT;
#define EXPAND_IF_STR_TO_CODE(ENUMS_ELEMENT, VAL)				\
																if(name_string == QStringLiteral(#ENUMS_ELEMENT)){ \
																	return VAL; \
																}

/******************************************************************************/
static char const * const GetInstrumentNameString(int8_t const instrument_code)
{
	switch (instrument_code)
	{
		MIDI_INSTRUMENT_CODE_LIST(EXPAND_CASE_TO_STR)
	}

	return "UnknownInstrument";
}

/******************************************************************************/
static int8_t GetInstrumentCode(QString const& name_string)
{
	MIDI_INSTRUMENT_CODE_LIST(EXPAND_IF_STR_TO_CODE)
	return -1;
}

#undef EXPAND_CASE_TO_STR
#undef EXPAND_IF_STR_TO_CODE

/******************************************************************************/
static QString GetWaveformNameString(int8_t const waveform)
{
	switch(waveform)
	{
	case ChiptuneWaveformSquareDutyCycle12_5:
	case ChiptuneWaveformSquareDutyCycle25:
	case ChiptuneWaveformSquareDutyCycle50:
	case ChiptuneWaveformSquareDutyCycle75:
		return QStringLiteral("square");
	case ChiptuneWaveformTriangle:
		return QStringLiteral("triangle");
	case ChiptuneWaveformSaw:
		return QStringLiteral("saw");
	default:
		return QStringLiteral("triangle");
	}
}

/******************************************************************************/
static QString GetDutycycleNameString(int8_t const waveform)
{
	switch(waveform)
	{
	case ChiptuneWaveformSquareDutyCycle12_5:
		return QStringLiteral("12.5");
	case ChiptuneWaveformSquareDutyCycle25:
		return QStringLiteral("25");
	case ChiptuneWaveformSquareDutyCycle50:
		return QStringLiteral("50");
	case ChiptuneWaveformSquareDutyCycle75:
		return QStringLiteral("75");
	default:
		return QStringLiteral("50");
	}
}

/******************************************************************************/
static int8_t GetWaveformCode(QString const& waveform_name_string, QString const& dutycycle_name_string)
{
	QString const waveform = waveform_name_string.trimmed().toLower();
	if(QStringLiteral("triangle") == waveform){
		return ChiptuneWaveformTriangle;
	}
	if(QStringLiteral("saw") == waveform){
		return ChiptuneWaveformSaw;
	}
	if(QStringLiteral("square") != waveform){
		return -1;
	}

	QString const dutycycle = dutycycle_name_string.trimmed();
	if(QStringLiteral("12.5") == dutycycle){
		return ChiptuneWaveformSquareDutyCycle12_5;
	}
	if(QStringLiteral("25") == dutycycle){
		return ChiptuneWaveformSquareDutyCycle25;
	}
	if(QStringLiteral("50") == dutycycle){
		return ChiptuneWaveformSquareDutyCycle50;
	}
	if(QStringLiteral("75") == dutycycle){
		return ChiptuneWaveformSquareDutyCycle75;
	}
	return -1;
}

/******************************************************************************/
static QString GetEnvelopeCurveNameString(int8_t const curve)
{
	switch(curve)
	{
	case ChiptuneEnvelopeCurveLinear:
		return QStringLiteral("linear");
	case ChiptuneEnvelopeCurveExponential:
		return QStringLiteral("exponential");
	case ChiptuneEnvelopeCurveGaussian:
		return QStringLiteral("gaussian");
	case ChiptuneEnvelopeCurveFermi:
		return QStringLiteral("fermi");
	default:
		return QStringLiteral("linear");
	}
}

/******************************************************************************/
static int8_t GetEnvelopeCurveCode(QString const& curve_name_string)
{
	QString const curve = curve_name_string.trimmed().toLower();
	if(QStringLiteral("linear") == curve){
		return ChiptuneEnvelopeCurveLinear;
	}
	if(QStringLiteral("exponential") == curve){
		return ChiptuneEnvelopeCurveExponential;
	}
	if(QStringLiteral("gaussian") == curve){
		return ChiptuneEnvelopeCurveGaussian;
	}
	if(QStringLiteral("fermi") == curve){
		return ChiptuneEnvelopeCurveFermi;
	}
	return -1;
}

/******************************************************************************/
InstrumentTimbreIniFile::InstrumentTimbreIniFile(QString const& file_name_string)
	: m_settings(file_name_string, QSettings::IniFormat)
{

}

/******************************************************************************/
int InstrumentTimbreIniFile::ReadTimbre(int8_t const instrument_code, int8_t * const p_waveform,
										int8_t * const p_envelope_attack_curve,
										float * const p_envelope_attack_duration_in_seconds,
										int8_t * const p_envelope_decay_curve,
										float * const p_envelope_decay_duration_in_seconds,
										uint8_t * const p_envelope_note_on_sustain_level,
										int8_t * const p_envelope_release_curve,
										float * const p_envelope_release_duration_in_seconds,
										uint8_t * const p_envelope_damper_sustain_level,
										int8_t * const p_envelope_damper_sustain_curve,
										float * const p_envelope_damper_sustain_duration_in_seconds)
{
	if(false == QFileInfo::exists(m_settings.fileName())){
		return -1;
	}

	QString const instrument_name_string = QString::fromLatin1(GetInstrumentNameString(instrument_code));
	if(false == m_settings.childGroups().contains(instrument_name_string)){
		return -2;
	}

	m_settings.beginGroup(instrument_name_string);
	int ret = 0;
	if(false == m_settings.contains("waveform")){
		qWarning() << "Instrument timbre missing key:" << instrument_name_string << "waveform";
		ret = 1;
		*p_waveform = ChiptuneWaveformTriangle;
	}else{
		QString const waveform_name_string = m_settings.value("waveform").toString();
		QString dutycycle_name_string = QStringLiteral("50");
		if(QStringLiteral("square") == waveform_name_string.trimmed().toLower()){
			if(false == m_settings.contains("dutycycle")){
				qWarning() << "Instrument timbre missing key:" << instrument_name_string << "dutycycle";
				ret = 1;
			}else{
				dutycycle_name_string = m_settings.value("dutycycle").toString();
			}
		}
		*p_waveform = GetWaveformCode(waveform_name_string, dutycycle_name_string);
		if(0 > *p_waveform){
			qWarning() << "Instrument timbre invalid waveform:"
					   << instrument_name_string << waveform_name_string << dutycycle_name_string;
			ret = 1;
			*p_waveform = ChiptuneWaveformTriangle;
		}
	}

	if(false == m_settings.contains("attack_curve")){
		qWarning() << "Instrument timbre missing key:" << instrument_name_string << "attack_curve";
		ret = 1;
		*p_envelope_attack_curve = ChiptuneEnvelopeCurveLinear;
	}else{
		*p_envelope_attack_curve = GetEnvelopeCurveCode(m_settings.value("attack_curve").toString());
		if(0 > *p_envelope_attack_curve){
			qWarning() << "Instrument timbre invalid key:"
					   << instrument_name_string << "attack_curve";
			ret = 1;
			*p_envelope_attack_curve = ChiptuneEnvelopeCurveLinear;
		}
	}
	if(false == m_settings.contains("attack_duration_ms")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "attack_duration_ms";
		ret = 1;
		*p_envelope_attack_duration_in_seconds = 0.02f;
	}else{
		*p_envelope_attack_duration_in_seconds =
				m_settings.value("attack_duration_ms").toFloat() / 1000.0f;
	}

	if(false == m_settings.contains("decay_curve")){
		qWarning() << "Instrument timbre missing key:" << instrument_name_string << "decay_curve";
		ret = 1;
		*p_envelope_decay_curve = ChiptuneEnvelopeCurveFermi;
	}else{
		*p_envelope_decay_curve = GetEnvelopeCurveCode(m_settings.value("decay_curve").toString());
		if(0 > *p_envelope_decay_curve){
			qWarning() << "Instrument timbre invalid key:"
					   << instrument_name_string << "decay_curve";
			ret = 1;
			*p_envelope_decay_curve = ChiptuneEnvelopeCurveFermi;
		}
	}
	if(false == m_settings.contains("decay_duration_ms")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "decay_duration_ms";
		ret = 1;
		*p_envelope_decay_duration_in_seconds = 0.01f;
	}else{
		*p_envelope_decay_duration_in_seconds =
				m_settings.value("decay_duration_ms").toFloat() / 1000.0f;
	}
	if(false == m_settings.contains("note_on_sustain_level")){
		qWarning() << "Instrument timbre missing key:" << instrument_name_string
				   << "note_on_sustain_level";
		ret = 1;
		*p_envelope_note_on_sustain_level = 96;
	}else{
		*p_envelope_note_on_sustain_level =
				(uint8_t)m_settings.value("note_on_sustain_level").toUInt();
	}

	if(false == m_settings.contains("release_curve")){
		qWarning() << "Instrument timbre missing key:" << instrument_name_string << "release_curve";
		ret = 1;
		*p_envelope_release_curve = ChiptuneEnvelopeCurveExponential;
	}else{
		*p_envelope_release_curve = GetEnvelopeCurveCode(m_settings.value("release_curve").toString());
		if(0 > *p_envelope_release_curve){
			qWarning() << "Instrument timbre invalid key:"
					   << instrument_name_string << "release_curve";
			ret = 1;
			*p_envelope_release_curve = ChiptuneEnvelopeCurveExponential;
		}
	}
	if(false == m_settings.contains("release_duration_ms")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "release_duration_ms";
		ret = 1;
		*p_envelope_release_duration_in_seconds = 0.03f;
	}else{
		*p_envelope_release_duration_in_seconds =
				m_settings.value("release_duration_ms").toFloat() / 1000.0f;
	}

	if(false == m_settings.contains("damper_sustain_level")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "damper_sustain_level";
		ret = 1;
		*p_envelope_damper_sustain_level = 72;
	}else{
		*p_envelope_damper_sustain_level =
				(uint8_t)m_settings.value("damper_sustain_level").toUInt();
	}
	if(false == m_settings.contains("damper_sustain_curve")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "damper_sustain_curve";
		ret = 1;
		*p_envelope_damper_sustain_curve = ChiptuneEnvelopeCurveLinear;
	}else{
		*p_envelope_damper_sustain_curve =
				GetEnvelopeCurveCode(m_settings.value("damper_sustain_curve").toString());
		if(0 > *p_envelope_damper_sustain_curve){
			qWarning() << "Instrument timbre invalid key:"
					   << instrument_name_string << "damper_sustain_curve";
			ret = 1;
			*p_envelope_damper_sustain_curve = ChiptuneEnvelopeCurveLinear;
		}
	}
	if(false == m_settings.contains("damper_sustain_duration_ms")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "damper_sustain_duration_ms";
		ret = 1;
		*p_envelope_damper_sustain_duration_in_seconds = 8.0f;
	}else{
		*p_envelope_damper_sustain_duration_in_seconds =
				m_settings.value("damper_sustain_duration_ms").toFloat() / 1000.0f;
	}
	m_settings.endGroup();

	return ret;
}

/******************************************************************************/
int InstrumentTimbreIniFile::ReadTimbres(QMap<int8_t, instrument_timbre_t> * const p_timbres)
{
	if(false == QFileInfo::exists(m_settings.fileName())){
		return -1;
	}

	p_timbres->clear();

	int ret = 0;
	QStringList const instrument_name_strings = m_settings.childGroups();
	for(QString const& instrument_name_string : instrument_name_strings){
		int8_t const instrument_code = GetInstrumentCode(instrument_name_string);
		if(0 > instrument_code){
			qWarning() << "Instrument timbre unknown instrument:" << instrument_name_string;
			ret = 1;
			continue;
		}

		instrument_timbre_t timbre = {};
		int const read_ret = ReadTimbre(instrument_code,
										&timbre.waveform,
										&timbre.envelope_attack_curve,
										&timbre.envelope_attack_duration_in_seconds,
										&timbre.envelope_decay_curve,
										&timbre.envelope_decay_duration_in_seconds,
										&timbre.envelope_note_on_sustain_level,
										&timbre.envelope_release_curve,
										&timbre.envelope_release_duration_in_seconds,
										&timbre.envelope_damper_sustain_level,
										&timbre.envelope_damper_sustain_curve,
										&timbre.envelope_damper_sustain_duration_in_seconds);
		if(1 == read_ret){
			ret = 1;
		}else if(0 > read_ret){
			return read_ret;
		}
		p_timbres->insert(instrument_code, timbre);
	}

	return ret;
}

/******************************************************************************/
void InstrumentTimbreIniFile::WriteTimbre(int8_t const instrument_code, int8_t const waveform,
										  int8_t const envelope_attack_curve,
										  float const envelope_attack_duration_in_seconds,
										  int8_t const envelope_decay_curve,
										  float const envelope_decay_duration_in_seconds,
										  uint8_t const envelope_note_on_sustain_level,
										  int8_t const envelope_release_curve,
										  float const envelope_release_duration_in_seconds,
										  uint8_t const envelope_damper_sustain_level,
										  int8_t const envelope_damper_sustain_curve,
										  float const envelope_damper_sustain_duration_in_seconds)
{
	m_settings.beginGroup(QString::fromLatin1(GetInstrumentNameString(instrument_code)));
	m_settings.setValue("waveform", GetWaveformNameString(waveform));
	if(QStringLiteral("square") == GetWaveformNameString(waveform)){
		m_settings.setValue("dutycycle", GetDutycycleNameString(waveform));
	}
	m_settings.setValue("attack_curve", GetEnvelopeCurveNameString(envelope_attack_curve));
	m_settings.setValue("attack_duration_ms", 1000.0f * envelope_attack_duration_in_seconds);
	m_settings.setValue("decay_curve", GetEnvelopeCurveNameString(envelope_decay_curve));
	m_settings.setValue("decay_duration_ms", 1000.0f * envelope_decay_duration_in_seconds);
	m_settings.setValue("note_on_sustain_level", (uint)envelope_note_on_sustain_level);
	m_settings.setValue("release_curve", GetEnvelopeCurveNameString(envelope_release_curve));
	m_settings.setValue("release_duration_ms", 1000.0f * envelope_release_duration_in_seconds);
	m_settings.setValue("damper_sustain_level", (uint)envelope_damper_sustain_level);
	m_settings.setValue("damper_sustain_curve",
						GetEnvelopeCurveNameString(envelope_damper_sustain_curve));
	m_settings.setValue("damper_sustain_duration_ms",
						1000.0f * envelope_damper_sustain_duration_in_seconds);
	m_settings.endGroup();
	m_settings.sync();
}
