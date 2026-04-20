/* Written by Codex. */
#include "MidSong.h"

#include <QByteArray>

#include <limits.h>
#include <stdlib.h>

#include "mid_reader/mid_reader.h"

/******************************************************************************/
static uint8_t get_status_byte(uint32_t const message)
{
	return (uint8_t)(message & 0xFF);
}

/******************************************************************************/
static uint8_t get_data1_byte(uint32_t const message)
{
	return (uint8_t)((message >> 8) & 0x7F);
}

/******************************************************************************/
static uint8_t get_data2_byte(uint32_t const message)
{
	return (uint8_t)((message >> 16) & 0x7F);
}

/******************************************************************************/
MidEvent::MidEvent(mid_event_t const *p_event)
	: m_p_event(p_event)
{
}

/******************************************************************************/
bool MidEvent::IsValid(void) const
{
	return (nullptr != m_p_event);
}

/******************************************************************************/
bool MidEvent::IsMessage(void) const
{
	if(nullptr == m_p_event){
		return false;
	}

	if(MidEventTypeMessage != m_p_event->event_type){
		return false;
	}

	return true;
}

/******************************************************************************/
MidEvent::Type MidEvent::GetType(void) const
{
	uint8_t status;

	if(nullptr == m_p_event){
		return Invalid;
	}

	switch(m_p_event->event_type){
	case MidEventTypeTempo:
		return Tempo;
	case MidEventTypeTimeSignature:
		return TimeSignature;
	case MidEventTypeMeta:
		return Meta;
	case MidEventTypeSysex:
		return SysEx;
	case MidEventTypeMessage:
		status = get_status_byte(m_p_event->message);
		switch(status & 0xF0){
		case 0x80:
			return NoteOff;
		case 0x90:
			return NoteOn;
		case 0xA0:
			return KeyPressure;
		case 0xB0:
			return ControlChange;
		case 0xC0:
			return ProgramChange;
		case 0xD0:
			return ChannelPressure;
		case 0xE0:
			return PitchWheel;
		default:
			return Invalid;
		}
	default:
		return Invalid;
	}
}

/******************************************************************************/
uint32_t MidEvent::GetTick(void) const
{
	if(nullptr == m_p_event){
		return 0;
	}
	return m_p_event->tick;
}

/******************************************************************************/
uint16_t MidEvent::GetTrack(void) const
{
	if(nullptr == m_p_event){
		return 0;
	}
	return m_p_event->track;
}

/******************************************************************************/
uint32_t MidEvent::GetMessage(void) const
{
	if(nullptr == m_p_event){
		return 0;
	}

	if(false == IsMessage()){
		return 0;
	}
	return m_p_event->message;
}

/******************************************************************************/
int MidEvent::GetVoice(void) const
{
	if(nullptr == m_p_event){
		return -1;
	}

	if(false == IsMessage()){
		return -1;
	}
	return (int)(get_status_byte(m_p_event->message) & 0x0F);
}

/******************************************************************************/
int MidEvent::GetNote(void) const
{
	if(nullptr == m_p_event){
		return -1;
	}

	switch(GetType()){
	case NoteOn:
	case NoteOff:
	case KeyPressure:
		return (int)get_data1_byte(m_p_event->message);
	default:
		return -1;
	}
}

/******************************************************************************/
int MidEvent::GetVelocity(void) const
{
	if(nullptr == m_p_event){
		return -1;
	}

	switch(GetType()){
	case NoteOn:
	case NoteOff:
		return (int)get_data2_byte(m_p_event->message);
	default:
		return -1;
	}
}

/******************************************************************************/
int MidEvent::GetNumber(void) const
{
	if(nullptr == m_p_event){
		return -1;
	}

	switch(GetType()){
	case ControlChange:
	case ProgramChange:
		return (int)get_data1_byte(m_p_event->message);
	case Meta:
		return (int)m_p_event->meta.meta_type;
	default:
		return -1;
	}
}

/******************************************************************************/
int MidEvent::GetValue(void) const
{
	if(nullptr == m_p_event){
		return -1;
	}

	switch(GetType()){
	case ControlChange:
		return (int)get_data2_byte(m_p_event->message);
	case PitchWheel:
		return (int)(((uint16_t)get_data2_byte(m_p_event->message) << 7) | (uint16_t)get_data1_byte(m_p_event->message));
	default:
		return -1;
	}
}

/******************************************************************************/
int MidEvent::GetAmount(void) const
{
	if(nullptr == m_p_event){
		return -1;
	}

	switch(GetType()){
	case KeyPressure:
		return (int)get_data2_byte(m_p_event->message);
	case ChannelPressure:
		return (int)get_data1_byte(m_p_event->message);
	default:
		return -1;
	}
}

/******************************************************************************/
float MidEvent::GetTempo(void) const
{
	if(nullptr == m_p_event){
		return -1.0f;
	}

	if(Tempo != GetType()){
		return -1.0f;
	}
	return 60000000.0f / (float)m_p_event->microseconds_per_quarter_note;
}

/******************************************************************************/
MidSong::MidSong(void)
	: m_p_song((mid_song_t*)malloc(sizeof(mid_song_t))),
	  m_is_loaded(false)
{
	if(nullptr != m_p_song){
		mid_song_init(m_p_song);
	}
}

/******************************************************************************/
MidSong::~MidSong(void)
{
	if(nullptr != m_p_song){
		mid_song_close(m_p_song);
		free(m_p_song);
		m_p_song = nullptr;
	}
}

/******************************************************************************/
bool MidSong::Load(QString const &file_name)
{
	QByteArray const encoded_name = file_name.toLocal8Bit();

	Clear();
	if(nullptr == m_p_song){
		return false;
	}
	m_is_loaded = (MID_RESULT_OK == mid_song_load(m_p_song, encoded_name.constData()));
	return m_is_loaded;
}

/******************************************************************************/
void MidSong::Clear(void)
{
	if(nullptr != m_p_song){
		mid_song_close(m_p_song);
		mid_song_init(m_p_song);
	}
	m_is_loaded = false;
}

/******************************************************************************/
bool MidSong::IsLoaded(void) const
{
	return m_is_loaded;
}

/******************************************************************************/
int MidSong::GetFileFormat(void) const
{
	return (nullptr != m_p_song) ? m_p_song->file_format : 0;
}

/******************************************************************************/
int MidSong::GetTrackCount(void) const
{
	return (nullptr != m_p_song) ? m_p_song->track_count : 0;
}

/******************************************************************************/
int MidSong::GetResolution(void) const
{
	return (nullptr != m_p_song) ? m_p_song->resolution : 0;
}

/******************************************************************************/
MidSong::DivisionType MidSong::GetDivisionType(void) const
{
	if(nullptr == m_p_song){
		return DivisionTypeInvalid;
	}

	switch(m_p_song->division_type){
	case MidDivisionTypePpq:
		return DivisionTypePpq;
	case MidDivisionTypeSmpte24:
		return DivisionTypeSmpte24;
	case MidDivisionTypeSmpte25:
		return DivisionTypeSmpte25;
	case MidDivisionTypeSmpte30Drop:
		return DivisionTypeSmpte30Drop;
	case MidDivisionTypeSmpte30:
		return DivisionTypeSmpte30;
	default:
		return DivisionTypeInvalid;
	}
}

/******************************************************************************/
int MidSong::GetEventCount(void) const
{
	if(nullptr == m_p_song){
		return 0;
	}
	if(m_p_song->event_count > (size_t)INT_MAX){
		return INT_MAX;
	}
	return (int)m_p_song->event_count;
}

/******************************************************************************/
MidEvent MidSong::GetEvent(int const index) const
{
	if((nullptr == m_p_song) || (index < 0) || ((size_t)index >= m_p_song->event_count)){
		return MidEvent();
	}
	return MidEvent(&m_p_song->event_array[(size_t)index]);
}

/******************************************************************************/
float MidSong::TimeFromTick(uint32_t const tick) const
{
	return (nullptr != m_p_song) ? mid_song_time_from_tick(m_p_song, tick) : -1.0f;
}

/******************************************************************************/
uint32_t MidSong::TickFromTime(float const time_in_seconds) const
{
	return (nullptr != m_p_song) ? mid_song_tick_from_time(m_p_song, time_in_seconds) : 0;
}
