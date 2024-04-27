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

NoteNameWidget::~NoteNameWidget(void) { }

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

SequencerWidget::SequencerWidget(TuneManager *p_tune_manager, QScrollBar *p_scrollbar, QWidget *parent) :
	QWidget(parent),
	m_p_scrollbar(p_scrollbar),
	m_p_tune_manager(p_tune_manager),
	m_is_corrected_posistion(false),
	m_last_sought_index(0),
	m_last_tick_in_center(0)
{
	QSize size = QSize(parent->width() - ONE_NAME_WIDTH * 3 /2, (INT8_MAX - A0 + 1) * ONE_NAME_HEIGHT);
	setFixedSize(size);
#if(0)
	QList<QMidiEvent*> midievent_list = m_p_midi_file->events();
	//QList<QMidiEvent*> midievent_list = m_p_midi_file->events();
	int highest_pitch = A0;
	for(int i = 0; i < midievent_list.size(); i++){
		QMidiEvent * const p_event = midievent_list.at(i);
		if(QMidiEvent::NoteOn == p_event->type()){
			if(p_event->note() > highest_pitch){
				highest_pitch = p_event->note();
			}
		}
	}

	int y = (QWidget::height() - (highest_pitch - A0 - 1) * ONE_BEAT_HEIGHT);
	qDebug() << "y  = " << y;
	p_scrollbar->setMaximum(y);
#endif
	//QWidget::setAutoFillBackground(true);

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

SequencerWidget::~SequencerWidget(){ }

/**********************************************************************************/

int SequencerWidget::tickToX(int tick, int const tick_in_center)
{
	int x = ONE_BEAT_WIDTH * (tick - tick_in_center)/(double)m_p_tune_manager->GetMidiFilePointer()->resolution();
	x += QWidget::width()/2;
	return x;
}

/**********************************************************************************/

int SequencerWidget::XtoTick(int x, int const tick_in_center)
{
	int tick  = tick_in_center;
	x -= QWidget::width()/2;
	tick += ( x / (double) ONE_BEAT_WIDTH) * m_p_tune_manager->GetMidiFilePointer()->resolution();
	return tick;
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

	QList<QMidiEvent*> midievent_list = m_p_tune_manager->GetMidiFilePointer()->events();

	typedef struct
	{
		int note_on_midievent_index;
		int start_tick;
		int end_tick;
		int note;
		int voice;
	}draw_note_t;

	QList<draw_note_t> draw_note_list;

	int const left_tick = XtoTick(0, tick_in_center);
	//qDebug() << "left_tick = " << left_tick;
	int start_index = 0;
	if(m_last_tick_in_center <= tick_in_center){
		start_index = m_last_sought_index;
	}
	//start_index = 0;
	//qDebug() << "start_index " << start_index;
	int sought_index = INT32_MAX;
	for(int i = start_index; i < midievent_list.size(); i++){
		QMidiEvent * const p_event = midievent_list.at(i);

		int tick_x_position = tickToX(p_event->tick(), tick_in_center);
		if(tick_x_position > QWidget::width()){
			bool is_all_closed = true;
			for(int j = 0; j < draw_note_list.size(); j++){
				if(-1 == draw_note_list.at(j).start_tick
						|| -1 == draw_note_list.at(j).end_tick){
					is_all_closed = false;
					break;
				}
			}

			if(true == is_all_closed){
				break;
			}
		}

		if(false == m_is_channel_to_draw[p_event->voice()]){
			continue;
		}

		do {
			if(QMidiEvent::NoteOn == p_event->type()){
				draw_note_t draw_note;
				draw_note.note_on_midievent_index = i;
				draw_note.start_tick = p_event->tick();
				draw_note.end_tick = -1;
				draw_note.note = p_event->note();
				draw_note.voice = p_event->voice();
				draw_note_list.append(draw_note);
				break;
			}

			if(QMidiEvent::NoteOff == p_event->type()){
				bool is_matched = false;
				int k;
				for(k = 0; k < draw_note_list.size(); k++){
					draw_note_t *p_draw_note = &draw_note_list[k];
					if(-1 != p_draw_note->end_tick){
						continue;
					}
					if(p_draw_note->voice != p_event->voice()){
						continue;
					}
					if(p_draw_note->note != p_event->note()){
						continue;
					}
					if(p_draw_note->start_tick > p_event->tick()){
						continue;
					}

					is_matched = true;
					do
					{
						p_draw_note->end_tick = p_event->tick();
						if(left_tick > p_draw_note->end_tick){
							break;
						}

						int x = tickToX(p_draw_note->start_tick, tick_in_center);
						int y = (QWidget::height() - (p_draw_note->note - A0 - 1) * ONE_BEAT_HEIGHT);
						int width = tickToX(p_draw_note->end_tick, tick_in_center) - x;
						int height = ONE_BEAT_HEIGHT;

						m_rectangle_vector_list[preparing_index][p_draw_note->voice].append(QRect(x, y, width, height));
						if(sought_index > p_draw_note->note_on_midievent_index){
							sought_index = p_draw_note->note_on_midievent_index;
						}
					} while(0);
					break;
				}

				do {
					if(true == is_matched){
						draw_note_list.removeAt(k);
						break;
					}
					if(left_tick < p_event->tick()){
						qDebug() << "WARNING :: note not matched : voice = " << p_event->voice()
								 << ", note =" << p_event->note()
								 << "(might be double off)";
					}
				} while(0);
			}
		}while(0);
	}

	//qDebug() << "sought_index " << sought_index;
	//qDebug() << "start_index_list.size() " << start_index_list.size();
	m_last_tick_in_center = tick_in_center;
	if(INT32_MAX != sought_index){
		m_last_sought_index = sought_index;
	}
	m_drawing_index = preparing_index;
	QWidget::update();
}

/**********************************************************************************/

void SequencerWidget::paintEvent(QPaintEvent *event)
{
	QMutexLocker locker(&m_mutex);
	QWidget::paintEvent(event);

	if(false == m_is_corrected_posistion){
		QList<QMidiEvent*> midievent_list = m_p_tune_manager->GetMidiFilePointer()->events();
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
	int total = 0;
	for(int k = 0; k < 16; k++){
		QColor color = GetChannelColor(k);
		painter.setBrush(color);
		for(int i = 0; i < m_rectangle_vector_list[m_drawing_index].at(k).size(); i++){
			total += 1;
			painter.drawRect(m_rectangle_vector_list[m_drawing_index].at(k).at(i));
		}
	}

	QColor color = QColor(0xE0, 0xE0, 0xE0, 0xE0);
	//color.setAlpha(0xFF);
	painter.setPen(color);
	int delay_x = (2 * 2 * 0.1) * m_p_tune_manager->GetTempo()/60.0 * ONE_BEAT_WIDTH;
	//AUDIO buffer size = 2 * 100 ms, the sme value for tunemanager pre-buffer
	painter.drawLine(QWidget::width()/2 - delay_x, 0, QWidget::width()/2 - delay_x, QWidget::height());
}
