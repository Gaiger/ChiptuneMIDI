#ifndef _INSTRUMENTTIMBREINIFILE_H_
#define _INSTRUMENTTIMBREINIFILE_H_

#include <stdint.h>

#include <QMap>
#include <QSettings>
#include <QString>

#include "ChiptuneMidiValues.h"

class InstrumentTimbreIniFile
{
public:
	explicit InstrumentTimbreIniFile(QString const& file_name_string);
	int ReadTimbres(QMap<int8_t, instrument_timbre_t> * const p_instrument_timbres);
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
