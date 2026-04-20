#include <cstdint>
#include <cstring>
#include <cstdio>

#include <QByteArray>
#include <QFileInfo>
#include <QString>

#include "QMidiFile.h"
#include "mid_reader.h"
#include "qt/MidSong.h"

/******************************************************************************/
static mid_division_type_t to_mid_division_type(QMidiFile::DivisionType const type)
{
	switch(type){
	case QMidiFile::PPQ:
		return MidDivisionTypePpq;
	case QMidiFile::SMPTE24:
		return MidDivisionTypeSmpte24;
	case QMidiFile::SMPTE25:
		return MidDivisionTypeSmpte25;
	case QMidiFile::SMPTE30DROP:
		return MidDivisionTypeSmpte30Drop;
	case QMidiFile::SMPTE30:
		return MidDivisionTypeSmpte30;
	default:
		return MidDivisionTypeInvalid;
	}
}

/******************************************************************************/
static int compare_qmidi_event_to_mid_event(
	QMidiEvent *p_midi_event,
	mid_event_t *p_event)
{
	uint32_t const message = p_midi_event->message();

	if(0 != message){
		if(MidEventTypeMessage != p_event->event_type){
			return MID_RESULT_ERROR_EVENT;
		}
		return (message == p_event->message) ? MID_RESULT_OK : MID_RESULT_ERROR_EVENT;
	}

	if((QMidiEvent::Meta == p_midi_event->type()) && (QMidiEvent::Tempo == p_midi_event->number())){
		QByteArray const data = p_midi_event->data();
		uint32_t us_per_quarter;

		if((MidEventTypeTempo != p_event->event_type) || (3 != data.size())){
			return MID_RESULT_ERROR_EVENT;
		}
		us_per_quarter =
			((uint32_t)(unsigned char)data[0] << 16)
		  | ((uint32_t)(unsigned char)data[1] << 8)
		  | (uint32_t)(unsigned char)data[2];
		return (us_per_quarter == p_event->microseconds_per_quarter_note) ? MID_RESULT_OK : MID_RESULT_ERROR_EVENT;
	}

	if((QMidiEvent::Meta == p_midi_event->type()) && (QMidiEvent::TimeSignature == p_midi_event->number())){
		QByteArray const data = p_midi_event->data();

		if((MidEventTypeTimeSignature != p_event->event_type) || (4 != data.size())){
			return MID_RESULT_ERROR_EVENT;
		}
		return (((uint8_t)data[0] == p_event->time_signature.numerator)
			&& ((uint8_t)(1 << (uint8_t)data[1]) == p_event->time_signature.denominator)
			&& ((uint8_t)data[2] == p_event->time_signature.clocks_per_click)
			&& ((uint8_t)data[3] == p_event->time_signature.thirtysecond_notes_per_quarter))
			? MID_RESULT_OK
			: MID_RESULT_ERROR_EVENT;
	}

	if((QMidiEvent::Meta == p_midi_event->type())){
		QByteArray const data = p_midi_event->data();

		if(MidEventTypeMeta != p_event->event_type){
			return MID_RESULT_ERROR_EVENT;
		}
		if(((uint8_t)p_midi_event->number() != p_event->meta.meta_type)
		|| ((uint32_t)data.size() != p_event->meta.data_length)){
			return MID_RESULT_ERROR_EVENT;
		}
		if((0 != p_event->meta.data_length)
		&& (0 != std::memcmp(data.constData(), p_event->meta.p_data, p_event->meta.data_length))){
			return MID_RESULT_ERROR_EVENT;
		}
		return MID_RESULT_OK;
	}

	if(QMidiEvent::SysEx == p_midi_event->type()){
		QByteArray const data = p_midi_event->data();

		if(MidEventTypeSysex != p_event->event_type){
			return MID_RESULT_ERROR_EVENT;
		}
		if((uint32_t)data.size() != p_event->sysex.data_length){
			return MID_RESULT_ERROR_EVENT;
		}
		if((0 != p_event->sysex.data_length)
		&& (0 != std::memcmp(data.constData(), p_event->sysex.p_data, p_event->sysex.data_length))){
			return MID_RESULT_ERROR_EVENT;
		}
		return MID_RESULT_OK;
	}

	return MID_RESULT_OK;
}

/******************************************************************************/
static int compare_qmidi_event_to_mid_wrapped_event(
	QMidiEvent *p_midi_event,
	MidEvent const &event)
{
	uint32_t const message = p_midi_event->message();

	if(false == event.IsValid()){
		return MID_RESULT_ERROR_EVENT;
	}

	if(0 != message){
		if(false == event.IsMessage()){
			return MID_RESULT_ERROR_EVENT;
		}
		return (message == event.GetMessage()) ? MID_RESULT_OK : MID_RESULT_ERROR_EVENT;
	}

	if((QMidiEvent::Meta == p_midi_event->type()) && (QMidiEvent::Tempo == p_midi_event->number())){
		QByteArray const data = p_midi_event->data();
		uint32_t us_per_quarter;

		if((MidEvent::Tempo != event.GetType()) || (3 != data.size())){
			return MID_RESULT_ERROR_EVENT;
		}
		us_per_quarter =
			((uint32_t)(unsigned char)data[0] << 16)
		  | ((uint32_t)(unsigned char)data[1] << 8)
		  | (uint32_t)(unsigned char)data[2];
		return (60000000.0f / (float)us_per_quarter == event.GetTempo()) ? MID_RESULT_OK : MID_RESULT_ERROR_EVENT;
	}

	if((QMidiEvent::Meta == p_midi_event->type()) && (QMidiEvent::TimeSignature == p_midi_event->number())){
		if(MidEvent::TimeSignature != event.GetType()){
			return MID_RESULT_ERROR_EVENT;
		}
		return MID_RESULT_OK;
	}

	if(QMidiEvent::Meta == p_midi_event->type()){
		if(MidEvent::Meta != event.GetType()){
			return MID_RESULT_ERROR_EVENT;
		}
		return ((uint8_t)p_midi_event->number() == (uint8_t)event.GetNumber()) ? MID_RESULT_OK : MID_RESULT_ERROR_EVENT;
	}

	if(QMidiEvent::SysEx == p_midi_event->type()){
		return (MidEvent::SysEx == event.GetType()) ? MID_RESULT_OK : MID_RESULT_ERROR_EVENT;
	}

	return MID_RESULT_OK;
}

/******************************************************************************/
static int test_raw_mid_song(char const * const p_file_path)
{
	QString const file_path_string = QString::fromLocal8Bit(p_file_path);

	QMidiFile qmidi_file;
	if(false == qmidi_file.load(file_path_string)){
		std::printf("raw: QMidiFile load failed\r\n");
		return -2;
	}

	QList<QMidiEvent*> const midi_events = qmidi_file.events();

	mid_song_t raw_song;
	mid_song_init(&raw_song);
	if(0 != mid_song_load(&raw_song, p_file_path)){
		std::printf("raw: mid_song_load failed\r\n");
		mid_song_close(&raw_song);
		return -3;
	}

	if(qmidi_file.fileFormat() != raw_song.file_format){
		std::printf("raw file_format DIFF q=%d c=%d\r\n", qmidi_file.fileFormat(), raw_song.file_format);
		mid_song_close(&raw_song);
		return -4;
	}

	if(qmidi_file.tracks().size() != raw_song.track_count){
		std::printf("raw track_count DIFF q=%lld c=%d\r\n", (long long)qmidi_file.tracks().size(), raw_song.track_count);
		mid_song_close(&raw_song);
		return -5;
	}

	if(qmidi_file.resolution() != raw_song.resolution){
		std::printf("raw resolution DIFF q=%d c=%d\r\n", qmidi_file.resolution(), raw_song.resolution);
		mid_song_close(&raw_song);
		return -6;
	}

	if(to_mid_division_type(qmidi_file.divisionType()) != raw_song.division_type){
		std::printf(
			"raw division_type DIFF q=%d c=%d\r\n",
			(int)to_mid_division_type(qmidi_file.divisionType()),
			(int)raw_song.division_type);
		mid_song_close(&raw_song);
		return -7;
	}

	if(midi_events.size() != (int)raw_song.event_count){
		std::printf("raw qmidi_event_count = %lld\r\n", (long long)midi_events.size());
		std::printf("raw event_count = %lld\r\n", (long long)raw_song.event_count);
		mid_song_close(&raw_song);
		return -8;
	}

	std::printf("raw event_count = %lld\r\n", (long long)raw_song.event_count);

	for(size_t i = 0; i < raw_song.event_count; i++){
		QMidiEvent * const p_midi_event = midi_events.at((int)i);
		mid_event_t * const p_event = &raw_song.event_array[i];

		if(((uint32_t)p_midi_event->tick() != p_event->tick)
		|| ((uint16_t)p_midi_event->track() != p_event->track)){
			std::printf("raw event[%05lld] header DIFF\r\n", (long long)i);
			mid_song_close(&raw_song);
			return -9;
		}

		if(0 != compare_qmidi_event_to_mid_event(p_midi_event, p_event)){
			std::printf(
				"raw event[%05lld] payload DIFF q_type=%d q_number=%d c_type=%d\r\n",
				(long long)i,
				(int)p_midi_event->type(),
				(int)p_midi_event->number(),
				(int)p_event->event_type);
			mid_song_close(&raw_song);
			return -10;
		}
	}

	std::printf("raw mid_song_t is identical to QMidiFile result\r\n");

	mid_song_close(&raw_song);

	return 0;
}

/******************************************************************************/
static int test_wrapped_mid_song(char const * const p_file_path)
{
	QString const file_path_string = QString::fromLocal8Bit(p_file_path);

	QMidiFile qmidi_file;
	if(false == qmidi_file.load(file_path_string)){
		std::printf("wrapped: QMidiFile load failed\r\n");
		return -11;
	}

	QList<QMidiEvent*> const midi_events = qmidi_file.events();

	MidSong wrapped_song;
	if(false == wrapped_song.Load(file_path_string)){
		std::printf("wrapped: MidSong::Load failed\r\n");
		return -12;
	}

	if(qmidi_file.fileFormat() != wrapped_song.GetFileFormat()){
		std::printf("wrapped file_format DIFF q=%d cpp=%d\r\n", qmidi_file.fileFormat(), wrapped_song.GetFileFormat());
		return -13;
	}

	if(qmidi_file.tracks().size() != wrapped_song.GetTrackCount()){
		std::printf("wrapped track_count DIFF q=%lld cpp=%d\r\n", (long long)qmidi_file.tracks().size(), wrapped_song.GetTrackCount());
		return -14;
	}

	if(qmidi_file.resolution() != wrapped_song.GetResolution()){
		std::printf("wrapped resolution DIFF q=%d cpp=%d\r\n", qmidi_file.resolution(), wrapped_song.GetResolution());
		return -15;
	}

	if((int)to_mid_division_type(qmidi_file.divisionType()) != (int)wrapped_song.GetDivisionType()){
		std::printf(
			"wrapped division_type DIFF q=%d cpp=%d\r\n",
			(int)to_mid_division_type(qmidi_file.divisionType()),
			(int)wrapped_song.GetDivisionType());
		return -16;
	}

	if(midi_events.size() != wrapped_song.GetEventCount()){
		std::printf("wrapped qmidi_event_count = %lld\r\n", (long long)midi_events.size());
		std::printf("wrapped_event_count = %d\r\n", wrapped_song.GetEventCount());
		return -17;
	}

	for(int i = 0; i < wrapped_song.GetEventCount(); i++){
		QMidiEvent * const p_midi_event = midi_events.at(i);
		MidEvent const event = wrapped_song.GetEvent(i);

		if(((uint32_t)p_midi_event->tick() != event.GetTick())
		|| ((uint16_t)p_midi_event->track() != event.GetTrack())){
			std::printf("wrapped event[%05d] header DIFF\r\n", i);
			return -18;
		}

		if(0 != compare_qmidi_event_to_mid_wrapped_event(p_midi_event, event)){
			std::printf(
				"wrapped event[%05d] payload DIFF q_type=%d q_number=%d cpp_type=%d\r\n",
				i,
				(int)p_midi_event->type(),
				(int)p_midi_event->number(),
				(int)event.GetType());
			return -19;
		}
	}

	std::printf("MidSong is identical to QMidiFile result\r\n");

	return 0;
}

/******************************************************************************/
int main(int const argc, char **p_argv)
{
	if(1 >= argc){
		std::printf("no argrument\r\n");
		return -1;
	}

	char const * const p_file_path = p_argv[1];
	QFileInfo const file_info(QString::fromLocal8Bit(p_file_path));
	QByteArray const display_file_name = file_info.fileName().toLocal8Bit();

	std::printf("file_name = %s\r\n", display_file_name.constData());

	int ret = test_raw_mid_song(p_file_path);
	if(0 != ret){
		return ret;
	}

	ret = test_wrapped_mid_song(p_file_path);
	if(0 != ret){
		return ret;
	}

	return 0;
}
