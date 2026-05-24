#ifndef _PLAYERSEQUENCERWIDGET_H_
#define _PLAYERSEQUENCERWIDGET_H_

#include <QWidget>
#include <QScrollArea>

class MidSongManager;
class TuneManager;
class NoteNameWidget;
class NoteDurationWidget;

class PlayerSequencerWidget : public QWidget
{
	Q_OBJECT
public :
	explicit PlayerSequencerWidget(MidSongManager *p_mid_song_manager, TuneManager *p_tune_manager,
							 double audio_out_latency_in_seconds,
							 QScrollArea *p_parent_scroll_area);
	~PlayerSequencerWidget(void);
	void Prepare(int tick_in_center);
	void Update(void);
	void SetChannelDrawAsEnabled(int channel_index, bool is_enabled);
private:
	NoteNameWidget *m_p_note_name_widget;
	NoteDurationWidget *m_p_note_duration_widget;

	QScrollArea *m_p_parent_scroll_area;
	int m_vertical_scrolling_shift;
};

#endif // _PLAYERSEQUENCERWIDGET_H_
