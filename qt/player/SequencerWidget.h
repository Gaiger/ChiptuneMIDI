#ifndef _SEQUENCERWIDGET_H_
#define _SEQUENCERWIDGET_H_

#include <QWidget>
#include <QScrollArea>

#include "TuneManager.h"

class NoteNameWidget;
class NoteDurationWidget;

class SequencerWidget : public QWidget
{
	Q_OBJECT
public :
	explicit SequencerWidget(TuneManager *p_tune_manager, double audio_out_latency_in_seconds,
							 QScrollArea *p_parent_scroll_area);
	~SequencerWidget(void);
	void Prepare(int tick_in_center);
	void Update(void);
	void SetChannelDrawAsEnabled(int channel_index, bool is_enabled);
private:
	NoteNameWidget *m_p_note_name_widget;
	NoteDurationWidget *m_p_note_duration_widget;

	QScrollArea *m_p_parent_scroll_area;
	int m_vertical_scrolling_shift;
};

#endif // _SEQUENCERWIDGET_H_
