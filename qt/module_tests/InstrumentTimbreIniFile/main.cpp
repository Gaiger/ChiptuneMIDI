/* Written by Codex. */
#include "../../InstrumentTimbreIniFile.h"

#include "../../../chiptune.h"
#include "../../../chiptune_midi_define.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>

#include <math.h>
#include <stdint.h>

#define EXPAND_CASE_TO_STR(ENUMS_ELEMENT, VAL)					case VAL:	return #ENUMS_ELEMENT;

typedef struct _test_instrument_timbre_t
{
	bool has_instrument;

	bool has_waveform;
	bool has_dutycycle;
	bool has_attack_curve;
	bool has_attack_duration;
	bool has_decay_curve;
	bool has_decay_duration;
	bool has_note_on_sustain_level;
	bool has_release_curve;
	bool has_release_duration;
	bool has_damper_sustain_level;
	bool has_damper_sustain_curve;
	bool has_damper_sustain_duration;

	instrument_timbre_t expected_instrument_timbre;
} test_instrument_timbre_t;

static uint32_t g_test_seed = 20260429;

static int ExpectTrue(bool const condition, char const * const message)
{
	if(false == condition){
		qCritical() << message;
		return -1;
	}
	return 0;
}

/******************************************************************************/
static bool IsNear(float const lhs, float const rhs)
{
	return fabsf(lhs - rhs) < 0.0001f;
}

/******************************************************************************/
static int8_t WaveformFromIndex(int const index)
{
	switch(index % 6)
	{
	case 0:
		return ChiptuneWaveformSquareDutyCycle12_5;
	case 1:
		return ChiptuneWaveformSquareDutyCycle25;
	case 2:
		return ChiptuneWaveformSquareDutyCycle50;
	case 3:
		return ChiptuneWaveformSquareDutyCycle75;
	case 4:
		return ChiptuneWaveformTriangle;
	default:
		return ChiptuneWaveformSaw;
	}
}

/******************************************************************************/
static int8_t CurveFromIndex(int const index)
{
	switch(index % 4)
	{
	case 0:
		return ChiptuneEnvelopeCurveLinear;
	case 1:
		return ChiptuneEnvelopeCurveExponential;
	case 2:
		return ChiptuneEnvelopeCurveGaussian;
	default:
		return ChiptuneEnvelopeCurveFermi;
	}
}

/******************************************************************************/
static bool IsSquareWaveform(int8_t const waveform)
{
	return ChiptuneWaveformSquareDutyCycle12_5 == waveform
			|| ChiptuneWaveformSquareDutyCycle25 == waveform
			|| ChiptuneWaveformSquareDutyCycle50 == waveform
			|| ChiptuneWaveformSquareDutyCycle75 == waveform;
}

/******************************************************************************/
static uint32_t GetDice(int const instrument_code, int const key_index)
{
	return g_test_seed
			^ (2654435761u * (uint32_t)(instrument_code + 1)
			   + 1013904223u * (uint32_t)(key_index + 1));
}

/******************************************************************************/
static bool ShouldKeepInstrument(int const instrument_code)
{
	uint32_t const dice = g_test_seed ^ (1103515245u * (uint32_t)(instrument_code + 1) + 12345u);
	return 0 == (dice >> 30);
}

/******************************************************************************/
static bool ShouldKeepBasicKey(int const instrument_code, int const key_index)
{
	return 1 <= (GetDice(instrument_code, key_index) >> 28);
}

/******************************************************************************/
static bool ShouldKeepDamperSustainKey(int const instrument_code, int const key_index)
{
	return 1 <= (GetDice(instrument_code, key_index) >> 31);
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

#undef EXPAND_CASE_TO_STR

/******************************************************************************/
static QString GetWaveformNameString(int8_t const waveform)
{
	if(true == IsSquareWaveform(waveform)){
		return QStringLiteral("square");
	}
	switch(waveform)
	{
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
static QString GetCurveNameString(int8_t const curve)
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
static test_instrument_timbre_t MakeTestInstrumentTimbre(int const instrument_code)
{
	test_instrument_timbre_t test_timbre = {};

	test_timbre.has_instrument = ShouldKeepInstrument(instrument_code);
	test_timbre.has_waveform = ShouldKeepBasicKey(instrument_code, 0);
	test_timbre.has_dutycycle = ShouldKeepBasicKey(instrument_code, 1);
	test_timbre.has_attack_curve = ShouldKeepBasicKey(instrument_code, 2);
	test_timbre.has_attack_duration = ShouldKeepBasicKey(instrument_code, 3);
	test_timbre.has_decay_curve = ShouldKeepBasicKey(instrument_code, 4);
	test_timbre.has_decay_duration = ShouldKeepBasicKey(instrument_code, 5);
	test_timbre.has_note_on_sustain_level = ShouldKeepBasicKey(instrument_code, 6);
	test_timbre.has_release_curve = ShouldKeepBasicKey(instrument_code, 7);
	test_timbre.has_release_duration = ShouldKeepBasicKey(instrument_code, 8);
	test_timbre.has_damper_sustain_level = ShouldKeepDamperSustainKey(instrument_code, 9);
	test_timbre.has_damper_sustain_curve = ShouldKeepDamperSustainKey(instrument_code, 10);
	test_timbre.has_damper_sustain_duration = ShouldKeepDamperSustainKey(instrument_code, 11);

	test_timbre.expected_instrument_timbre.waveform = WaveformFromIndex(instrument_code);
	test_timbre.expected_instrument_timbre.envelope_attack_curve = CurveFromIndex(instrument_code);
	test_timbre.expected_instrument_timbre.envelope_attack_duration_in_seconds = 0.001f * (float)(instrument_code + 1);
	test_timbre.expected_instrument_timbre.envelope_decay_curve = CurveFromIndex(instrument_code + 1);
	test_timbre.expected_instrument_timbre.envelope_decay_duration_in_seconds = 0.002f * (float)(instrument_code + 1);
	test_timbre.expected_instrument_timbre.envelope_note_on_sustain_level = (uint8_t)(instrument_code & 0x7f);
	test_timbre.expected_instrument_timbre.envelope_release_curve = CurveFromIndex(instrument_code + 2);
	test_timbre.expected_instrument_timbre.envelope_release_duration_in_seconds = 0.003f * (float)(instrument_code + 1);
	test_timbre.expected_instrument_timbre.envelope_damper_sustain_level = (uint8_t)((127 - instrument_code) & 0x7f);
	test_timbre.expected_instrument_timbre.envelope_damper_sustain_curve = CurveFromIndex(instrument_code + 3);
	test_timbre.expected_instrument_timbre.envelope_damper_sustain_duration_in_seconds = 0.004f * (float)(instrument_code + 1);

	return test_timbre;
}

/******************************************************************************/
static void WriteTestTimbre(QSettings * const p_settings, int const instrument_code,
							test_instrument_timbre_t const& test_timbre)
{
	if(false == test_timbre.has_instrument){
		return;
	}

	p_settings->beginGroup(QString::fromLatin1(GetInstrumentNameString((int8_t)instrument_code)));
	if(true == test_timbre.has_waveform){
		p_settings->setValue("waveform", GetWaveformNameString(test_timbre.expected_instrument_timbre.waveform));
	}
	if(true == test_timbre.has_dutycycle && true == IsSquareWaveform(test_timbre.expected_instrument_timbre.waveform)){
		p_settings->setValue("dutycycle", GetDutycycleNameString(test_timbre.expected_instrument_timbre.waveform));
	}
	if(true == test_timbre.has_attack_curve){
		p_settings->setValue("attack_curve", GetCurveNameString(test_timbre.expected_instrument_timbre.envelope_attack_curve));
	}
	if(true == test_timbre.has_attack_duration){
		p_settings->setValue("attack_duration_ms",
							 1000.0f * test_timbre.expected_instrument_timbre.envelope_attack_duration_in_seconds);
	}
	if(true == test_timbre.has_decay_curve){
		p_settings->setValue("decay_curve", GetCurveNameString(test_timbre.expected_instrument_timbre.envelope_decay_curve));
	}
	if(true == test_timbre.has_decay_duration){
		p_settings->setValue("decay_duration_ms",
							 1000.0f * test_timbre.expected_instrument_timbre.envelope_decay_duration_in_seconds);
	}
	if(true == test_timbre.has_note_on_sustain_level){
		p_settings->setValue("note_on_sustain_level", (uint)test_timbre.expected_instrument_timbre.envelope_note_on_sustain_level);
	}
	if(true == test_timbre.has_release_curve){
		p_settings->setValue("release_curve", GetCurveNameString(test_timbre.expected_instrument_timbre.envelope_release_curve));
	}
	if(true == test_timbre.has_release_duration){
		p_settings->setValue("release_duration_ms",
							 1000.0f * test_timbre.expected_instrument_timbre.envelope_release_duration_in_seconds);
	}
	if(true == test_timbre.has_damper_sustain_level){
		p_settings->setValue("damper_sustain_level", (uint)test_timbre.expected_instrument_timbre.envelope_damper_sustain_level);
	}
	if(true == test_timbre.has_damper_sustain_curve){
		p_settings->setValue("damper_sustain_curve",
							 GetCurveNameString(test_timbre.expected_instrument_timbre.envelope_damper_sustain_curve));
	}
	if(true == test_timbre.has_damper_sustain_duration){
		p_settings->setValue("damper_sustain_duration_ms",
							 1000.0f * test_timbre.expected_instrument_timbre.envelope_damper_sustain_duration_in_seconds);
	}
	p_settings->endGroup();
}

/******************************************************************************/
static int TestMissingFile(QString const& ini_file_name_string)
{
	QFile::remove(ini_file_name_string);

	InstrumentTimbreIniFile reader(ini_file_name_string);
	QMap<int8_t, instrument_timbre_t> instrument_timbre_map;
	int const read_timbres_ret = reader.ReadTimbres(&instrument_timbre_map);
	if(0 != ExpectTrue(-1 == read_timbres_ret,
					   "ReadTimbres should return -1 when the INI file does not exist")){
		return -1;
	}
	return ExpectTrue(true == instrument_timbre_map.isEmpty(),
					  "ReadTimbres should leave an empty map when the INI file does not exist");
}

/******************************************************************************/
static bool HasMissingEffectiveKey(test_instrument_timbre_t const& test_timbre)
{
	bool const does_dutycycle_matter =
			true == test_timbre.has_waveform && true == IsSquareWaveform(test_timbre.expected_instrument_timbre.waveform);

	return false == test_timbre.has_waveform
			|| (true == does_dutycycle_matter && false == test_timbre.has_dutycycle)
			|| false == test_timbre.has_attack_curve
			|| false == test_timbre.has_attack_duration
			|| false == test_timbre.has_decay_curve
			|| false == test_timbre.has_decay_duration
			|| false == test_timbre.has_note_on_sustain_level
			|| false == test_timbre.has_release_curve
			|| false == test_timbre.has_release_duration
			|| false == test_timbre.has_damper_sustain_level
			|| false == test_timbre.has_damper_sustain_curve
			|| false == test_timbre.has_damper_sustain_duration;
}

/******************************************************************************/
static int TestSingleInstrumentFromMap(QMap<int8_t, instrument_timbre_t> const& instrument_timbre_map,
									   int const instrument_code,
									   test_instrument_timbre_t const& test_timbre)
{
	if(false == test_timbre.has_instrument){
		return ExpectTrue(false == instrument_timbre_map.contains((int8_t)instrument_code),
						  "ReadTimbres should not return missing random instruments");
	}

	if(0 != ExpectTrue(true == instrument_timbre_map.contains((int8_t)instrument_code),
					   "ReadTimbres should return existing random instruments")){
		return -1;
	}

	instrument_timbre_t const actual_instrument_timbre = instrument_timbre_map.value((int8_t)instrument_code);
	bool const does_dutycycle_matter =
			true == test_timbre.has_waveform && true == IsSquareWaveform(test_timbre.expected_instrument_timbre.waveform);
	int8_t const expected_waveform = false == test_timbre.has_waveform
			? ChiptuneWaveformTriangle
			: ((true == does_dutycycle_matter && false == test_timbre.has_dutycycle)
			   ? ChiptuneWaveformSquareDutyCycle50
			   : test_timbre.expected_instrument_timbre.waveform);
	if(0 != ExpectTrue(expected_waveform == actual_instrument_timbre.waveform, "missing-key waveform mismatch")){
		return -1;
	}
	if(0 != ExpectTrue((false == test_timbre.has_attack_curve
						 ? ChiptuneEnvelopeCurveLinear : test_timbre.expected_instrument_timbre.envelope_attack_curve)
						== actual_instrument_timbre.envelope_attack_curve, "missing-key attack curve mismatch")){
		return -1;
	}
	if(0 != ExpectTrue(IsNear(false == test_timbre.has_attack_duration
								? 0.02f : test_timbre.expected_instrument_timbre.envelope_attack_duration_in_seconds,
								actual_instrument_timbre.envelope_attack_duration_in_seconds),
						"missing-key attack duration mismatch")){
		return -1;
	}
	if(0 != ExpectTrue((false == test_timbre.has_decay_curve
						 ? ChiptuneEnvelopeCurveFermi : test_timbre.expected_instrument_timbre.envelope_decay_curve)
						== actual_instrument_timbre.envelope_decay_curve, "missing-key decay curve mismatch")){
		return -1;
	}
	if(0 != ExpectTrue(IsNear(false == test_timbre.has_decay_duration
								? 0.01f : test_timbre.expected_instrument_timbre.envelope_decay_duration_in_seconds,
								actual_instrument_timbre.envelope_decay_duration_in_seconds),
						"missing-key decay duration mismatch")){
		return -1;
	}
	if(0 != ExpectTrue((false == test_timbre.has_note_on_sustain_level
						 ? 96 : test_timbre.expected_instrument_timbre.envelope_note_on_sustain_level)
						== actual_instrument_timbre.envelope_note_on_sustain_level,
						"missing-key note-on sustain level mismatch")){
		return -1;
	}
	if(0 != ExpectTrue((false == test_timbre.has_release_curve
						 ? ChiptuneEnvelopeCurveExponential : test_timbre.expected_instrument_timbre.envelope_release_curve)
						== actual_instrument_timbre.envelope_release_curve, "missing-key release curve mismatch")){
		return -1;
	}
	if(0 != ExpectTrue(IsNear(false == test_timbre.has_release_duration
								? 0.03f : test_timbre.expected_instrument_timbre.envelope_release_duration_in_seconds,
								actual_instrument_timbre.envelope_release_duration_in_seconds),
						"missing-key release duration mismatch")){
		return -1;
	}
	if(0 != ExpectTrue((false == test_timbre.has_damper_sustain_level
						 ? 72 : test_timbre.expected_instrument_timbre.envelope_damper_sustain_level)
						== actual_instrument_timbre.envelope_damper_sustain_level,
						"missing-key damper sustain level mismatch")){
		return -1;
	}
	if(0 != ExpectTrue((false == test_timbre.has_damper_sustain_curve
						 ? ChiptuneEnvelopeCurveLinear : test_timbre.expected_instrument_timbre.envelope_damper_sustain_curve)
						== actual_instrument_timbre.envelope_damper_sustain_curve,
						"missing-key damper sustain curve mismatch")){
		return -1;
	}
	if(0 != ExpectTrue(IsNear(false == test_timbre.has_damper_sustain_duration
								? 8.0f : test_timbre.expected_instrument_timbre.envelope_damper_sustain_duration_in_seconds,
								actual_instrument_timbre.envelope_damper_sustain_duration_in_seconds),
						"missing-key damper sustain duration mismatch")){
		return -1;
	}

	return 0;
}

/******************************************************************************/
static int TestInstrumentTimbreIniFile(QString const& ini_file_name_string)
{
	test_instrument_timbre_t test_instrument_timbres[128];
	for(int instrument_code = 0; instrument_code < 128; instrument_code++){
		test_instrument_timbres[instrument_code] = MakeTestInstrumentTimbre(instrument_code);
	}
	int expected_instrument_timbre_count = 0;
	bool has_missing_effective_key = false;
	for(int instrument_code = 0; instrument_code < 128; instrument_code++){
		if(true == test_instrument_timbres[instrument_code].has_instrument){
			expected_instrument_timbre_count++;
			if(true == HasMissingEffectiveKey(test_instrument_timbres[instrument_code])){
				has_missing_effective_key = true;
			}
		}
	}

	{
		QSettings settings(ini_file_name_string, QSettings::IniFormat);
		for(int instrument_code = 0; instrument_code < 128; instrument_code++){
			WriteTestTimbre(&settings, instrument_code, test_instrument_timbres[instrument_code]);
		}
		settings.sync();
	}

	QMap<int8_t, instrument_timbre_t> instrument_timbre_map;
	InstrumentTimbreIniFile reader(ini_file_name_string);
	int const read_timbres_ret = reader.ReadTimbres(&instrument_timbre_map);
	if(0 != ExpectTrue((true == has_missing_effective_key ? 1 : 0) == read_timbres_ret,
					   "ReadTimbres return code should reflect missing effective keys")){
		return -1;
	}
	if(0 != ExpectTrue(expected_instrument_timbre_count == instrument_timbre_map.size(),
					   "ReadTimbres returned instrument timbre count mismatch")){
		return -1;
	}

	for(int instrument_code = 0; instrument_code < 128; instrument_code++){
		if(0 != TestSingleInstrumentFromMap(instrument_timbre_map,
											instrument_code,
											test_instrument_timbres[instrument_code])){
			qCritical() << "missing-key instrument test failed:" << instrument_code;
			return -1;
		}
	}
	return 0;
}

/******************************************************************************/
int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	Q_UNUSED(app)

	QString const ini_file_name_string = QDir::current().filePath("InstrumentTimbreIniFileTest.ini");
	if(0 != TestMissingFile(ini_file_name_string)){
		return 1;
	}
	if(0 != TestInstrumentTimbreIniFile(ini_file_name_string)){
		return 1;
	}

	return 0;
}
