#ifndef _SYNTHESIZERSEQUENCERWIDGET_H_
#define _SYNTHESIZERSEQUENCERWIDGET_H_

#include <QtGlobal>
#include <QWidget>
#include <QScrollArea>

class NoteNameWidget;
class NoteDurationWidget;
class QGridLayout;

class SynthesizerSequencerNoteEvent
{
public:
	enum NoteState {
		NoteOn,
		NoteOff,
	};

	SynthesizerSequencerNoteEvent(NoteState const note_state,
								  int const channel_index,
								  int const note_number,
								  qint64 const timestamp_in_milliseconds)
		: m_note_state(note_state),
		  m_channel_index(channel_index),
		  m_note_number(note_number),
		  m_timestamp_in_milliseconds(timestamp_in_milliseconds){}

	NoteState GetNoteState(void) const { return m_note_state; }
	int GetChannelIndex(void) const { return m_channel_index; }
	int GetNoteNumber(void) const { return m_note_number; }
	qint64 GetTimestampInMilliseconds(void) const { return m_timestamp_in_milliseconds; }
private:
	NoteState const m_note_state;
	int const m_channel_index;
	int const m_note_number;
	qint64 const m_timestamp_in_milliseconds;
};

class SynthesizerSequencerWidget : public QWidget
{
	Q_OBJECT
public:
	enum ViewMode {
		ViewModeWaterfall,
		ViewModeRoll,
	};

	explicit SynthesizerSequencerWidget(QScrollArea * const p_parent_scroll_area);
	~SynthesizerSequencerWidget(void);
	void SendNoteEvent(SynthesizerSequencerNoteEvent const &note_event);
	void SendAllNotesOffEvent(int const channel_index, qint64 const current_timestamp_in_ms);
	void SetViewMode(ViewMode const view_mode);
	void Update(void);
	void Prepare(qint64 const current_timestamp_in_ms);
private:
	void ApplyViewModeLayout(void);
private:
	NoteNameWidget *m_p_note_name_widget;
	NoteDurationWidget *m_p_note_duration_widget;
	QScrollArea * const m_p_parent_scroll_area;
	QGridLayout *m_p_layout;
	ViewMode m_view_mode;
};

#endif // _SYNTHESIZERSEQUENCERWIDGET_H_
