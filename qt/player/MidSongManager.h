#ifndef _MIDSONGMANAGER_H_
#define _MIDSONGMANAGER_H_


#include <QString>

#include "mid_reader/qt/MidSong.h"
#include "TuneManager.h"

class MidSongManager : public MidiMessageProvider
{
public:
	static MidSongManager *Create(QString const midi_file_name_string);
	~MidSongManager(void) Q_DECL_OVERRIDE;

	MidSong *GetMidSongPointer(void) const Q_DECL_OVERRIDE;

private:
	explicit MidSongManager(MidSong *p_mid_song);

	MidSong *m_p_mid_song;
};

#endif // _MIDSONGMANAGER_H_
