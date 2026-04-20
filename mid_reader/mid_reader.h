/* Written by Codex. */
#ifndef _MID_READER_DRAFT_H_
#define _MID_READER_DRAFT_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MID_RESULT_OK              (0)
#define MID_RESULT_ERROR           (-1)
#define MID_RESULT_ERROR_IO        (-2)
#define MID_RESULT_ERROR_MEMORY    (-3)
#define MID_RESULT_ERROR_FORMAT    (-4)
#define MID_RESULT_ERROR_EVENT     (-5)

typedef enum mid_division_type_t {
	MidDivisionTypeInvalid = -1,
	MidDivisionTypePpq = 0,
	MidDivisionTypeSmpte24 = -24,
	MidDivisionTypeSmpte25 = -25,
	MidDivisionTypeSmpte30Drop = -29,
	MidDivisionTypeSmpte30 = -30
} mid_division_type_t;

typedef enum mid_event_type_t {
	MidEventTypeInvalid = 0,
	MidEventTypeMessage,
	MidEventTypeTempo,
	MidEventTypeTimeSignature,
	MidEventTypeMeta,
	MidEventTypeSysex
} mid_event_type_t;

typedef struct mid_time_signature_t {
	uint8_t numerator;
	uint8_t denominator;
	uint8_t clocks_per_click;
	uint8_t thirtysecond_notes_per_quarter;
} mid_time_signature_t;

typedef struct mid_meta_t {
	uint8_t meta_type;
	uint8_t *p_data;
	uint32_t data_length;
} mid_meta_t;

typedef struct mid_sysex_t {
	uint8_t *p_data;
	uint32_t data_length;
} mid_sysex_t;

typedef struct mid_event_t {
	uint32_t tick;
	uint16_t track;
	mid_event_type_t event_type;
	union {
		uint32_t message;
		uint32_t microseconds_per_quarter_note;
		mid_time_signature_t time_signature;
		mid_meta_t meta;
		mid_sysex_t sysex;
	};
} mid_event_t;

typedef struct mid_song_t {
	int file_format;
	int track_count;
	int resolution;
	mid_division_type_t division_type;

	mid_event_t *event_array;
	size_t event_count;
	size_t event_capacity;

	size_t sysex_count;
	size_t meta_count;
} mid_song_t;

void mid_song_init(mid_song_t * const p_song);
void mid_song_close(mid_song_t * const p_song);
int mid_song_load(mid_song_t * const p_song, char const * const p_path);

float mid_song_time_from_tick(mid_song_t const * const p_song, uint32_t const tick);
uint32_t mid_song_tick_from_time(mid_song_t const * const p_song, float const time_in_seconds);

#ifdef __cplusplus
}
#endif

#endif
