#ifndef _SYNTHESIZERSEQUENCERWIDGET_H_
#define _SYNTHESIZERSEQUENCERWIDGET_H_

#include <QWidget>
#include <QScrollArea>

class NoteNameWidget;
class NoteDurationWidget;

class SynthesizerSequencerWidget : public QWidget
{
	Q_OBJECT
public:
	explicit SynthesizerSequencerWidget(QScrollArea *p_parent_scroll_area);
	~SynthesizerSequencerWidget(void);
private:
	NoteNameWidget *m_p_note_name_widget;
	NoteDurationWidget *m_p_note_duration_widget;
};

#endif // _SYNTHESIZERSEQUENCERWIDGET_H_
