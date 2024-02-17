#ifndef _CHIPTUNE_MIDI_DEFINE_H_
#define _CHIPTUNE_MIDI_DEFINE_H_


#define MIDI_MAX_CHANNEL_NUMBER						(16)
#define MIDI_CC_CENTER_VALUE						(64)
#define MIDI_PITCH_WHEEL_CENTER						(0x2000)
#define MIDI_DEFAULT_PITCH_WHEEL_BEND_RANGE_IN_SEMITONES	\
															(2 * 2)

/**********************************************************************************/

//https://anotherproducer.com/online-tools-for-musicians/midi-cc-list/
#define MIDI_CC_MODULATION_WHEEL					(1)

#define MIDI_CC_DATA_ENTRY_MSB						(6)
#define MIDI_CC_VOLUME								(7)
#define MIDI_CC_PAN									(10)
#define MIDI_CC_EXPRESSION							(11)

#define MIDI_CC_DATA_ENTRY_LSB						(32 + MIDI_CC_DATA_ENTRY_MSB)

#define MIDI_CC_DAMPER_PEDAL						(64)

#define MIDI_CC_EFFECT_1_DEPTH						(91)
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

#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL_0		(9)
#define MIDI_PERCUSSION_INSTRUMENT_CHANNEL_1		(10)

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

#define EXPAND_ENUM(ITEM, VAL)						ITEM = VAL,

enum PERCUSSION_CODE
{
	PERCUSSION_CODE_LIST(EXPAND_ENUM)
};

/**********************************************************************************/

#define EXPAND_CASE_TO_STR(X, DUMMY_VAR)			case X:	return #X;

#include <stdint.h>

static char const * const get_percussion_name_string(int8_t const index)
{
	switch (index)
	{
		PERCUSSION_CODE_LIST(EXPAND_CASE_TO_STR)
	}

	return "NOT_IMPLEMENTED";
}

#endif // _CHIPTUNE_MIDI_DEFINE_H_
