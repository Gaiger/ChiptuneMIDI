#include <QDebug>
#include <QFileInfo>

#include "MidSongManager.h"

/**********************************************************************************/

MidSongManager *MidSongManager::Create(QString const midi_file_name_string)
{
	MidSongManager *p_mid_song_manager = nullptr;
	do
	{
		QFileInfo file_info(midi_file_name_string);
		if(false == file_info.isFile()){
			qDebug() << Q_FUNC_INFO << midi_file_name_string << "is not a file";
			break;
		}

		MidSong *p_mid_song = new MidSong();
		if(false == p_mid_song->Load(midi_file_name_string))
		{
			delete p_mid_song; p_mid_song = nullptr;
			break;
		}

		p_mid_song_manager = new MidSongManager(p_mid_song);
	} while(0);
	return p_mid_song_manager;
}

/**********************************************************************************/

MidSongManager::MidSongManager(MidSong *p_mid_song)
	: m_p_mid_song(p_mid_song)
{
}

/**********************************************************************************/

MidSongManager::~MidSongManager(void)
{
	delete m_p_mid_song;
	m_p_mid_song = nullptr;
}

/**********************************************************************************/

MidSong *MidSongManager::GetMidSongPointer(void) const
{
	return m_p_mid_song;
}
