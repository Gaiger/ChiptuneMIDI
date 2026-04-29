#ifndef _CHIPTUNEMIDIVALUES_H_
#define _CHIPTUNEMIDIVALUES_H_

#include <stdint.h>

#include <QString>
#include <QColor>

typedef struct _instrument_timbre_t
{
	int8_t waveform;

	int8_t envelope_attack_curve;
	uint8_t attack_padding[2];
	float envelope_attack_duration_in_seconds;

	int8_t envelope_decay_curve;
	uint8_t decay_padding[3];
	float envelope_decay_duration_in_seconds;

	uint8_t envelope_note_on_sustain_level;

	int8_t envelope_release_curve;
	uint8_t release_padding[2];
	float envelope_release_duration_in_seconds;

	uint8_t envelope_damper_sustain_level;
	int8_t envelope_damper_sustain_curve;
	uint8_t damper_sustain_padding[2];
	float envelope_damper_sustain_duration_in_seconds;
} instrument_timbre_t;

QString GetInstrumentNameString(int const instrument_code);
QColor GetChannelColor(int const channel_index);
instrument_timbre_t GetDefaultInstrumentTimbre(void);

#endif // _CHIPTUNEMIDIVALUES_H_
