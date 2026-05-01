#include <QDebug>
#include <QFileInfo>
#include <QString>
#include <QStringList>

#include "chiptune.h"
#include "chiptune_midi_define.h"

#include "InstrumentTimbreIniFile.h"

#define INSTRUMENT_CODE_UNKNOWN								(-128)

#define EXPAND_IF_STR_TO_CODE(ENUMS_ELEMENT, VAL)				\
																if(name_string == QStringLiteral(#ENUMS_ELEMENT) \
																	|| name_string == GetInstrumentNameString(VAL)){ \
																	return VAL; \
																}

/******************************************************************************/
static int GetInstrumentCode(QString const& name_string)
{
	if(name_string == GetInstrumentNameString(CHIPTUNE_INSTRUMENT_NOT_SPECIFIED)){
		return CHIPTUNE_INSTRUMENT_NOT_SPECIFIED;
	}
	if(name_string == GetInstrumentNameString(CHIPTUNE_INSTRUMENT_UNUSED_CHANNEL)){
		return CHIPTUNE_INSTRUMENT_UNUSED_CHANNEL;
	}

	MIDI_INSTRUMENT_CODE_LIST(EXPAND_IF_STR_TO_CODE)
	return INSTRUMENT_CODE_UNKNOWN;
}

/******************************************************************************/
static QString GetInstrumentTimbreGroupName(int8_t const instrument_code)
{
	return GetInstrumentNameString(instrument_code);
}

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

	QString const instrument_name_string = GetInstrumentTimbreGroupName(instrument_code);
	if(false == m_settings.childGroups().contains(instrument_name_string)){
		return -2;
	}

	m_settings.beginGroup(instrument_name_string);
	instrument_timbre_t const default_instrument_timbre = GetDefaultInstrumentTimbre();
	int ret = 0;
	if(false == m_settings.contains("waveform")){
		qWarning() << "Instrument timbre missing key:" << instrument_name_string << "waveform";
		ret = 1;
		*p_waveform = default_instrument_timbre.waveform;
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
			*p_waveform = default_instrument_timbre.waveform;
		}
	}

	if(false == m_settings.contains("attack_curve")){
		qWarning() << "Instrument timbre missing key:" << instrument_name_string << "attack_curve";
		ret = 1;
		*p_envelope_attack_curve = default_instrument_timbre.envelope_attack_curve;
	}else{
		*p_envelope_attack_curve = GetEnvelopeCurveCode(m_settings.value("attack_curve").toString());
		if(0 > *p_envelope_attack_curve){
			qWarning() << "Instrument timbre invalid key:"
					   << instrument_name_string << "attack_curve";
			ret = 1;
			*p_envelope_attack_curve = default_instrument_timbre.envelope_attack_curve;
		}
	}
	if(false == m_settings.contains("attack_duration_ms")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "attack_duration_ms";
		ret = 1;
		*p_envelope_attack_duration_in_seconds = default_instrument_timbre.envelope_attack_duration_in_seconds;
	}else{
		*p_envelope_attack_duration_in_seconds =
				m_settings.value("attack_duration_ms").toFloat() / 1000.0f;
	}

	if(false == m_settings.contains("decay_curve")){
		qWarning() << "Instrument timbre missing key:" << instrument_name_string << "decay_curve";
		ret = 1;
		*p_envelope_decay_curve = default_instrument_timbre.envelope_decay_curve;
	}else{
		*p_envelope_decay_curve = GetEnvelopeCurveCode(m_settings.value("decay_curve").toString());
		if(0 > *p_envelope_decay_curve){
			qWarning() << "Instrument timbre invalid key:"
					   << instrument_name_string << "decay_curve";
			ret = 1;
			*p_envelope_decay_curve = default_instrument_timbre.envelope_decay_curve;
		}
	}
	if(false == m_settings.contains("decay_duration_ms")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "decay_duration_ms";
		ret = 1;
		*p_envelope_decay_duration_in_seconds = default_instrument_timbre.envelope_decay_duration_in_seconds;
	}else{
		*p_envelope_decay_duration_in_seconds =
				m_settings.value("decay_duration_ms").toFloat() / 1000.0f;
	}
	if(false == m_settings.contains("note_on_sustain_level")){
		qWarning() << "Instrument timbre missing key:" << instrument_name_string
				   << "note_on_sustain_level";
		ret = 1;
		*p_envelope_note_on_sustain_level = default_instrument_timbre.envelope_note_on_sustain_level;
	}else{
		*p_envelope_note_on_sustain_level =
				(uint8_t)m_settings.value("note_on_sustain_level").toUInt();
	}

	if(false == m_settings.contains("release_curve")){
		qWarning() << "Instrument timbre missing key:" << instrument_name_string << "release_curve";
		ret = 1;
		*p_envelope_release_curve = default_instrument_timbre.envelope_release_curve;
	}else{
		*p_envelope_release_curve = GetEnvelopeCurveCode(m_settings.value("release_curve").toString());
		if(0 > *p_envelope_release_curve){
			qWarning() << "Instrument timbre invalid key:"
					   << instrument_name_string << "release_curve";
			ret = 1;
			*p_envelope_release_curve = default_instrument_timbre.envelope_release_curve;
		}
	}
	if(false == m_settings.contains("release_duration_ms")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "release_duration_ms";
		ret = 1;
		*p_envelope_release_duration_in_seconds = default_instrument_timbre.envelope_release_duration_in_seconds;
	}else{
		*p_envelope_release_duration_in_seconds =
				m_settings.value("release_duration_ms").toFloat() / 1000.0f;
	}

	if(false == m_settings.contains("damper_sustain_level")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "damper_sustain_level";
		ret = 1;
		*p_envelope_damper_sustain_level = default_instrument_timbre.envelope_damper_sustain_level;
	}else{
		*p_envelope_damper_sustain_level =
				(uint8_t)m_settings.value("damper_sustain_level").toUInt();
	}
	if(false == m_settings.contains("damper_sustain_curve")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "damper_sustain_curve";
		ret = 1;
		*p_envelope_damper_sustain_curve = default_instrument_timbre.envelope_damper_sustain_curve;
	}else{
		*p_envelope_damper_sustain_curve =
				GetEnvelopeCurveCode(m_settings.value("damper_sustain_curve").toString());
		if(0 > *p_envelope_damper_sustain_curve){
			qWarning() << "Instrument timbre invalid key:"
					   << instrument_name_string << "damper_sustain_curve";
			ret = 1;
			*p_envelope_damper_sustain_curve = default_instrument_timbre.envelope_damper_sustain_curve;
		}
	}
	if(false == m_settings.contains("damper_sustain_duration_ms")){
		qWarning() << "Instrument timbre missing key:"
				   << instrument_name_string << "damper_sustain_duration_ms";
		ret = 1;
		*p_envelope_damper_sustain_duration_in_seconds =
				default_instrument_timbre.envelope_damper_sustain_duration_in_seconds;
	}else{
		*p_envelope_damper_sustain_duration_in_seconds =
				m_settings.value("damper_sustain_duration_ms").toFloat() / 1000.0f;
	}
	m_settings.endGroup();

	return ret;
}

/******************************************************************************/
int InstrumentTimbreIniFile::ReadTimbres(QMap<int8_t, instrument_timbre_t> * const p_instrument_timbres)
{
	if(false == QFileInfo::exists(m_settings.fileName())){
		return -1;
	}

	p_instrument_timbres->clear();

	int ret = 0;
	QStringList const instrument_name_strings = m_settings.childGroups();
	for(QString const& instrument_name_string : instrument_name_strings){
		int const instrument_code = GetInstrumentCode(instrument_name_string);
		if(INSTRUMENT_CODE_UNKNOWN == instrument_code){
			qWarning() << "Instrument timbre unknown instrument:" << instrument_name_string;
			ret = 1;
			continue;
		}

		instrument_timbre_t read_instrument_timbre = {};
		int const read_ret = ReadTimbre((int8_t)instrument_code,
										&read_instrument_timbre.waveform,
										&read_instrument_timbre.envelope_attack_curve,
										&read_instrument_timbre.envelope_attack_duration_in_seconds,
										&read_instrument_timbre.envelope_decay_curve,
										&read_instrument_timbre.envelope_decay_duration_in_seconds,
										&read_instrument_timbre.envelope_note_on_sustain_level,
										&read_instrument_timbre.envelope_release_curve,
										&read_instrument_timbre.envelope_release_duration_in_seconds,
										&read_instrument_timbre.envelope_damper_sustain_level,
										&read_instrument_timbre.envelope_damper_sustain_curve,
										&read_instrument_timbre.envelope_damper_sustain_duration_in_seconds);
		if(1 == read_ret){
			ret = 1;
		}else if(0 > read_ret){
			return read_ret;
		}
		p_instrument_timbres->insert((int8_t)instrument_code, read_instrument_timbre);
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
	m_settings.beginGroup(GetInstrumentTimbreGroupName(instrument_code));
	m_settings.setValue("waveform", GetWaveformNameString(waveform));
	if(QStringLiteral("square") == GetWaveformNameString(waveform)){
		m_settings.setValue("dutycycle", GetDutycycleNameString(waveform));
	}
	m_settings.setValue("attack_curve", GetEnvelopeCurveNameString(envelope_attack_curve));
	m_settings.setValue("attack_duration_ms", qRound(1000.0f * envelope_attack_duration_in_seconds));
	m_settings.setValue("decay_curve", GetEnvelopeCurveNameString(envelope_decay_curve));
	m_settings.setValue("decay_duration_ms", qRound(1000.0f * envelope_decay_duration_in_seconds));
	m_settings.setValue("note_on_sustain_level", (uint)envelope_note_on_sustain_level);
	m_settings.setValue("release_curve", GetEnvelopeCurveNameString(envelope_release_curve));
	m_settings.setValue("release_duration_ms", qRound(1000.0f * envelope_release_duration_in_seconds));
	m_settings.setValue("damper_sustain_level", (uint)envelope_damper_sustain_level);
	m_settings.setValue("damper_sustain_curve",
						GetEnvelopeCurveNameString(envelope_damper_sustain_curve));
	m_settings.setValue("damper_sustain_duration_ms",
						qRound(1000.0f * envelope_damper_sustain_duration_in_seconds));
	m_settings.endGroup();
	m_settings.sync();
}

/******************************************************************************/
void InstrumentTimbreIniFile::RemoveTimbre(int8_t const instrument_code)
{
	m_settings.beginGroup(GetInstrumentTimbreGroupName(instrument_code));
	m_settings.remove(QString());
	m_settings.endGroup();
	m_settings.sync();
}
