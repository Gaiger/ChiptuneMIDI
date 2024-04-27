#ifndef _CHIPTUNE_MIDI_DEFINE_H_
#define _CHIPTUNE_MIDI_DEFINE_H_

#define MIDI_DEFAULT_TEMPO							(120.0)
#define MIDI_DEFAULT_RESOLUTION						(960)

#define MIDI_MAX_CHANNEL_NUMBER						(16)
#define MIDI_SEVEN_BITS_CENTER_VALUE				(64)
#define MIDI_FOURTEEN_BITS_CENTER_VALUE				(0x2000)
#define MIDI_DEFAULT_PITCH_WHEEL_BEND_RANGE_IN_SEMITONES	\
															(2 * 2)

/**********************************************************************************/
#define MIDI_MESSAGE_NOTE_OFF						(0x80)
#define MIDI_MESSAGE_NOTE_ON						(0x90)
#define MIDI_MESSAGE_KEY_PRESSURE					(0xA0)
#define MIDI_MESSAGE_CONTROL_CHANGE					(0xB0)
#define MIDI_MESSAGE_PROGRAM_CHANGE					(0xC0)
#define MIDI_MESSAGE_CHANNEL_PRESSURE				(0xD0)
#define MIDI_MESSAGE_PITCH_WHEEL					(0xE0)
/**********************************************************************************/
//https://anotherproducer.com/online-tools-for-musicians/midi-cc-list/
#define MIDI_CC_MODULATION_WHEEL					(1)
#define MIDI_CC_BREATH_CONTROLLER					(2)
#define MIDI_CC_DATA_ENTRY_MSB						(6)
#define MIDI_CC_VOLUME								(7)
#define MIDI_CC_PAN									(10)
#define MIDI_CC_EXPRESSION							(11)

#define MIDI_CC_DATA_ENTRY_LSB						(32 + MIDI_CC_DATA_ENTRY_MSB)

#define MIDI_CC_DAMPER_PEDAL						(64)

#define MIDI_CC_REVERB_DEPTH						(91)
#define MIDI_CC_EFFECT_2_DEPTH						(92)
#define MIDI_CC_EFFECT_3_DEPTH						(93)
#define MIDI_CC_CHORUS_EFFECT						(MIDI_CC_EFFECT_3_DEPTH)
#define MIDI_CC_EFFECT_4_DEPTH						(94)
#define MIDI_CC_EFFECT_5_DEPTH						(95)

#define MIDI_CC_NRPN_LSB							(98)
#define MIDI_CC_NRPN_MSB							(99)

//https://zh.wikipedia.org/zh-tw/General_MIDI
#define MIDI_CC_RPN_LSB								(100)
#define MIDI_CC_RPN_MSB								(101)

#define MIDI_CC_RESET_ALL_CONTROLLERS				(121)

//http://www.philrees.co.uk/nrpnq.htm
#define MIDI_CC_RPN_PITCH_BEND_SENSITIVY			(0)
#define MIDI_CC_RPN_CHANNEL_FINE_TUNING				(1)
#define MIDI_CC_RPN_CHANNEL_COARSE_TUNING			(2)
#define MIDI_CC_RPN_TURNING_PROGRAM_CHANGE			(3)
#define MIDI_CC_RPN_TURNING_BANK_SELECT				(4)
#define MIDI_CC_RPN_MODULATION_DEPTH_RANGE			(5)
#define MIDI_CC_RPN_NULL						((127 << 8) + 127)

/**********************************************************************************/

#define EXPAND_ENUM(ITEM, VAL)						ITEM = VAL,

#define EXPAND_CASE_TO_STR(X, DUMMY_VAR)			case X:	return #X;

#include <stdint.h>
#ifdef __clang__
	#define MAYBE_UNUSED_FUNCTION __attribute__((unused))
#else
	#define MAYBE_UNUSED_FUNCTION
#endif

/**********************************************************************************/

#define INSTRUMENT_CODE_LIST(X)	\
	X(AcousticGrandPiano,		0) \
	X(BrightAcousticPiano,		1) \
	X(ElectricGrandPiano,		2) \
	X(HonkyTonkPiano,			3) \
	X(RhodesPiano,				4) \
	X(ChorusedPiano,			5) \
	X(Harpsichord,				6) \
	X(Clavinet,					7) \
	\
	X(Celesta,					8) \
	X(Glockenspiel,				9) \
	X(MusicBox,					10) \
	X(Vibraphone,				11) \
	X(Marimba,					12) \
	X(Xylophone,				13) \
	X(TubularBells,				14) \
	X(Dulcimer,					15) \
	\
	X(HammondOrgan,				16) \
	X(PercussiveOrgan,			17) \
	X(RockOrgan,				18) \
	X(ChurchOrgan,				19) \
	X(ReedOrgan,				20) \
	X(Accordion,				21) \
	X(Harmonica,				22) \
	X(TangoAccordion,			23) \
	\
	X(AcousticGuitarNylon,		24) \
	X(AcousticGuitarSteel,		25) \
	X(ElectricGuitarJazz,		26) \
	X(ElectricGuitarClean,		27) \
	X(ElectricGuitarMuted,		28) \
	X(OverdrivenGuitar,			29) \
	X(DistortionGuitar,			30) \
	X(GuitarHarmonics,			31) \
	\
	X(AcousticBass,				32) \
	X(ElectricBassFinger,		33) \
	X(ElectricBassPick,			34) \
	X(FretlessBass,				35) \
	X(SlapBass1,				36) \
	X(SlapBass2,				37) \
	X(SynthBass1,				38) \
	X(SynthBass2,				39) \
	\
	X(Violin,					40) \
	X(Viola,					41) \
	X(Cello,					42) \
	X(Contrabass,				43) \
	X(TremoloStrings,			44) \
	X(PizzicatoStrings,			45) \
	X(OrchestralHarp,			46) \
	X(Timpani,					47) \
	\
	X(StringEnsemble1,			48) \
	X(StringEnsemble2,			49) \
	X(SynthStrings1,			50) \
	X(SynthStrings2,			51) \
	X(ChoirAahs,				52) \
	X(VoiceOohs,				53) \
	X(SynthVoice,				54) \
	X(OrchestraHit,				55) \
	\
	X(Trumpet,					56) \
	X(Trombone,					57) \
	X(Tuba,						58) \
	X(MutedTrumpet,				59) \
	X(FrenchHorn,				60) \
	X(BrassSection,				61) \
	X(SynthBrass1,				62) \
	X(SynthBrass2,				63) \
	\
	X(SopranoSax,				64) \
	X(AltoSax,					65) \
	X(TenorSax,					66) \
	X(BaritoneSax,				67) \
	X(Oboe,						68) \
	X(EnglishHorn,				69) \
	X(Bassoon,					70) \
	X(Clarinet,					71) \
	\
	X(Piccolo,					72) \
	X(Flute,					73) \
	X(Recorder,					74) \
	X(PanFlute,					75) \
	X(BottleBlow,				76) \
	X(Shakuhachi,				77) \
	X(Whistle,					78) \
	X(Ocarina,					79) \
	\
	X(Lead1Square,				80) \
	X(Lead2Sawtooth,			81) \
	X(Lead3CalliopeLead,		82) \
	X(Lead4ChifferLead,			83) \
	X(Lead5Charang,				84) \
	X(Lead6Voice,				85) \
	X(Lead7Fifths,				86) \
	X(Lead8BrassLead,			87) \
	\
	X(Pad1NewAge,				88) \
	X(Pad2Warm,					89) \
	X(Pad3Polysynth,			90) \
	X(Pad4Choir,				91) \
	X(Pad5Bowed,				92) \
	X(Pad6Metallic,				93) \
	X(Pad7Halo,					94) \
	X(Pad8Sweep,				95) \
	\
	X(FX1Rain,					96) \
	X(FX2Soundtrack,			97) \
	X(FX3Crystal,				98) \
	X(FX4Atmosphere,			99) \
	X(FX5Brightness,			100) \
	X(FX6Goblins,				101) \
	X(FX7Echoes,				102) \
	X(FX8SciFi,					103) \
	\
	X(Sitar,					104) \
	X(Banjo,					105) \
	X(Shamisen,					106) \
	X(Koto,						107) \
	X(Kalimba,					108) \
	X(Bagpipe,					109) \
	X(Fiddle,					110) \
	X(Shana,					111) \
	\
	X(TinkleBell,				112) \
	X(Agogo,					113) \
	X(SteelDrums,				114) \
	X(Woodblock,				115) \
	X(TaikoDrum,				116) \
	X(MelodicTom,				117) \
	X(SynthDrum,				118) \
	X(ReverseCymbal,			119) \
	\
	X(GuitarFretNoise,			120) \
	X(BreathNoise,				121) \
	X(Seashore,					122) \
	X(BirdTweet,				123) \
	X(TelephoneRing,			124) \
	X(Helicopter,				125) \
	X(Applause,					126) \
	X(Gunshot,					127) \

enum INSTRUMENT_CODE
{
	INSTRUMENT_CODE_LIST(EXPAND_ENUM)
};

MAYBE_UNUSED_FUNCTION static inline char const * const get_instrument_name_string(int8_t const index)
{
	switch (index)
	{
		INSTRUMENT_CODE_LIST(EXPAND_CASE_TO_STR)
	}

	return "UNKNOWN_INSTRUMENT";
}
/**********************************************************************************/

#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL			(9)

//https://usermanuals.finalemusic.com/SongWriter2012Win/Content/PercussionMaps.htm

#define PERCUSSION_CODE_MIN							(27)
#define PERCUSSION_CODE_MAX							(93)

#define PERCUSSION_CODE_LIST(X)	\
	X(LASER,				27) \
	X(WHIP,					28) \
	X(SCRATCH_PUSH,			29) \
	X(SCRATCH_PULL,			30) \
	X(STICK_CLICK,			31) \
	X(METRONOME_CLICK,		32) \
	\
	X(METRONOME_BELL,		34) \
	X(BASS_DRUM,			35) \
	X(KICK_DRUM,			36) \
	X(SIDE_STICK,			37) \
	X(SNARE_DRUM_1,			38) \
	X(HAND_CLAP,			39)	\
	X(SNARE_DRUM_2,			40) \
	X(LOW_FLOOR_TOM,		41) \
	X(CLOSED_HI_HAT,		42) \
	X(HIGH_FLOOR_TOM,		43) \
	X(PADEL_HI_HAT,			44) \
	X(LOW_TOM,				45) \
	X(OPEN_HI_HAT,			46) \
	X(LOW_MID_TOM,			47) \
	X(HIGH_MID_TOM,			48) \
	X(CRASH_CYMBAL_1,		49) \
	X(HIGH_TOM,				50) \
	X(RIDE_CYMBAL_1,		51) \
	X(CHINESE_CYMBAL,		52) \
	X(RIDE_BELL,			53) \
	X(TAMBOURINE,			54) \
	X(SPLASH_CYMBAL,		55) \
	X(COWBELL,				56) \
	X(CRASH_CYMBAL_2,		57) \
	X(VIBRASLAP,			58) \
	X(RIDE_CYMBAL_2,		59) \
	X(HIGH_BONGO,			60) \
	X(LOW_BONGO,			61) \
	X(CONGA_DEAD_STROKE,	62) \
	X(CONGA,				63) \
	X(TUMBA,				64) \
	X(HIGH_TIMBALE,			65) \
	X(LOW_TIMBALE,			66) \
	X(HIGH_AGOGO,			67) \
	X(LOW_AGOGO,			68) \
	X(CABASA,				69) \
	X(MARACAS,				70) \
	X(WHISTLE_SHORT,		71) \
	X(WHISTLE_LONG,			72) \
	X(GUIRO_SHORT,			73) \
	X(GUIRO_LONG,			74) \
	X(CLAVES,				75) \
	X(HIGH_WOODBLOCK,		76) \
	X(LOW_WOODBLOCK,		77) \
	X(CUICA_HIGH,			78) \
	X(CUICA_LOW,			79) \
	X(TRIANGLE_MUTE,		80) \
	X(TRIANGLE_OPEN,		81) \
	X(SHAKER,				82) \
	X(SLEIGH_BELL,			83) \
	X(BELL_TREE,			84) \
	X(CASTANETS,			85) \
	X(SURDU_DEAD_STROKE,	86) \
	X(SURDU,				87) \
	\
	X(SNARE_DRUM_ROD,		91) \
	X(OCEAN_DRUM,			92) \
	X(SNARE_DRUM_BRUSH,		93) \


enum PERCUSSION_CODE
{
	PERCUSSION_CODE_LIST(EXPAND_ENUM)
};

/**********************************************************************************/

MAYBE_UNUSED_FUNCTION static inline char const * const get_percussion_name_string(int8_t const index)
{
	switch (index)
	{
		PERCUSSION_CODE_LIST(EXPAND_CASE_TO_STR)
	}

	return "UNKNOWN_PERCUSSIOM";
}

#endif // _CHIPTUNE_MIDI_DEFINE_H_
