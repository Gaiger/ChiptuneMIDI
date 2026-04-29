#ifndef _INSTRUMENTTIMBREINIFILE_H_
#define _INSTRUMENTTIMBREINIFILE_H_

#include <stdint.h>

#include <QMap>
#include <QSettings>
#include <QString>

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

class InstrumentTimbreIniFile
{
public:
	explicit InstrumentTimbreIniFile(QString const& file_name_string);
	int ReadTimbres(QMap<int8_t, instrument_timbre_t> * const p_timbres);
	void WriteTimbre(int8_t const instrument_code, int8_t const waveform,
					 int8_t const envelope_attack_curve, float const envelope_attack_duration_in_seconds,
					 int8_t const envelope_decay_curve, float const envelope_decay_duration_in_seconds,
					 uint8_t const envelope_note_on_sustain_level,
					 int8_t const envelope_release_curve, float const envelope_release_duration_in_seconds,
					 uint8_t const envelope_damper_sustain_level,
					 int8_t const envelope_damper_sustain_curve,
					 float const envelope_damper_sustain_duration_in_seconds);

private:
	int ReadTimbre(int8_t const instrument_code, int8_t * const p_waveform,
				   int8_t * const p_envelope_attack_curve, float * const p_envelope_attack_duration_in_seconds,
				   int8_t * const p_envelope_decay_curve, float * const p_envelope_decay_duration_in_seconds,
				   uint8_t * const p_envelope_note_on_sustain_level,
				   int8_t * const p_envelope_release_curve, float * const p_envelope_release_duration_in_seconds,
				   uint8_t * const p_envelope_damper_sustain_level,
				   int8_t * const p_envelope_damper_sustain_curve,
				   float * const p_envelope_damper_sustain_duration_in_seconds);
	QSettings m_settings;
};

#endif // _INSTRUMENTTIMBREINIFILE_H_
