/* Written by Codex. */
#include "mid_reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************/
static void *grow_array(void *p_ptr, size_t const elem_size, size_t *p_capacity)
{
	size_t const new_capacity = (*p_capacity == 0) ? 64 : (*p_capacity * 2);
	void *p_new_ptr;

	p_new_ptr = realloc(p_ptr, elem_size * new_capacity);
	if (NULL == p_new_ptr) {
		return NULL;
	}
	*p_capacity = new_capacity;
	return p_new_ptr;
}

/******************************************************************************/
static int read_exact(FILE *p_fp, void *p_buffer, size_t const size)
{
	return (fread(p_buffer, 1, size, p_fp) == size) ? MID_RESULT_OK : MID_RESULT_ERROR_IO;
}

/******************************************************************************/
static int read_u16_be(FILE *p_fp, uint16_t *p_out_value)
{
	unsigned char buffer[2];

	if(0 != read_exact(p_fp, buffer, sizeof(buffer))){
		return MID_RESULT_ERROR_IO;
	}
	*p_out_value = ((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1];
	return MID_RESULT_OK;
}

/******************************************************************************/
static int read_u32_be(FILE *p_fp, uint32_t *p_out_value)
{
	unsigned char buffer[4];

	if(0 != read_exact(p_fp, buffer, sizeof(buffer))){
		return MID_RESULT_ERROR_IO;
	}
	*p_out_value = ((uint32_t)buffer[0] << 24)
				 | ((uint32_t)buffer[1] << 16)
				 | ((uint32_t)buffer[2] << 8)
				 | (uint32_t)buffer[3];
	return MID_RESULT_OK;
}

/******************************************************************************/
static int read_vlq(FILE *p_fp, uint32_t *p_out_value)
{
	unsigned char byte;
	uint32_t value;
	int i;

	value = 0;
	for(i = 0; i < 4; i++){
		if(0 != read_exact(p_fp, &byte, 1)){
			return MID_RESULT_ERROR_IO;
		}
		value = (value << 7) | (uint32_t)(byte & 0x7F);
		if(0 == (byte & 0x80)){
			*p_out_value = value;
			return MID_RESULT_OK;
		}
	}
	return MID_RESULT_ERROR_FORMAT;
}

/******************************************************************************/
static int skip_bytes(FILE *p_fp, uint32_t const size)
{
	return (0 == fseek(p_fp, (long)size, SEEK_CUR)) ? MID_RESULT_OK : MID_RESULT_ERROR_IO;
}

/******************************************************************************/
static int append_event(mid_song_t *p_song, mid_event_t const *p_event)
{
	if(p_song->event_count == p_song->event_capacity){
		mid_event_t *p_events;
		p_events = grow_array(p_song->event_array, sizeof(mid_event_t), &p_song->event_capacity);
		if(NULL == p_events){
			return MID_RESULT_ERROR_MEMORY;
		}
		p_song->event_array = p_events;
	}

	p_song->event_array[p_song->event_count] = *p_event;
	p_song->event_count++;
	return MID_RESULT_OK;
}

/******************************************************************************/
static int append_meta_event(
	mid_song_t *p_song,
	uint32_t const tick,
	uint16_t const track,
	uint8_t const number,
	uint32_t const data_length,
	uint8_t const *p_data)
{
	mid_event_t event;

	memset(&event, 0, sizeof(event));
	event.tick = tick;
	event.track = track;
	event.type = MidEventTypeMeta;
	event.meta.number = number;
	event.meta.data_length = data_length;
	if(0 != data_length){
		event.meta.p_data = malloc(data_length);
		if(NULL == event.meta.p_data){
			return MID_RESULT_ERROR_MEMORY;
		}
		memcpy(event.meta.p_data, p_data, data_length);
	}
	if(0 != append_event(p_song, &event)){
		free(event.meta.p_data);
		return MID_RESULT_ERROR_MEMORY;
	}
	return MID_RESULT_OK;
}

/******************************************************************************/
static int append_sysex_event(
	mid_song_t *p_song,
	uint32_t const tick,
	uint16_t const track,
	uint32_t const data_length,
	uint8_t const *p_data)
{
	mid_event_t event;

	memset(&event, 0, sizeof(event));
	event.tick = tick;
	event.track = track;
	event.type = MidEventTypeSysex;
	event.sysex.data_length = data_length;
	if(0 != data_length){
		event.sysex.p_data = malloc(data_length);
		if(NULL == event.sysex.p_data){
			return MID_RESULT_ERROR_MEMORY;
		}
		memcpy(event.sysex.p_data, p_data, data_length);
	}
	if(0 != append_event(p_song, &event)){
		free(event.sysex.p_data);
		return MID_RESULT_ERROR_MEMORY;
	}
	return MID_RESULT_OK;
}

/******************************************************************************/
static int append_tempo_event(
	mid_song_t *p_song,
	uint32_t const tick,
	uint16_t const track,
	uint32_t const us_per_quarter)
{
	mid_event_t event;

	memset(&event, 0, sizeof(event));
	event.tick = tick;
	event.track = track;
	event.type = MidEventTypeTempo;
	event.microseconds_per_quarter_note = us_per_quarter;
	return append_event(p_song, &event);
}

/******************************************************************************/
static int append_timesig_event(
	mid_song_t *p_song,
	uint32_t const tick,
	uint16_t const track,
	unsigned char const p_data[4])
{
	uint8_t const denominator = (uint8_t)(1 << p_data[1]);
	mid_event_t event;

	memset(&event, 0, sizeof(event));
	event.tick = tick;
	event.track = track;
	event.type = MidEventTypeTimeSignature;
	event.time_signature.numerator = p_data[0];
	event.time_signature.denominator = denominator;
	event.time_signature.clocks_per_click = p_data[2];
	event.time_signature.thirtysecond_notes_per_quarter = p_data[3];
	return append_event(p_song, &event);
}

/******************************************************************************/
static void merge_sorted_events(
	mid_event_t *p_events,
	mid_event_t *p_tmp,
	size_t const begin,
	size_t const middle,
	size_t const end)
{
	size_t left = begin;
	size_t right = middle;
	size_t out = begin;

	while((left < middle) && (right < end)){
		do {
			if(p_events[left].tick <= p_events[right].tick){
				p_tmp[out++] = p_events[left++];
				break;
			}
			p_tmp[out++] = p_events[right++];
		} while(0);
	}

	while(left < middle){
		p_tmp[out++] = p_events[left++];
	}
	while(right < end){
		p_tmp[out++] = p_events[right++];
	}

	memcpy(p_events + begin, p_tmp + begin, sizeof(*p_events) * (end - begin));
}

/******************************************************************************/
static int stable_sort_song_events_recursive(
	mid_event_t *p_events,
	mid_event_t *p_tmp,
	size_t const begin,
	size_t const end)
{
	size_t middle;

	if((end - begin) < 2){
		return MID_RESULT_OK;
	}

	middle = begin + ((end - begin) / 2);
	if(MID_RESULT_OK != stable_sort_song_events_recursive(p_events, p_tmp, begin, middle)){
		return MID_RESULT_ERROR_MEMORY;
	}
	if(MID_RESULT_OK != stable_sort_song_events_recursive(p_events, p_tmp, middle, end)){
		return MID_RESULT_ERROR_MEMORY;
	}

	if(p_events[middle - 1].tick <= p_events[middle].tick){
		return MID_RESULT_OK;
	}

	merge_sorted_events(p_events, p_tmp, begin, middle, end);
	return MID_RESULT_OK;
}

/******************************************************************************/
static int sort_song_events(mid_song_t *p_song)
{
	mid_event_t *p_tmp;

	if(p_song->event_count < 2){
		return MID_RESULT_OK;
	}

	p_tmp = malloc(sizeof(*p_tmp) * p_song->event_count);
	if(NULL == p_tmp){
		return MID_RESULT_ERROR_MEMORY;
	}

	if(MID_RESULT_OK != stable_sort_song_events_recursive(
		p_song->event_array, p_tmp, 0, p_song->event_count)){
		free(p_tmp);
		return MID_RESULT_ERROR_MEMORY;
	}

	free(p_tmp);
	return MID_RESULT_OK;
}

/******************************************************************************/
static uint32_t make_channel_message(uint8_t const status, uint8_t const data1, uint8_t const data2)
{
	return (uint32_t)status
		 | ((uint32_t)data1 << 8)
		 | ((uint32_t)data2 << 16);
}

/******************************************************************************/
static uint32_t make_pitch_wheel_message(uint8_t const status, uint8_t const data1, uint8_t const data2)
{
	uint16_t const value = (uint16_t)(((uint16_t)(data2 & 0x7F) << 7) | (uint16_t)(data1 & 0x7F));

#ifdef _IDENTICAL_TO_QMIDI
	return (uint32_t)status
		 | ((uint32_t)((uint8_t)value) << 8)
		 | ((uint32_t)((uint8_t)(value >> 7)) << 16);
#else
	return (uint32_t)status
		 | ((uint32_t)(value & 0x7F) << 8)
		 | ((uint32_t)(((value >> 7) & 0x7F)) << 16);
#endif
}

/******************************************************************************/
static int load_track(
	FILE *p_fp,
	mid_song_t *p_song,
	uint16_t const track_index,
	uint32_t const chunk_start,
	uint32_t const chunk_size)
{
	uint32_t previous_tick;
	long const chunk_end = (long)(chunk_start + chunk_size);
	uint8_t running_status;
	int at_end_of_track;

	previous_tick = 0;
	running_status = 0;
	at_end_of_track = 0;

	while((ftell(p_fp) < chunk_end) && (0 == at_end_of_track)){
		uint32_t delta_tick;
		uint32_t tick;
		uint8_t status;

		if(0 != read_vlq(p_fp, &delta_tick)){
			return MID_RESULT_ERROR_IO;
		}
		tick = previous_tick + delta_tick;
		previous_tick = tick;

		if(0 != read_exact(p_fp, &status, 1)){
			return MID_RESULT_ERROR_IO;
		}

		do {
			if(0 != (status & 0x80)){
				running_status = status;
				break;
			}
			if(0 == running_status){
				return MID_RESULT_ERROR_FORMAT;
			}
			if(0 != fseek(p_fp, -1L, SEEK_CUR)){
				return MID_RESULT_ERROR_IO;
			}
			status = running_status;
		} while(0);

		switch(status & 0xF0){
		case 0x80:
		case 0x90:
		case 0xA0:
		case 0xB0:
		case 0xE0: {
			uint8_t data1;
			uint8_t data2;

			if((0 != read_exact(p_fp, &data1, 1)) || (0 != read_exact(p_fp, &data2, 1))){
				return MID_RESULT_ERROR_IO;
			}

			if((0x90 == (status & 0xF0)) && (0 == data2)){
				mid_event_t event;
				memset(&event, 0, sizeof(event));
				event.tick = tick;
				event.track = track_index;
				event.type = MidEventTypeMessage;
				event.message =
					make_channel_message((uint8_t)(0x80 | (status & 0x0F)), data1, 64);
				if(0 != append_event(p_song,
										 &event)){
					return MID_RESULT_ERROR_EVENT;
				}
				break;
			}

			if(0xE0 == (status & 0xF0)){
				mid_event_t event;
				memset(&event, 0, sizeof(event));
				event.tick = tick;
				event.track = track_index;
				event.type = MidEventTypeMessage;
				event.message = make_pitch_wheel_message(status, data1, data2);
				if(0 != append_event(p_song,
										 &event)){
					return MID_RESULT_ERROR_EVENT;
				}
				break;
			}

			{
				mid_event_t event;
				memset(&event, 0, sizeof(event));
				event.tick = tick;
				event.track = track_index;
				event.type = MidEventTypeMessage;
				event.message = make_channel_message(status, data1, data2);
				if(0 != append_event(p_song,
										 &event)){
					return MID_RESULT_ERROR_EVENT;
				}
			}
			break;
		}
		case 0xC0:
		case 0xD0: {
			uint8_t data1;

			if(0 != read_exact(p_fp, &data1, 1)){
				return MID_RESULT_ERROR_IO;
			}
			mid_event_t event;
			memset(&event, 0, sizeof(event));
			event.tick = tick;
			event.track = track_index;
			event.type = MidEventTypeMessage;
			event.message = make_channel_message(status, data1, 0);
			if(0 != append_event(p_song,
									 &event)){
				return MID_RESULT_ERROR_MEMORY;
			}
			break;
		}
		case 0xF0: {
			if((0xF0 == status) || (0xF7 == status)){
				uint32_t data_length;
				uint8_t *p_data;
				if(0 != read_vlq(p_fp, &data_length)){
					return -1;
				}
				p_data = malloc(data_length + 1);
				if(NULL == p_data){
					return -1;
				}
				p_data[0] = status;
				if(0 != data_length){
					if(NULL == p_data){
						return MID_RESULT_ERROR_MEMORY;
					}
					if(0 != read_exact(p_fp, p_data + 1, data_length)){
						free(p_data);
						return MID_RESULT_ERROR_IO;
					}
				}
				if(0 != append_sysex_event(p_song, tick, track_index, data_length + 1, p_data)){
					free(p_data);
					return MID_RESULT_ERROR_EVENT;
				}
				free(p_data);
				p_song->sysex_count++;
				break;
			}

			if(0xFF == status){
				uint8_t meta_number;
				uint32_t data_length;

				if(0 != read_exact(p_fp, &meta_number, 1)){
					return MID_RESULT_ERROR_IO;
				}
				if(0 != read_vlq(p_fp, &data_length)){
					return MID_RESULT_ERROR_FORMAT;
				}

				if(0x2F == meta_number){
					if(0 != skip_bytes(p_fp, data_length)){
						return MID_RESULT_ERROR_IO;
					}
					at_end_of_track = 1;
					break;
				}

				if((0x51 == meta_number) && (3 == data_length)){
					unsigned char data[3];
					uint32_t us_per_quarter;

					if(0 != read_exact(p_fp, data, sizeof(data))){
						return MID_RESULT_ERROR_IO;
					}
					us_per_quarter = ((uint32_t)data[0] << 16)
								   | ((uint32_t)data[1] << 8)
								   | (uint32_t)data[2];
					if(0 != append_tempo_event(p_song, tick, track_index, us_per_quarter)){
						return MID_RESULT_ERROR_EVENT;
					}
					p_song->meta_count++;
					break;
				}

				if((0x58 == meta_number) && (4 == data_length)){
					unsigned char data[4];

					if(0 != read_exact(p_fp, data, sizeof(data))){
						return MID_RESULT_ERROR_IO;
					}
					if(0 != append_timesig_event(p_song, tick, track_index, data)){
						return MID_RESULT_ERROR_EVENT;
					}
					p_song->meta_count++;
					break;
				}

				{
					uint8_t *p_data;
					p_data = NULL;
					if(0 != data_length){
						p_data = malloc(data_length);
						if(NULL == p_data){
							return MID_RESULT_ERROR_MEMORY;
						}
						if(0 != read_exact(p_fp, p_data, data_length)){
							free(p_data);
							return MID_RESULT_ERROR_IO;
						}
					}
					if(0 != append_meta_event(p_song, tick, track_index, meta_number, data_length, p_data)){
						free(p_data);
						return MID_RESULT_ERROR_EVENT;
					}
					free(p_data);
					p_song->meta_count++;
					break;
				}
			}

			return MID_RESULT_ERROR_FORMAT;
		}
		default:
			return MID_RESULT_ERROR_FORMAT;
		}
	}

	if(0 != fseek(p_fp, chunk_end, SEEK_SET)){
		return MID_RESULT_ERROR_IO;
	}
	return MID_RESULT_OK;
}

/******************************************************************************/
static void mid_song_reset(mid_song_t * const p_song)
{
	memset(p_song, 0, sizeof(*p_song));
	p_song->division_type = MidDivisionTypePpq;
	p_song->file_format = 1;
}

/******************************************************************************/
void mid_song_init(mid_song_t * const p_song)
{
	mid_song_reset(p_song);
}

/******************************************************************************/
void mid_song_close(mid_song_t * const p_song)
{
	size_t i;

	for(i = 0; i < p_song->event_count; i++){
		if(MidEventTypeMeta == p_song->event_array[i].type){
			free(p_song->event_array[i].meta.p_data);
		}
		if(MidEventTypeSysex == p_song->event_array[i].type){
			free(p_song->event_array[i].sysex.p_data);
		}
	}
	free(p_song->event_array);
}

/******************************************************************************/
int mid_song_load(mid_song_t * const p_song, char const * const p_path)
{
	FILE *p_fp;
	unsigned char chunk_id[4];
	uint32_t chunk_size;
	uint32_t chunk_start;
	uint16_t file_format;
	uint16_t track_count;
	unsigned char division[2];
	uint16_t tracks_read;

	mid_song_close(p_song);
	mid_song_reset(p_song);

	p_fp = fopen(p_path, "rb");
	if(NULL == p_fp){
		return MID_RESULT_ERROR_IO;
	}

	if((0 != read_exact(p_fp, chunk_id, sizeof(chunk_id))) || (0 != read_u32_be(p_fp, &chunk_size))){
		fclose(p_fp);
		return MID_RESULT_ERROR_IO;
	}
	chunk_start = (uint32_t)ftell(p_fp);

	if(0 == memcmp(chunk_id, "RIFF", 4)){
		if((0 != read_exact(p_fp, chunk_id, sizeof(chunk_id))) || (0 != memcmp(chunk_id, "RMID", 4))){
			fclose(p_fp);
			return MID_RESULT_ERROR_IO;
		}
		if((0 != read_exact(p_fp, chunk_id, sizeof(chunk_id))) || (0 != read_u32_be(p_fp, &chunk_size)) || (0 != memcmp(chunk_id, "data", 4))){
			fclose(p_fp);
			return MID_RESULT_ERROR_IO;
		}
		if((0 != read_exact(p_fp, chunk_id, sizeof(chunk_id))) || (0 != read_u32_be(p_fp, &chunk_size))){
			fclose(p_fp);
			return MID_RESULT_ERROR_IO;
		}
		chunk_start = (uint32_t)ftell(p_fp);
	}

	if(0 != memcmp(chunk_id, "MThd", 4)){
		fclose(p_fp);
		return MID_RESULT_ERROR_FORMAT;
	}

	if((0 != read_u16_be(p_fp, &file_format)) || (0 != read_u16_be(p_fp, &track_count)) || (0 != read_exact(p_fp, division, sizeof(division)))){
		fclose(p_fp);
		return MID_RESULT_ERROR_IO;
	}

	p_song->file_format = (int)file_format;
	p_song->track_count = (int)track_count;
	switch((int8_t)division[0]){
	case -24:
		p_song->division_type = MidDivisionTypeSmpte24;
		p_song->resolution = division[1];
		break;
	case -25:
		p_song->division_type = MidDivisionTypeSmpte25;
		p_song->resolution = division[1];
		break;
	case -29:
		p_song->division_type = MidDivisionTypeSmpte30Drop;
		p_song->resolution = division[1];
		break;
	case -30:
		p_song->division_type = MidDivisionTypeSmpte30;
		p_song->resolution = division[1];
		break;
	default:
		p_song->division_type = MidDivisionTypePpq;
		p_song->resolution = ((int)division[0] << 8) | (int)division[1];
		break;
	}

	if(0 != fseek(p_fp, (long)(chunk_start + chunk_size), SEEK_SET)){
		fclose(p_fp);
		mid_song_close(p_song);
		mid_song_reset(p_song);
		return MID_RESULT_ERROR_IO;
	}

	tracks_read = 0;
	while(tracks_read < track_count){
		uint32_t track_chunk_size;
		uint32_t track_chunk_start;

		if((0 != read_exact(p_fp, chunk_id, sizeof(chunk_id))) || (0 != read_u32_be(p_fp, &track_chunk_size))){
			fclose(p_fp);
			mid_song_close(p_song);
			mid_song_reset(p_song);
			return MID_RESULT_ERROR_IO;
		}
		track_chunk_start = (uint32_t)ftell(p_fp);

		if(0 != memcmp(chunk_id, "MTrk", 4)){
			fclose(p_fp);
			mid_song_close(p_song);
			mid_song_reset(p_song);
			return MID_RESULT_ERROR_FORMAT;
		}

		if(0 != load_track(p_fp, p_song, tracks_read, track_chunk_start, track_chunk_size)){
			fclose(p_fp);
			mid_song_close(p_song);
			mid_song_reset(p_song);
			return MID_RESULT_ERROR_EVENT;
		}
		tracks_read++;
	}

	fclose(p_fp);
	if(MID_RESULT_OK != sort_song_events(p_song)){
		mid_song_close(p_song);
		mid_song_reset(p_song);
		return MID_RESULT_ERROR_MEMORY;
	}
	return MID_RESULT_OK;
}

/******************************************************************************/
float mid_song_time_from_tick(mid_song_t const * const p_song, uint32_t const tick)
{
	size_t i;

	if((NULL == p_song) || (p_song->resolution <= 0)){
		return -1.0f;
	}

	switch(p_song->division_type){
	case MidDivisionTypePpq: {
		float tempo_event_time = 0.0f;
		uint32_t tempo_event_tick = 0;
		float tempo = 120.0f;

		for(i = 0; i < p_song->event_count; i++){
			mid_event_t const * const p_event = &p_song->event_array[i];

			if((MidEventTypeTempo != p_event->type) || (0 != p_event->track)){
				continue;
			}
			if(p_event->tick >= tick){
				break;
			}

			tempo_event_time +=
				((float)(p_event->tick - tempo_event_tick))
				/ (float)p_song->resolution
				/ (tempo / 60.0f);
			tempo_event_tick = p_event->tick;
			tempo = 60000000.0f / (float)p_event->microseconds_per_quarter_note;
		}

		return tempo_event_time
			+ ((float)(tick - tempo_event_tick))
			/ (float)p_song->resolution
			/ (tempo / 60.0f);
	}
	case MidDivisionTypeSmpte24:
		return (float)tick / ((float)p_song->resolution * 24.0f);
	case MidDivisionTypeSmpte25:
		return (float)tick / ((float)p_song->resolution * 25.0f);
	case MidDivisionTypeSmpte30Drop:
		return (float)tick / ((float)p_song->resolution * 29.97f);
	case MidDivisionTypeSmpte30:
		return (float)tick / ((float)p_song->resolution * 30.0f);
	default:
		return -1.0f;
	}
}

/******************************************************************************/
uint32_t mid_song_tick_from_time(mid_song_t const * const p_song, float const time_in_seconds)
{
	size_t i;

	if((NULL == p_song) || (p_song->resolution <= 0) || (time_in_seconds < 0.0f)){
		return 0;
	}

	switch(p_song->division_type){
	case MidDivisionTypePpq: {
		float tempo_event_time = 0.0f;
		uint32_t tempo_event_tick = 0;
		float tempo = 120.0f;

		for(i = 0; i < p_song->event_count; i++){
			mid_event_t const * const p_event = &p_song->event_array[i];
			float next_tempo_event_time;

			if((MidEventTypeTempo != p_event->type) || (0 != p_event->track)){
				continue;
			}

			next_tempo_event_time =
				tempo_event_time
				+ ((float)(p_event->tick - tempo_event_tick))
				/ (float)p_song->resolution
				/ (tempo / 60.0f);
			if(next_tempo_event_time >= time_in_seconds){
				break;
			}

			tempo_event_time = next_tempo_event_time;
			tempo_event_tick = p_event->tick;
			tempo = 60000000.0f / (float)p_event->microseconds_per_quarter_note;
		}

		return tempo_event_tick
			+ (uint32_t)((time_in_seconds - tempo_event_time) * (tempo / 60.0f) * (float)p_song->resolution);
	}
	case MidDivisionTypeSmpte24:
		return (uint32_t)(time_in_seconds * (float)p_song->resolution * 24.0f);
	case MidDivisionTypeSmpte25:
		return (uint32_t)(time_in_seconds * (float)p_song->resolution * 25.0f);
	case MidDivisionTypeSmpte30Drop:
		return (uint32_t)(time_in_seconds * (float)p_song->resolution * 29.97f);
	case MidDivisionTypeSmpte30:
		return (uint32_t)(time_in_seconds * (float)p_song->resolution * 30.0f);
	default:
		return 0;
	}
}
