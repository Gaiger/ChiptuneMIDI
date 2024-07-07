#ifndef SEQUENCERWIDGET_H
#define SEQUENCERWIDGET_H

#include <QWidget>
#include "QMidiFile.h"
#include <QMutex>

#include <QScrollBar>
#include "TuneManager.h"

class QMidiEvent;

class NoteNameWidget : public QWidget
{
	Q_OBJECT
public:
	explicit NoteNameWidget(QWidget *parent = nullptr);
	~NoteNameWidget(void);
private:
	virtual void paintEvent(QPaintEvent  *event) Q_DECL_OVERRIDE;
};


class SequencerWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SequencerWidget(TuneManager *p_tune_manager, QScrollBar *p_scrollbar,
							 double audio_out_latency_in_seconds = 0.0, QWidget *parent = nullptr);
	~SequencerWidget(void);
public :
	void PrepareSequencer(int tick_in_center);
	void DrawSequencer(void);

	void SetChannelToDrawEnabled(int channel_index, bool is_enabled);
private :
	QRect NoteToQRect(int start_tick, int end_tick, int tick_in_center, int note);
	bool IsTickOutOfRightBound(int tick, int tick_in_center);
	int tickToX(int tick, int const tick_in_center);
	int XtoTick(int x, int const tick_in_center);

	void ReduceRectangles(int preparing_index);
private:
	virtual void paintEvent(QPaintEvent  *event) Q_DECL_OVERRIDE;
private:
	TuneManager *m_p_tune_manager;
	QScrollBar *m_p_scrollbar;

	double m_audio_out_latency_in_seconds;

	QList<QVector<QRect>>  m_rectangle_vector_list[2];
	int m_drawing_rectangle_vector_list_index;
	bool m_is_channel_to_draw[MIDI_MAX_CHANNEL_NUMBER];

	int m_last_sought_index;
	int m_last_tick_in_center;

	bool m_is_scrollbar_posistion_corrected;
	int m_scrollbar_minimum;

	QMutex m_mutex;
};

#endif // SEQUENCERWIDGET_H
