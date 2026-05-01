#include <QRegularExpression>

#include "chiptune.h"
#include "chiptune_midi_define.h"

#include "ChiptuneMidiValues.h"
#include "TuneManager.h"

#define EXPAND_CASE(name, code) case code: return QStringLiteral(#name);

/**********************************************************************************/

QString GetRawInstrumentNameString(int const instrument_code)
{
	switch (instrument_code)
	{
		MIDI_INSTRUMENT_CODE_LIST(EXPAND_CASE)
		default:
			return QStringLiteral("Unknown");
	}
}

/**********************************************************************************/

// clazy:excludeall=use-static-qregularexpression
QString GetInstrumentNameString(int const instrument_code)
{
	if(CHIPTUNE_INSTRUMENT_NOT_SPECIFIED == instrument_code){
		return QStringLiteral("Not Specified");
	}
	if(CHIPTUNE_INSTRUMENT_UNUSED_CHANNEL == instrument_code){
		return QStringLiteral("Unused channel");
	}

	QString name = GetRawInstrumentNameString(instrument_code);

	// Step 1: CamelCase 拆開：...aB... -> ...a B...
	// 不會拆 FX 這種全大寫前綴
	name.replace(QRegularExpression(R"((?<=[a-z])(?=[A-Z]))"), " ");

	// Step 2: 字母/非數字 ↔ 數字 之間插空格
	// 例如 Lead8BassLead -> Lead 8BassLead -> Lead 8 BassLead
	name.replace(QRegularExpression(R"((?<=\D)(?=\d))"), " ");
	name.replace(QRegularExpression(R"((?<=\d)(?=[A-Za-z]))"), " ");

	// Step 3: "Lead 4 Chiff" → "Lead 4 (Chiff)"
	//        "Pad 1 New Age" → "Pad 1 (New Age)"
	//        "FX 8 Sci Fi"   → "FX 8 (Sci Fi)"
	name.replace(QRegularExpression(R"(^(\S+)\s+(\d+)\s+(.*)$)"),
				 R"(\1 \2 (\3))");

	// Step 4: 特例 Guitar / Bass
	// Acoustic Guitar Steel  -> Acoustic Guitar (steel)
	// Electric Bass Finger   -> Electric Bass (finger)
	{
		QRegularExpression reGuitarBass(
			R"(^((?:Acoustic|Electric)\s+(?:Bass|Guitar))\s+(\S.*)$)"
		);
		QRegularExpressionMatch match = reGuitarBass.match(name);
		if (match.hasMatch()) {
			const QString head = match.captured(1);
			const QString tail = match.captured(2).toLower();
			name = QStringLiteral("%1 (%2)").arg(head, tail);
		}
	}

	// Step 5: 括號內內容：全部小寫 + 特例修正
	{
		QRegularExpression reParen(R"(\(([^)]*)\))");
		QRegularExpressionMatch match = reParen.match(name);
		if (match.hasMatch()) {
			QString inner = match.captured(1).toLower();

			// 先壓成單一空白，避免 "new  age" 這種
			inner.replace(QRegularExpression(R"(\s{2,})"), " ");

			// 特例轉換：這裡用有空格的版本
			inner.replace("bass lead", "bass & lead");
			inner.replace("sci fi",   "sci-fi");
			// "new age" 本來就對，保持不動即可

			name.replace(match.captured(0),
						 QStringLiteral("(%1)").arg(inner));
		}
	}

	// Step 6: 全域再壓一次多重空白，保險
	name.replace(QRegularExpression(R"(\s{2,})"), " ");

	return name.trimmed();
}

/**********************************************************************************/

QColor GetChannelColor(int const channel_index)
{
	QColor color = QColor(Qt::GlobalColor::black);
	switch(channel_index){
	case 0:
		color = QColor(0x00, 0x00, 0x00);
		break;
	case 1:
		color = QColor(0x80, 0x00, 0x00);
		break;
	case 2:
		color = QColor(0x00, 0x80, 0x00);
		break;
	case 3:
		color = QColor(0x80, 0x80, 0x00);
		break;

	case 4:
		color = QColor(0x00, 0x00, 0x80);
		break;
	case 5:
		color = QColor(0x80, 0x00, 0x80);
		break;
	case 6:
		color = QColor(0x00, 0x80, 0x80);
		break;
	case 7:
		color = QColor(0xC0, 0xC0, 0xC0);
		break;

	case 8:
		color = QColor(0x80, 0x80, 0x80);
		break;
	case 9:
		color = QColor(0xFF, 0x00, 0x00);
		break;

	case 10:
		color = QColor(0x00, 0xFF, 0x00);
		break;
	case 11:
		color = QColor(0xFF, 0xFF, 0x00);
		break;

	case 12:
		color = QColor(0x00, 0x00, 0xFF);
		break;
	case 13:
		color = QColor(0xFF, 0x00, 0xFF);
		break;
	case 14:
		color = QColor(0x00, 0xFF, 0xFF);
		break;
	case 15:
		color = QColor(0xFF, 0xFF, 0xFF);
		break;
	default:
		break;
	}
	color.setAlpha(0xC0);
	return color;
}

/**********************************************************************************/

instrument_timbre_t GetInstrumentTimbre(int waveform,
										int envelope_attack_curve,
										float envelope_attack_duration_in_seconds,
										int envelope_decay_curve,
										float envelope_decay_duration_in_seconds,
										int envelope_note_on_sustain_level,
										int envelope_release_curve,
										float envelope_release_duration_in_seconds,
										int envelope_damper_sustain_level,
										int envelope_damper_sustain_curve,
										float envelope_damper_sustain_duration_in_seconds)
{
	return
	{
		(int8_t)waveform, //waveform

		(int8_t)envelope_attack_curve, //envelope_attack_curve
		{0, 0}, //attack_padding
		envelope_attack_duration_in_seconds,

		(int8_t)envelope_decay_curve, //envelope_decay_curve
		{0, 0, 0}, //decay_padding[3]
		envelope_decay_duration_in_seconds, //envelope_decay_duration_in_seconds

		(uint8_t)envelope_note_on_sustain_level, //envelope_note_on_sustain_level

		(int8_t)envelope_release_curve, //envelope_release_curve
		{0, 0}, //release_padding[2]
		envelope_release_duration_in_seconds, //envelope_release_duration_in_seconds

		(uint8_t)envelope_damper_sustain_level, //envelope_damper_sustain_level
		(int8_t)envelope_damper_sustain_curve, //envelope_damper_sustain_curve
		{0, 0}, //damper_sustain_padding[2]
		envelope_damper_sustain_duration_in_seconds //envelope_damper_sustain_duration_in_seconds
	};
}

/**********************************************************************************/
#define DEFAULT_WAVEFORM							(TuneManager::WaveformTriangle)
//#define DEFAULT_WAVEFORM_DUTYCYCLE					(WaveformDutyCycle50)
#define DEFAULT_ENVELOPE_ATTACK_CURVE				(TuneManager::EnvelopeCurveLinear)
#define DEFAULT_ENVELOPE_ATTACK_DURATION_IN_SECOND	(0.02f)
#define DEFAULT_ENVELOPE_DECAY_CURVE				(TuneManager::EnvelopeCurveFermi)
#define DEFAULT_ENVELOPE_DECAY_DURATION_IN_SECOND	(0.01f)
#define DEFAULT_ENVELOPE_NOTE_ON_SUSTAIN_LEVEL		(96)
#define DEFAULT_ENVELOPE_RELEASE_CURVE				(TuneManager::EnvelopeCurveExponential)
#define DEFAULT_ENVELOPE_RELEASE_DURATION_IN_SECOND	(0.03f)
#define DEFAULT_ENVELOPE_DAMPER_ON_SUSTAIN_LEVEL	(72)
#define DEFAULT_ENVELOPE_DAMPER_ON_CURVE			(TuneManager::EnvelopeCurveLinear)
#define DEFAULT_ENVELOPE_DAMPER_ON_SUSTAIN_DURATION_IN_SECOND (8.0f)

instrument_timbre_t GetDefaultInstrumentTimbre(void)
{
	return GetInstrumentTimbre(DEFAULT_WAVEFORM,
							   DEFAULT_ENVELOPE_ATTACK_CURVE,
							   DEFAULT_ENVELOPE_ATTACK_DURATION_IN_SECOND,
							   DEFAULT_ENVELOPE_DECAY_CURVE,
							   DEFAULT_ENVELOPE_DECAY_DURATION_IN_SECOND,
							   DEFAULT_ENVELOPE_NOTE_ON_SUSTAIN_LEVEL,
							   DEFAULT_ENVELOPE_RELEASE_CURVE,
							   DEFAULT_ENVELOPE_RELEASE_DURATION_IN_SECOND,
							   DEFAULT_ENVELOPE_DAMPER_ON_SUSTAIN_LEVEL,
							   DEFAULT_ENVELOPE_DAMPER_ON_CURVE,
							   DEFAULT_ENVELOPE_DAMPER_ON_SUSTAIN_DURATION_IN_SECOND);
}
