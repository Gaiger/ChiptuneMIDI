#ifndef _SYNTHESIZERSEQUENCERWIDGET_H_
#define _SYNTHESIZERSEQUENCERWIDGET_H_

#include <QtGlobal>
#include <QWidget>
#include <QScrollArea>

class NoteNameWidget;
class NoteDurationWidget;

class SynthesizerSequencerNoteEvent
{
public:
	enum NoteState {
		NoteOn,
		NoteOff,
	};

	SynthesizerSequencerNoteEvent(NoteState note_state,
								  int channel_index,
								  int note_number,
								  qint64 timestamp_in_milliseconds)
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
	explicit SynthesizerSequencerWidget(QScrollArea *p_parent_scroll_area);
	~SynthesizerSequencerWidget(void);
	void SendNoteEvent(SynthesizerSequencerNoteEvent const &note_event);
	void SendAllNotesOffEvent(int channel_index, qint64 timestamp_in_ms);
	void Update(void);
	void Prepare(void);
private:
	NoteNameWidget *m_p_note_name_widget;
	NoteDurationWidget *m_p_note_duration_widget;
};

#endif // _SYNTHESIZERSEQUENCERWIDGET_H_
