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

#define MIDI_CC_EFFECT_1							(91)
#define MIDI_CC_EFFECT_2							(92)
#define MIDI_CC_EFFECT_3							(93)
#define MIDI_CC_EFFECT_4							(94)
#define MIDI_CC_EFFECT_5							(95)
#define MIDI_CC_REVERB_EFFECT						(MIDI_CC_EFFECT_1)
#define MIDI_CC_CHORUS_EFFECT						(MIDI_CC_EFFECT_3)
#define MIDI_CC_DETUNE_EFFECT						(MIDI_CC_EFFECT_4)

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

#define MIDI_INSTRUMENT_CODE_LIST(X)	\
	X(AcousticGrandPiano,		0) \
	X(BrightAcousticPiano,		1) \
	X(ElectricGrandPiano,		2) \
	X(HonkyTonkPiano,			3) \
	X(ElectricPiano1,			4) \
	X(ElectricPiano2,			5) \
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
	X(BlownBottle,				76) \
	X(Shakuhachi,				77) \
	X(Whistle,					78) \
	X(Ocarina,					79) \
	\
	X(Lead1Square,				80) \
	X(Lead2Sawtooth,			81) \
	X(Lead3Calliope,			82) \
	X(Lead4Chiffer,				83) \
	X(Lead5Charang,				84) \
	X(Lead6Voice,				85) \
	X(Lead7Fifths,				86) \
	X(Lead8BassLead,			87) \
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
	X(Shanai,					111) \
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

/**********************************************************************************/

#define MIDI_PERCUSSION_CHANNEL 					(9)

//https://usermanuals.finalemusic.com/SongWriter2012Win/Content/PercussionMaps.htm

#define MIDI_PERCUSSION_KEY_MAP_MIN					(27)
#define MIDI_PERCUSSION_KEY_MAP_MAX					(87)

/* 35–81 ... */
//https://musescore.org/sites/musescore.org/files/General%20MIDI%20Standard%20Percussion%20Set%20Key%20Map.pdf

#define GM1_PERCUSSION_KEY_MAP_LIST(X) \
	X(AcousticBassDrum,		35) \
	X(BassDrum1,			36) \
	X(SideStick,			37) \
	X(AcousticSnare,		38) \
	X(HandClap,				39) \
	X(ElectricSnare,		40) \
	X(LowFloorTom,			41) \
	X(ClosedHiHat,			42) \
	X(HighFloorTom,			43) \
	X(PedalHiHat,			44) \
	X(LowTom,				45) \
	X(OpenHiHat,			46) \
	X(LowMidTom,			47) \
	X(HighMidTom,			48) \
	X(CrashCymbal1,			49) \
	X(HighTom,				50) \
	X(RideCymbal1,			51) \
	X(ChineseCymbal,		52) \
	X(RideBell,				53) \
	X(Tambourine,			54) \
	X(SplashCymbal,			55) \
	X(Cowbell,				56) \
	X(CrashCymbal2,			57) \
	X(Vibraslap,			58) \
	X(RideCymbal2,			59) \
	X(HighBongo,			60) \
	X(LowBongo,				61) \
	X(MuteHighConga,		62) \
	X(OpenHighConga,		63) \
	X(LowConga,				64) \
	X(HighTimbale,			65) \
	X(LowTimbale,			66) \
	X(HighAgogo,			67) \
	X(LowAgogo,				68) \
	X(Cabasa,				69) \
	X(Maracas,				70) \
	X(ShortWhistle,			71) \
	X(LongWhistle,			72) \
	X(ShortGuiro,			73) \
	X(LongGuiro,			74) \
	X(Claves,				75) \
	X(HighWoodBlock,		76) \
	X(LowWoodBlock,			77) \
	X(MuteCuica,			78) \
	X(OpenCuica,			79) \
	X(MuteTriangle,			80) \
	X(OpenTriangle,			81) \

//https://www.voidaudio.net/percussion.html
//https://midnightmusic.com/wp-content/uploads/2012/08/GMPercussion-and-Sibelius-Drum-Map.pdf
#define GM2_PERCUSSION_EXTENSION_KEY_MAP_LIST(X) \
	X(HighQ,				27) \
	X(Slap,					28) \
	X(ScratchPush,			29) \
	X(ScratchPull,			30) \
	X(Sticks,				31) \
	X(SquareClick,			32) \
	X(MetronomeClick,		33) \
	X(MetronomeBell,		34) \
	\
	X(Shaker,				82) \
	X(JingleBell,			83) \
	X(Belltree,				84) \
	X(Castanets,			85) \
	X(MuteSurdo,			86) \
	X(OpenSurdo,			87) \

#define MIDI_PERCUSSION_KEY_MAP_LIST(X) \
	GM1_PERCUSSION_KEY_MAP_LIST(X) \
	GM2_PERCUSSION_EXTENSION_KEY_MAP_LIST(X) \



#endif // _CHIPTUNE_MIDI_DEFINE_H_
