#include <cstdint>
#include <cstring>
#include <cstdio>

#include <QByteArray>
#include <QFileInfo>
#include <QString>

#include "QMidiFile.h"
#include "mid_reader.h"

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
		if(MidEventTypeMessage != p_event->type){
			return MID_RESULT_ERROR_EVENT;
		}
		return (message == p_event->message) ? MID_RESULT_OK : MID_RESULT_ERROR_EVENT;
	}

	if((QMidiEvent::Meta == p_midi_event->type()) && (QMidiEvent::Tempo == p_midi_event->number())){
		QByteArray const data = p_midi_event->data();
		uint32_t us_per_quarter;

		if((MidEventTypeTempo != p_event->type) || (3 != data.size())){
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

		if((MidEventTypeTimeSignature != p_event->type) || (4 != data.size())){
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

		if(MidEventTypeMeta != p_event->type){
			return MID_RESULT_ERROR_EVENT;
		}
		if(((uint8_t)p_midi_event->number() != p_event->meta.number)
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

		if(MidEventTypeSysex != p_event->type){
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
int main(int const argc, char **p_argv)
{
	if(1 >= argc){
		std::printf("no argrument\r\n");
		return -1;
	}

	char const * const p_file_name = p_argv[1];
	QString const file_path = QString::fromLocal8Bit(p_file_name);
	QFileInfo const file_info(file_path);
	QByteArray const display_file_name = file_info.fileName().toLocal8Bit();

	std::printf("file_name = %s\r\n", display_file_name.constData());

	QMidiFile midi_file;
	if(false == midi_file.load(file_path)){
		std::printf("QMidiFile load failed\r\n");
		return -2;
	}

	QList<QMidiEvent*> const midi_events = midi_file.events();

	mid_song_t song;
	mid_song_init(&song);
	if(0 != mid_song_load(&song, p_file_name)){
		std::printf("mid_song_load failed\r\n");
		mid_song_close(&song);
		return -3;
	}

	if(midi_file.fileFormat() != song.file_format){
		std::printf("file_format DIFF q=%d c=%d\r\n", midi_file.fileFormat(), song.file_format);
		mid_song_close(&song);
		return -4;
	}

	if(midi_file.tracks().size() != song.track_count){
		std::printf("track_count DIFF q=%lld c=%d\r\n", (long long)midi_file.tracks().size(), song.track_count);
		mid_song_close(&song);
		return -5;
	}

	if(midi_file.resolution() != song.resolution){
		std::printf("resolution DIFF q=%d c=%d\r\n", midi_file.resolution(), song.resolution);
		mid_song_close(&song);
		return -6;
	}

	if(to_mid_division_type(midi_file.divisionType()) != song.division_type){
		std::printf(
			"division_type DIFF q=%d c=%d\r\n",
			(int)to_mid_division_type(midi_file.divisionType()),
			(int)song.division_type);
		mid_song_close(&song);
		return -7;
	}

	if(midi_events.size() != (int)song.event_count){
		std::printf("qmidi_event_count = %lld\r\n", (long long)midi_events.size());
		std::printf("draft_event_count = %lld\r\n", (long long)song.event_count);
		mid_song_close(&song);
		return -8;
	}

	std::printf("event_count = %lld\r\n", (long long)song.event_count);

	for(size_t i = 0; i < song.event_count; i++){
		QMidiEvent * const p_midi_event = midi_events.at((int)i);
		mid_event_t * const p_event = &song.event_array[i];

		if(((uint32_t)p_midi_event->tick() != p_event->tick)
		|| ((uint16_t)p_midi_event->track() != p_event->track)){
			std::printf("event[%05lld] header DIFF\r\n", (long long)i);
			mid_song_close(&song);
			return -9;
		}

		if(0 != compare_qmidi_event_to_mid_event(p_midi_event, p_event)){
			std::printf(
				"event[%05lld] payload DIFF q_type=%d q_number=%d c_type=%d\r\n",
				(long long)i,
				(int)p_midi_event->type(),
				(int)p_midi_event->number(),
				(int)p_event->type);
			mid_song_close(&song);
			return -10;
		}
	}

	std::printf("identical to QMidiFile result\r\n");

	mid_song_close(&song);

	return 0;
}
