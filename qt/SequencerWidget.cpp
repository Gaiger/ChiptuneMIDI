#include <QDebug>
#include <QPainter>

#include "GetInstrumentNameString.h"

#include "SequencerWidget.h"
#include "ui_SequencerWidgetForm.h"

#define A0											(21)
#define ONE_NAME_WIDTH								(64)
#define ONE_NAME_HEIGHT								(12)

NoteNameWidget::NoteNameWidget(QWidget *parent)
	: QWidget(parent) {
	QSize size = QSize(ONE_NAME_WIDTH + 2, (INT8_MAX - A0 + 1) * ONE_NAME_HEIGHT);
	setFixedSize(size);
}

NoteNameWidget::~NoteNameWidget(void)
{
	qDebug() << Q_FUNC_INFO;
}

/**********************************************************************************/

void NoteNameWidget::paintEvent(QPaintEvent *event) {
	QWidget::paintEvent(event);

	QPainter painter(this);
	for(int i = 0; i < INT8_MAX - A0 + 1; i++){
		painter.drawRect(QRect(0, i * ONE_NAME_HEIGHT, ONE_NAME_WIDTH, ONE_NAME_HEIGHT));
	}

	QFont font = painter.font() ;
	font.setPointSize(ONE_NAME_HEIGHT/2);
	font.setBold(true);
	painter.setFont(font);

	painter.drawText( QPoint(ONE_NAME_WIDTH*1/8, (QWidget::height() - ONE_NAME_HEIGHT + ONE_NAME_HEIGHT*2/3)), "A0");
	painter.drawText( QPoint(ONE_NAME_WIDTH*1/8, (QWidget::height() - 2 * ONE_NAME_HEIGHT + ONE_NAME_HEIGHT*2/3)), "A#0/Bb0");
	painter.drawText( QPoint(ONE_NAME_WIDTH*1/8, (QWidget::height() - 3 * ONE_NAME_HEIGHT + ONE_NAME_HEIGHT*2/3)), "B0");

	QList<QString> note_name_string_list;
	note_name_string_list.append("C1");
	note_name_string_list.append("C#1/Db1");
	note_name_string_list.append("D1");
	note_name_string_list.append("D#1/Eb1");
	note_name_string_list.append("E1");
	note_name_string_list.append("F1");
	note_name_string_list.append("F#1/Gb1");
	note_name_string_list.append("G1");
	note_name_string_list.append("G#1/Ab1");
	note_name_string_list.append("A1");
	note_name_string_list.append("A#1/Bb1");
	note_name_string_list.append("B1");

	int kk = 0;
	for(int i = 3; i < INT8_MAX - A0 + 1; i++){
		QString note_name_string = note_name_string_list.at(kk % 12);
		QString number_string = QString::number(kk / 12 + 1);;
		note_name_string.replace("1", number_string);
		painter.drawText( QPoint(ONE_NAME_WIDTH*1/8, (QWidget::height() - (4 + kk) * ONE_NAME_HEIGHT + ONE_NAME_HEIGHT*2/3)),
						  note_name_string);
		kk += 1;
	}
}

/**********************************************************************************/
#define ONE_BEAT_WIDTH					(64)
#define ONE_BEAT_HEIGHT					(ONE_NAME_HEIGHT)

SequencerWidget::SequencerWidget(QMidiFile *p_midi_file, QScrollBar *p_scrollbar, QWidget *parent) :
	QWidget(parent),
	m_p_scrollbar(p_scrollbar),
	m_is_corrected_posistion(false)
{
	QSize size = QSize(parent->width() - ONE_NAME_WIDTH * 3 /2, (INT8_MAX - A0 + 1) * ONE_NAME_HEIGHT);

	setFixedSize(size);
	//QWidget::setAutoFillBackground(true);

	m_p_midi_file = p_midi_file;
	for(int j = 0; j < 2; j++){
		for(int i = 0; i < 16; i++){
			m_rectangle_vector_list[j].append(QVector<QRect>());
		}
	}

	for(int i = 0; i < 16; i++){
		m_is_channel_to_draw[i] = true;
	}
	m_drawing_index = 0;

}

/**********************************************************************************/

SequencerWidget::~SequencerWidget(){ qDebug() << Q_FUNC_INFO; }

/**********************************************************************************/

int SequencerWidget::tickToX(int tick, int const tick_in_center)
{
	int x = ONE_BEAT_WIDTH * (tick - tick_in_center)/(double)m_p_midi_file->resolution();
	x += QWidget::width()/2;
	return x;
}

/**********************************************************************************/

void SequencerWidget::DrawChannelEnabled(int channel_index, bool is_enabled)
{
	m_is_channel_to_draw[channel_index] = is_enabled;
}

/**********************************************************************************/

void SequencerWidget::DrawSequencer(int tick_in_center)
{
	QMutexLocker locker(&m_mutex);
	int preparing_index = (m_drawing_index + 1) % 2;

	for(int k = 0; k < 16; k++){
		m_rectangle_vector_list[preparing_index][k].clear();
	}

	QList<QMidiEvent*> midievent_list = m_p_midi_file->events();

	typedef struct
	{
		int start_tick;
		int end_tick;
		int note;
		int voice;
	}draw_note_t;

	QList<draw_note_t> draw_note_list;

	for(int i = 0; i < midievent_list.size(); i++){
		QMidiEvent * const p_event = midievent_list.at(i);

		int tick_x_position = tickToX(p_event->tick(), tick_in_center);
		if(tick_x_position < 0){
			continue;
		}

		if(tick_x_position > QWidget::width()){
			break;
		}

		if(false == m_is_channel_to_draw[p_event->voice()]){
			continue;
		}

		do {
			if(QMidiEvent::NoteOn == p_event->type()){
				draw_note_t draw_note;
				draw_note.start_tick = p_event->tick();
				draw_note.end_tick = -1;
				draw_note.note = p_event->note();
				draw_note.voice = p_event->voice();
				draw_note_list.append(draw_note);
				break;
			}

			if(QMidiEvent::NoteOff == p_event->type()){
				bool is_make_closed = false;
				int kk = 0;
				for( ; kk < draw_note_list.size(); kk++){
					draw_note_t draw_note = draw_note_list.at(kk);

					if(-1 == draw_note.start_tick){
						continue;
					}
					if(draw_note.voice != p_event->voice()){
						continue;
					}
					if(draw_note.note != p_event->note()){
						continue;
					}
					if(draw_note.start_tick > p_event->tick()){
						continue;
					}


					int x = tickToX(draw_note.start_tick, tick_in_center);
					int y = (QWidget::height() - (draw_note.note - A0 - 1) * ONE_BEAT_HEIGHT);
					int width = tickToX(p_event->tick(), tick_in_center) - x;

					int height = ONE_BEAT_HEIGHT;
					m_rectangle_vector_list[preparing_index][draw_note.voice].append(QRect(x, y, width, height));
					is_make_closed = true;
					break;
				}

				do {
					if(true == is_make_closed){
						draw_note_list.removeAt(kk);
						break;
					}

					draw_note_t draw_note;
					draw_note.start_tick = -1;
					draw_note.end_tick = p_event->tick();
					draw_note.note = p_event->note();
					draw_note.voice = p_event->voice();
					draw_note_list.append(draw_note);
				}while(0);
			}
		}while(0);
	}

	for(int i = 0; i < draw_note_list.size(); i++){
		draw_note_t draw_note = draw_note_list.at(i);
		int x = 0;
		int width = 0;
		do{
			if(-1 == draw_note.end_tick){
				x = tickToX(draw_note.start_tick, tick_in_center);
				//width = x;
				width = QWidget::width() - x;
				break;
			}

			if(-1 == draw_note.start_tick){
				x = 0;
				width = tickToX(draw_note.end_tick, tick_in_center);
				break;
			}

			qDebug() << "ERROR :: draw_note_list";
		}while(0);
		int y = (QWidget::height() - (draw_note.note - A0 - 1) * ONE_BEAT_HEIGHT);
		int height = ONE_BEAT_HEIGHT;
		m_rectangle_vector_list[preparing_index][draw_note.voice].append(QRect(x, y, width, height));
	}

#if(0)
	for(int i = 0; i < 16;i++){
		for(int j = 0; j < m_rectangle_vector_list[preparing_index][i].size(); j++){
			qDebug() << m_rectangle_vector_list[preparing_index][i].at(j);
		}
	}
#endif
	m_drawing_index = preparing_index;
	QWidget::update();
}

/**********************************************************************************/

void SequencerWidget::paintEvent(QPaintEvent *event)
{
	QMutexLocker locker(&m_mutex);
	QWidget::paintEvent(event);

	if(false == m_is_corrected_posistion){
		QList<QMidiEvent*> midievent_list = m_p_midi_file->events();
		for(int i = 0; i < midievent_list.size(); i++){
			QMidiEvent * const p_event = midievent_list.at(i);
			if(QMidiEvent::NoteOn == p_event->type()){
				m_p_scrollbar->setValue((QWidget::height() - (p_event->note() - A0 - 1) * ONE_BEAT_HEIGHT));
				break;
			}
		}
		m_is_corrected_posistion = true;
	}

	QPainter painter(this);

	for(int k = 0; k < 16; k++){
		QColor color = GetChannelColor(k);
		color.setAlpha(192);
		painter.setBrush(color);
		for(int i = 0; i < m_rectangle_vector_list[m_drawing_index].at(k).size(); i++){
			painter.drawRect(m_rectangle_vector_list[m_drawing_index].at(k).at(i));
		}
	}
}
