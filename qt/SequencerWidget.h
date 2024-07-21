#ifndef SEQUENCERWIDGET_H
#define SEQUENCERWIDGET_H

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
	void SetChannelToDrawEnabled(int channel_index, bool is_enabled);
private:
	NoteNameWidget *m_p_note_name_widget;
	NoteDurationWidget *m_p_note_duration_widget;
};

#endif // SEQUENCERWIDGET_H
