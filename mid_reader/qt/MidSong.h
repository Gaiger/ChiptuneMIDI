/* Written by Codex. */
#ifndef _MIDSONG_H_
#define _MIDSONG_H_

#include <stddef.h>
#include <stdint.h>

#include <QString>

struct mid_event_t;
struct mid_song_t;

class MidEvent
{
public:
	enum Type
	{
		Invalid = -1,
		NoteOn,
		NoteOff,
		KeyPressure,
		ChannelPressure,
		ControlChange,
		ProgramChange,
		PitchWheel,
		Tempo,
		TimeSignature,
		Meta,
		SysEx,
	};

	explicit MidEvent(mid_event_t const *p_event = nullptr);

	bool IsValid(void) const;
	Type GetType(void) const;

	uint32_t GetTick(void) const;
	uint16_t GetTrack(void) const;
	uint32_t GetMessage(void) const;

	int GetVoice(void) const;
	int GetNote(void) const;
	int GetVelocity(void) const;
	int GetNumber(void) const;
	int GetValue(void) const;
	int GetAmount(void) const;
	float GetTempo(void) const;

	// int GetNumerator(void) const;
	// int GetDenominator(void) const;
	// QByteArray GetData(void) const;
	// bool IsNoteEvent(void) const;
private:
	mid_event_t const *m_p_event;
};

class MidSong
{
public:
	enum DivisionType
	{
		DivisionTypeInvalid = -1,
		DivisionTypePpq = 0,
		DivisionTypeSmpte24 = -24,
		DivisionTypeSmpte25 = -25,
		DivisionTypeSmpte30Drop = -29,
		DivisionTypeSmpte30 = -30
	};

	MidSong(void);
	~MidSong(void);

	bool Load(QString const &file_name);
	void Clear(void);
	bool IsLoaded(void) const;

	int GetFileFormat(void) const;
	int GetTrackCount(void) const;
	int GetResolution(void) const;
	DivisionType GetDivisionType(void) const;

	int GetEventCount(void) const;
	MidEvent GetEvent(int const index) const;

	float TimeFromTick(uint32_t const tick) const;
	uint32_t TickFromTime(float const time_in_seconds) const;

	// QList<int> GetTracks(void) const;
	// float BeatFromTick(uint32_t const tick) const;
	// uint32_t TickFromBeat(float const beat) const;
private:
	mid_song_t *m_p_song;
	bool m_is_loaded;
};

#endif // _MIDSONG_H_
