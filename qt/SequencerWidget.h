#ifndef SEQUENCERWIDGET_H
#define SEQUENCERWIDGET_H

#include <QWidget>
#include "QMidiFile.h"
#include <QMutex>

#include <QScrollBar>


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
	explicit SequencerWidget(QMidiFile *p_midi_file, QScrollBar *p_scrollbar, QWidget *parent = nullptr);
	~SequencerWidget(void);
public :
	void DrawSequencer(int tick_in_center);
	void DrawChannelEnabled(int channel_index, bool is_enabled);
private :
	int tickToX(int tick, int const tick_in_center);
	int XtoTick(int x, int const tick_in_center);

private:
	virtual void paintEvent(QPaintEvent  *event) Q_DECL_OVERRIDE;
private:
	QScrollBar *m_p_scrollbar;
	QMidiFile *m_p_midi_file;

	QList<QVector<QRect>>  m_rectangle_vector_list[2];
	int m_drawing_index;
	QMutex m_mutex;
	bool m_is_channel_to_draw[16];
	bool m_is_corrected_posistion;
	int m_last_sought_index;
	int m_last_tick_in_center;
};

#endif // SEQUENCERWIDGET_H
