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

NoteDurationWidget::NoteDurationWidget(TuneManager *p_tune_manager, QScrollBar *p_scrollbar,
								 double audio_out_latency_in_seconds, QWidget *parent) :
	QWidget(parent),
	m_p_tune_manager(p_tune_manager),
	m_p_scrollbar(p_scrollbar),
	m_audio_out_latency_in_seconds(audio_out_latency_in_seconds),
	m_last_sought_index(0),
	m_last_tick_in_center(0),
	m_is_scrollbar_posistion_corrected(false),
	m_scrollbar_minimum(0)
{
	QSize parent_size = QSize(600, 800);
	if(nullptr != parent){
		parent_size = parent->size();
	}
	QSize size = QSize(parent_size.width() - ONE_NAME_WIDTH * 3 /2, (INT8_MAX - A0 + 1) * ONE_NAME_HEIGHT);
	setFixedSize(size);

	QList<QMidiEvent*> midievent_list = p_tune_manager->GetMidiFilePointer()->events();
	int highest_pitch = A0;
	for(int i = 0; i < midievent_list.size(); i++){
		QMidiEvent * const p_event = midievent_list.at(i);
		if(QMidiEvent::NoteOn == p_event->type()){
			if(p_event->note() > highest_pitch){
				highest_pitch = p_event->note();
			}
		}
	}
#define SCROLLING_UPPER_BORDER_IN_PITCH				(3)
	int sequencer_height = ((highest_pitch + SCROLLING_UPPER_BORDER_IN_PITCH) - A0 - 1) * ONE_BEAT_HEIGHT;
	m_scrollbar_minimum = QWidget::height() - sequencer_height;
	if(sequencer_height < parent_size.height()){
		//sequencer_height = ((parent_size.height()/ONE_BEAT_HEIGHT) + 1) * ONE_BEAT_HEIGHT;
#define LINE_BORDER_WIDTH							(1)
		sequencer_height = parent_size.height() + LINE_BORDER_WIDTH;
		m_scrollbar_minimum = QWidget::height() - sequencer_height;
	}

	for(int j = 0; j < 2; j++){
		for(int voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
			m_rectangle_vector_list[j].append(QVector<QRect>());
		}
	}

	for(int voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		m_is_channel_to_draw[voice] = true;
	}
	m_drawing_rectangle_vector_list_index = 0;

	m_p_scrollbar->setSingleStep(ONE_BEAT_HEIGHT);
}

/**********************************************************************************/

NoteDurationWidget::~NoteDurationWidget(){ }

/**********************************************************************************/

int NoteDurationWidget::tickToX(int tick, int const tick_in_center)
{
	int x = ONE_BEAT_WIDTH * (tick - tick_in_center)/(double)m_p_tune_manager->GetMidiFilePointer()->resolution();
	x += QWidget::width()/2;
	return x;
}

/**********************************************************************************/

int NoteDurationWidget::XtoTick(int x, int tick_in_center)
{
	int tick  = tick_in_center;
	x -= QWidget::width()/2;
	tick += ( x / (double) ONE_BEAT_WIDTH) * m_p_tune_manager->GetMidiFilePointer()->resolution();
	return tick;
}


/**********************************************************************************/

void NoteDurationWidget::SetChannelToDrawEnabled(int channel_index, bool is_enabled)
{
	m_is_channel_to_draw[channel_index] = is_enabled;
}

/**********************************************************************************/

QRect NoteDurationWidget::NoteToQRect(int start_tick, int end_tick, int tick_in_center, int note)
{
	int x = tickToX(start_tick, tick_in_center);
	int y = QWidget::height() - (note - A0 - 1) * ONE_BEAT_HEIGHT;
	int width = tickToX(end_tick, tick_in_center) - x;
	int height = ONE_BEAT_HEIGHT;

	return QRect(x, y, width, height);
}

/**********************************************************************************/

bool NoteDurationWidget::IsTickOutOfRightBound(int tick, int tick_in_center)
{
	int tick_x_position = tickToX(tick, tick_in_center);
	return (tick_x_position > QWidget::width() - 1) ? true : false;
}

/**********************************************************************************/

void NoteDurationWidget::ReduceRectangles(int working_rectangle_vector_list_index)
{
	// Reduce the rectangles from out of the widget boundary.

	int const right_endpoint = QWidget::width() + m_p_scrollbar->width() - 1;
	for(int voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		QMutableVectorIterator<QRect> rect_vector_iterator(m_rectangle_vector_list[working_rectangle_vector_list_index][voice]);
		while(rect_vector_iterator.hasNext()){
			rect_vector_iterator.next();
			QRect rect = rect_vector_iterator.value();

			if(rect.right() < 0){
				rect_vector_iterator.remove();
				continue;
			}

			bool is_reduced = false;
			do{
				if(0 > rect.left()){
					rect.setLeft(0);
					is_reduced = true;
				}

				if(right_endpoint < rect.right()){
					rect.setRight(right_endpoint - 1);
					is_reduced = true;
				}
			}while(0);

			if(true == is_reduced){
				rect_vector_iterator.setValue(rect);
			}
		}
	}
}

/**********************************************************************************/

void NoteDurationWidget::Prepare(int const tick_in_center)
{
	QMutexLocker locker(&m_mutex);
	int working_rectangle_vector_list_index = (m_drawing_rectangle_vector_list_index + 1) % 2;

	for(int voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		m_rectangle_vector_list[working_rectangle_vector_list_index][voice].clear();
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

	int start_index = 0;
	if(m_last_tick_in_center <= tick_in_center){
		start_index = m_last_sought_index;
	}
	//start_index = 0;
	//qDebug() << "start_index " << start_index;
	int sought_index = INT32_MAX;
	for(int i = start_index; i < midievent_list.size(); i++){
		QMidiEvent * const p_event = midievent_list.at(i);

		if(true == IsTickOutOfRightBound(p_event->tick(), tick_in_center)){
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
						m_rectangle_vector_list[working_rectangle_vector_list_index][p_draw_note->voice].append(
									NoteToQRect(p_draw_note->start_tick, p_draw_note->end_tick, tick_in_center, p_draw_note->note)
									);
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

					qDebug() << "WARNING :: note not matched : voice = " << p_event->voice()
							 << ", note =" << p_event->note()
							 << "(might be double off)";
				} while(0);
			}
		}while(0);
	}

	ReduceRectangles(working_rectangle_vector_list_index);

	//qDebug() << "sought_index " << sought_index;
	//qDebug() << "start_index_list.size() " << start_index_list.size();
	m_last_tick_in_center = tick_in_center;
	if(INT32_MAX != sought_index){
		m_last_sought_index = sought_index;
	}
	m_drawing_rectangle_vector_list_index = working_rectangle_vector_list_index;
}

/**********************************************************************************/

void NoteDurationWidget::paintEvent(QPaintEvent *event)
{
	QMutexLocker locker(&m_mutex);
	QWidget::paintEvent(event);

	if(false == m_is_scrollbar_posistion_corrected){
			QList<QMidiEvent*> midievent_list = m_p_tune_manager->GetMidiFilePointer()->events();
			for(int i = 0; i < midievent_list.size(); i++){
				QMidiEvent * const p_event = midievent_list.at(i);
				if(QMidiEvent::NoteOn == p_event->type()){
#define VERTICAL_SCROLLING_SHIFT					(16)
					m_p_scrollbar->setValue(
								(QWidget::height() - ((p_event->note() - A0 - 1) + VERTICAL_SCROLLING_SHIFT) * ONE_BEAT_HEIGHT));
					break;
				}
			}
			m_is_scrollbar_posistion_corrected = true;
	}

	QPainter painter(this);
	//painter.setRenderHint(QPainter::Antialiasing);
	for(int voice = MIDI_MAX_CHANNEL_NUMBER - 1; voice >= 0 ; voice--){
		if(true == m_is_channel_to_draw[voice]){
			continue;
		}
		QColor color = GetChannelColor(voice);

		painter.setBrush(QColor(0xff, 0xff, 0xff, 0x00));
		color.setAlpha(0x30);
		QPen pen(color.lighter(75));
		pen.setWidth(4);
		painter.setPen(pen);
		//painter.setPen(color);
		painter.setBrush(color);
		for(int i = 0; i < m_rectangle_vector_list[m_drawing_rectangle_vector_list_index].at(voice).size(); i++){
			painter.drawRect(m_rectangle_vector_list[m_drawing_rectangle_vector_list_index].at(voice).at(i));
		}
	}

	for(int voice = MIDI_MAX_CHANNEL_NUMBER - 1; voice >= 0 ; voice--){
		if(false == m_is_channel_to_draw[voice]){
			continue;
		}
		QColor color = GetChannelColor(voice);
		painter.setBrush(color);
		painter.setPen(QColor(0xFF, 0xFF, 0xFF, 0xC0));

		for(int i = 0; i < m_rectangle_vector_list[m_drawing_rectangle_vector_list_index].at(voice).size(); i++){
			painter.drawRect(m_rectangle_vector_list[m_drawing_rectangle_vector_list_index].at(voice).at(i));
		}
	}

	QColor color = QColor(0xE0, 0xE0, 0xE0, 0xE0);
	painter.setPen(color);
	int latency_x = m_audio_out_latency_in_seconds * m_p_tune_manager->GetPlayingTempo()/60.0 * ONE_BEAT_WIDTH;
	painter.drawLine(QWidget::width()/2 - latency_x, 0, QWidget::width()/2 - latency_x, QWidget::height());
}

/**********************************************************************************/

void NoteDurationWidget::Draw(void)
{
	m_p_scrollbar->setMinimum(m_scrollbar_minimum);
	do {
		if(false == m_is_scrollbar_posistion_corrected){
			QWidget::update();
			break;
		}
		QWidget::repaint();
	} while(0);
}

