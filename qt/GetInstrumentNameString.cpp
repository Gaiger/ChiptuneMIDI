#include "GetInstrumentNameString.h"

//https://zh.wikipedia.org/wiki/General_MIDI?utm_source=chatgpt.com
//https://fmslogo.sourceforge.io/manual/midi-instrument.html

//---- MIDI Instrument List (GM 0–127) ----
#define INSTRUMENT_LIST(X) \
	X(0,  "Acoustic Grand Piano") \
	X(1,  "Bright Acoustic Piano") \
	X(2,  "Electric Grand Piano") \
	X(3,  "Honky-tonk Piano") \
	X(4,  "Electric Piano 1") \
	X(5,  "Electric Piano 2") \
	X(6,  "Harpsichord") \
	X(7,  "Clavinet") \
	X(8,  "Celesta") \
	X(9,  "Glockenspiel") \
	X(10, "Music Box") \
	X(11, "Vibraphone") \
	X(12, "Marimba") \
	X(13, "Xylophone") \
	X(14, "Tubular Bells") \
	X(15, "Dulcimer") \
	X(16, "Hammond Organ") \
	X(17, "Percussive Organ") \
	X(18, "Rock Organ") \
	X(19, "Church Organ") \
	X(20, "Reed Organ") \
	X(21, "Accordion") \
	X(22, "Harmonica") \
	X(23, "Tango Accordion") \
	X(24, "Acoustic Guitar (nylon)") \
	X(25, "Acoustic Guitar (steel)") \
	X(26, "Electric Guitar (jazz)") \
	X(27, "Electric Guitar (clean)") \
	X(28, "Electric Guitar (muted)") \
	X(29, "Overdriven Guitar") \
	X(30, "Distortion Guitar") \
	X(31, "Guitar Harmonics") \
	X(32, "Acoustic Bass") \
	X(33, "Electric Bass (finger)") \
	X(34, "Electric Bass (pick)") \
	X(35, "Fretless Bass") \
	X(36, "Slap Bass 1") \
	X(37, "Slap Bass 2") \
	X(38, "Synth Bass 1") \
	X(39, "Synth Bass 2") \
	X(40, "Violin") \
	X(41, "Viola") \
	X(42, "Cello") \
	X(43, "Contrabass") \
	X(44, "Tremolo Strings") \
	X(45, "Pizzicato Strings") \
	X(46, "Orchestral Harp") \
	X(47, "Timpani") \
	X(48, "String Ensemble 1") \
	X(49, "String Ensemble 2") \
	X(50, "Synth Strings 1") \
	X(51, "Synth Strings 2") \
	X(52, "Choir Aahs") \
	X(53, "Voice Oohs") \
	X(54, "Synth Voice") \
	X(55, "Orchestra Hit") \
	X(56, "Trumpet") \
	X(57, "Trombone") \
	X(58, "Tuba") \
	X(59, "Muted Trumpet") \
	X(60, "French Horn") \
	X(61, "Brass Section") \
	X(62, "Synth Brass 1") \
	X(63, "Synth Brass 2") \
	X(64, "Soprano Sax") \
	X(65, "Alto Sax") \
	X(66, "Tenor Sax") \
	X(67, "Baritone Sax") \
	X(68, "Oboe") \
	X(69, "English Horn") \
	X(70, "Bassoon") \
	X(71, "Clarinet") \
	X(72, "Piccolo") \
	X(73, "Flute") \
	X(74, "Recorder") \
	X(75, "Pan Flute") \
	X(76, "Blown Bottle") \
	X(77, "Shakuhachi") \
	X(78, "Whistle") \
	X(79, "Ocarina") \
	X(80, "Lead 1 (square)") \
	X(81, "Lead 2 (sawtooth)") \
	X(82, "Lead 3 (calliope)") \
	X(83, "Lead 4 (chiffer)") \
	X(84, "Lead 5 (charang)") \
	X(85, "Lead 6 (voice)") \
	X(86, "Lead 7 (fifths)") \
	X(87, "Lead 8 (bass + lead)") \
	X(88, "Pad 1 (new age)") \
	X(89, "Pad 2 (warm)") \
	X(90, "Pad 3 (polysynth)") \
	X(91, "Pad 4 (choir)") \
	X(92, "Pad 5 (bowed)") \
	X(93, "Pad 6 (metallic)") \
	X(94, "Pad 7 (halo)") \
	X(95, "Pad 8 (sweep)") \
	X(96, "FX 1 (rain)") \
	X(97, "FX 2 (soundtrack)") \
	X(98, "FX 3 (crystal)") \
	X(99, "FX 4 (atmosphere)") \
	X(100, "FX 5 (brightness)") \
	X(101, "FX 6 (goblins)") \
	X(102, "FX 7 (echoes)") \
	X(103, "FX 8 (sci-fi)") \
	X(104, "Sitar") \
	X(105, "Banjo") \
	X(106, "Shamisen") \
	X(107, "Koto") \
	X(108, "Kalimba") \
	X(109, "Bagpipe") \
	X(110, "Fiddle") \
	X(111, "Shanai") \
	X(112, "Tinkle Bell") \
	X(113, "Agogo") \
	X(114, "Steel Drums") \
	X(115, "Woodblock") \
	X(116, "Taiko Drum") \
	X(117, "Melodic Tom") \
	X(118, "Synth Drum") \
	X(119, "Reverse Cymbal") \
	X(120, "Guitar Fret Noise") \
	X(121, "Breath Noise") \
	X(122, "Seashore") \
	X(123, "Bird Tweet") \
	X(124, "Telephone Ring") \
	X(125, "Helicopter") \
	X(126, "Applause") \
	X(127, "Gunshot")

#define EXPAND_CASE(code, name) case code: return QStringLiteral(name);

QString GetInstrumentNameString(int instrument_code)
{
	switch (instrument_code)
	{
		INSTRUMENT_LIST(EXPAND_CASE)
		default:
			return QStringLiteral("Not Specified");
	}
}

/**********************************************************************************/

QColor GetChannelColor(int channel_index)
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
