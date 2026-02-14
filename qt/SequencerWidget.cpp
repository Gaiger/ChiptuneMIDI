#include <QDebug>
#include <QHBoxLayout>
#include <QPainter>

#include <QScrollBar>

#include "chiptune_midi_define.h"
#include "GetInstrumentNameString.h"

#include "SequencerWidget.h"


#define A0											(21)
#define A4											(69)
#define G9											(127)

#define ONE_NAME_WIDTH								(64)
#define ONE_NAME_HEIGHT								(12)


class NoteNameWidget : public QWidget
{
	//Q_OBJECT
public:
	explicit NoteNameWidget(int drawn_highest_pitch, QWidget *parent = nullptr);
	~NoteNameWidget(void);
private:
	void paintEvent(QPaintEvent  *event) Q_DECL_OVERRIDE;
private:
	int m_drawn_highest_pitch;
};

/**********************************************************************************/

NoteNameWidget::NoteNameWidget(int drawn_highest_pitch, QWidget *parent)
	: QWidget(parent),
	m_drawn_highest_pitch(drawn_highest_pitch)
{
	QSize size = QSize(ONE_NAME_WIDTH + 2, (m_drawn_highest_pitch - A0 + 1) * ONE_NAME_HEIGHT);
	setFixedSize(size);
}

/**********************************************************************************/

NoteNameWidget::~NoteNameWidget(void) { }

/**********************************************************************************/

void NoteNameWidget::paintEvent(QPaintEvent *event)
{
	QWidget::paintEvent(event);
	QPainter painter(this);

#define _A4_IN_SPECIAL_COLOR
#ifdef _A4_IN_SPECIAL_COLOR
	int index_of_A4 = m_drawn_highest_pitch - A4;
	for(int i = 0; i < (m_drawn_highest_pitch - A0 + 1); i++){
		if(i == index_of_A4){
			continue;
		}
		painter.drawRect(QRect(0, i * ONE_NAME_HEIGHT, ONE_NAME_WIDTH, ONE_NAME_HEIGHT));
	}
	QBrush original_brush = painter.brush();
	QColor parent_background_color = QWidget::parentWidget()->palette().color(QWidget::parentWidget()->backgroundRole());
	painter.setBrush(QBrush(parent_background_color.lighter(200)));
	painter.drawRect(QRect(0, index_of_A4 * ONE_NAME_HEIGHT, ONE_NAME_WIDTH, ONE_NAME_HEIGHT));
	painter.setBrush(original_brush);
#else
	for(int i = 0; i < (m_drawn_highest_pitch -  A0 + 1); i++){
		painter.drawRect(QRect(0, i * ONE_NAME_HEIGHT, ONE_NAME_WIDTH, ONE_NAME_HEIGHT));
	}
#endif
	QFont font = painter.font();
#define ADDITIONAL_FONT_POINT_SIZE				  (2)
	font.setPointSize(ONE_NAME_HEIGHT/2 + ADDITIONAL_FONT_POINT_SIZE);
	//font.setBold(true);
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
	for(int i = 3; i < (m_drawn_highest_pitch -  A0 + 1); i++){
		QString note_name_string = note_name_string_list.at(kk % 12);
		QString number_string = QString::number(kk / 12 + 1);
		note_name_string.replace("1", number_string);
		painter.drawText( QPoint(ONE_NAME_WIDTH*1/8, (QWidget::height() - (4 + kk) * ONE_NAME_HEIGHT + ONE_NAME_HEIGHT*3/4) + ADDITIONAL_FONT_POINT_SIZE/2),
						  note_name_string);
		kk += 1;
	}
}

/**********************************************************************************/

class NoteDurationWidget : public QWidget
{
	//Q_OBJECT
public:
	explicit NoteDurationWidget(TuneManager *p_tune_manager, int drawn_highest_pitch,
							 double audio_out_latency_in_seconds = 0.0, QWidget *parent = nullptr);
	~NoteDurationWidget(void);
public :
	void Prepare(int tick_in_center);
	void Update(void);
	void SetChannelDrawAsEnabled(int channel_index, bool is_enabled);
private :
	QRect NoteToQRect(int start_tick, int end_tick, int tick_in_center, int note);
	bool IsTickOutOfRightBound(int tick, int tick_in_center);
	int tickToX(int tick, int const tick_in_center);
	int XtoTick(int x, int const tick_in_center);

	void ReduceRectangles(int preparing_index);
	void DrawChannelRectangles(QPainter *p_painter, bool const is_to_draw_enabled_channels);
private:
	void paintEvent(QPaintEvent  *event) Q_DECL_OVERRIDE;
private:
	TuneManager *m_p_tune_manager;
	int m_drawn_highest_pitch;

	double m_audio_out_latency_in_seconds;

	QList<QRect> m_channel_rectangle_list[2][MIDI_MAX_CHANNEL_NUMBER];
	int m_drawing_channel_rectangle_list_index;
	bool m_is_channel_draw_as_enabled[MIDI_MAX_CHANNEL_NUMBER];

	int m_last_sought_index;
	int m_last_tick_in_center;
	QList<QMidiEvent *> m_ignored_midi_event_ptr_list;
	QMutex m_mutex;
};

/**********************************************************************************/

#define ONE_BEAT_WIDTH					(64)
#define ONE_BEAT_HEIGHT					(ONE_NAME_HEIGHT)

NoteDurationWidget::NoteDurationWidget(TuneManager *p_tune_manager, int drawn_highest_pitch,
								 double audio_out_latency_in_seconds, QWidget *parent) :
	QWidget(parent),
	m_p_tune_manager(p_tune_manager),
	m_drawn_highest_pitch(drawn_highest_pitch),
	m_audio_out_latency_in_seconds(audio_out_latency_in_seconds),
	m_last_sought_index(0),
	m_last_tick_in_center(0)
{
	QSize size = QSize(parent->width() - ONE_NAME_WIDTH * 3 / 2, (m_drawn_highest_pitch - A0 + 1) * ONE_NAME_HEIGHT);
	setFixedSize(size);

	for(int j = 0; j < 2; j++){
		for(int voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
			m_channel_rectangle_list[j][voice].append(QList<QRect>());
		}
	}

	for(int voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		m_is_channel_draw_as_enabled[voice] = true;
	}
	m_drawing_channel_rectangle_list_index = 0;
	m_ignored_midi_event_ptr_list.clear();
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

void NoteDurationWidget::SetChannelDrawAsEnabled(int channel_index, bool is_enabled)
{
	m_is_channel_draw_as_enabled[channel_index] = is_enabled;
}

/**********************************************************************************/

QRect NoteDurationWidget::NoteToQRect(int start_tick, int end_tick, int tick_in_center, int note)
{
	int x = tickToX(start_tick, tick_in_center);
	int y = (m_drawn_highest_pitch - note) * ONE_BEAT_HEIGHT;
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

void NoteDurationWidget::ReduceRectangles(int preparing_channel_rectangle_list_index)
{
	// Reduce the rectangles from out of the widget boundary.
	int const right_endpoint = QWidget::width() - 1;
	for(int voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		QMutableListIterator<QRect> rect_list_iterator(m_channel_rectangle_list[preparing_channel_rectangle_list_index][voice]);
		while(rect_list_iterator.hasNext()){
			rect_list_iterator.next();
			QRect rect = rect_list_iterator.value();

			if(rect.right() < 0){
				rect_list_iterator.remove();
				continue;
			}

			bool is_reduced = false;
			do{
				if(rect.left() < 0){
					rect.setLeft(0);
					is_reduced = true;
				}

				if(right_endpoint < rect.right()){
					rect.setRight(right_endpoint);
					is_reduced = true;
				}
			}while(0);

			if(true == is_reduced){
				rect_list_iterator.setValue(rect);
			}
		}
	}
}

/**********************************************************************************/

void NoteDurationWidget::Prepare(int const tick_in_center)
{
	QMutexLocker locker(&m_mutex);
	int preparing_channel_rectangle_list_index = (m_drawing_channel_rectangle_list_index + 1) % 2;

	for(int voice = 0; voice < MIDI_MAX_CHANNEL_NUMBER; voice++){
		m_channel_rectangle_list[preparing_channel_rectangle_list_index][voice].clear();
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

				bool is_ignored = false;
				QListIterator<QMidiEvent*> ignored_midi_event_ptr_list_interator(m_ignored_midi_event_ptr_list);
				while(ignored_midi_event_ptr_list_interator.hasNext()){
					if(ignored_midi_event_ptr_list_interator.next() == p_event){
						is_ignored = true;
						break;
					}
				}
				if(true == is_ignored){
					break;
				}

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
						m_channel_rectangle_list[preparing_channel_rectangle_list_index][p_draw_note->voice].append(
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
					qDebug() << "WARNING ::"<< Q_FUNC_INFO
							 << ":: dangling off note :"
							 << "tick =" << p_event->tick()
							 << ", voice ="<< p_event->voice()
							 << ", note =" << p_event->note()
							 << "(might be double off)";
					m_ignored_midi_event_ptr_list.append(p_event);
				} while(0);
			}
		}while(0);
	}

	ReduceRectangles(preparing_channel_rectangle_list_index);

	//qDebug() << "sought_index " << sought_index;
	//qDebug() << "start_index_list.size() " << start_index_list.size();
	m_last_tick_in_center = tick_in_center;
	if(INT32_MAX != sought_index){
		m_last_sought_index = sought_index;
	}
	m_drawing_channel_rectangle_list_index = preparing_channel_rectangle_list_index;
}

/**********************************************************************************/

void NoteDurationWidget::DrawChannelRectangles(QPainter *p_painter, bool const is_to_draw_enabled_channels)
{
	for(int voice = MIDI_MAX_CHANNEL_NUMBER - 1; voice >= 0 ; voice--){
		if(is_to_draw_enabled_channels != m_is_channel_draw_as_enabled[voice]){
			continue;
		}

		QColor channel_color = GetChannelColor(voice);
		QColor pen_color = QColor(0xFF, 0xFF, 0xFF, 0xC0);
		if(false == m_is_channel_draw_as_enabled[voice]){
			channel_color.setAlpha(0x30);
			pen_color = pen_color.darker(150);
		}
		p_painter->setBrush(channel_color);
		p_painter->setPen(pen_color);

#if QT_VERSION  >= QT_VERSION_CHECK(6, 0, 0)
		const QList<QRect> &rectangles
				= m_channel_rectangle_list[m_drawing_channel_rectangle_list_index][voice];
#else
		const QList<QRect> &ref_rectangles_list =
			m_channel_rectangle_list[m_drawing_channel_rectangle_list_index][voice];
		const QVector<QRect> rectangles = QVector<QRect>::fromList(ref_rectangles_list);
#endif
		p_painter->drawRects(rectangles.constData(), rectangles.size());
	}
}

/**********************************************************************************/

void NoteDurationWidget::paintEvent(QPaintEvent *event)
{
	QMutexLocker locker(&m_mutex);
	QWidget::paintEvent(event);

	QPainter painter(this);
	DrawChannelRectangles(&painter, false);
	DrawChannelRectangles(&painter, true);

	QColor color = QColor(0xE0, 0xE0, 0xE0, 0xE0);
	painter.setPen(color);
	int latency_x = m_audio_out_latency_in_seconds * m_p_tune_manager->GetPlayingEffectiveTempo()/60.0 * ONE_BEAT_WIDTH;
	painter.drawLine(QWidget::width()/2 - latency_x, 0, QWidget::width()/2 - latency_x, QWidget::height());
}

/**********************************************************************************/

void NoteDurationWidget::Update(void)
{
	do {
		QWidget::repaint();
	} while(0);
}

/**********************************************************************************/
/**********************************************************************************/

SequencerWidget::SequencerWidget(TuneManager *p_tune_manager, double audio_out_latency_in_seconds,
						 QScrollArea *p_parent_scroll_area)
	: QWidget(p_parent_scroll_area),
	  m_p_parent_scroll_area(p_parent_scroll_area)
{
	p_parent_scroll_area->setWidget(this);
	QHBoxLayout *p_layout = new QHBoxLayout(this);
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->setSpacing(0);

	QList<QMidiEvent*> midievent_list = p_tune_manager->GetMidiFilePointer()->events();
	int highest_pitch = A0;
	int lowest_pitch = G9;
	int first_note_pitch = 0;
	for(int i = 0; i < midievent_list.size(); i++){
		QMidiEvent * const p_event = midievent_list.at(i);
		if(QMidiEvent::NoteOn == p_event->type()){

			if(0 == first_note_pitch){
				first_note_pitch = p_event->note();
			}
			if(p_event->note() > highest_pitch){
				highest_pitch = p_event->note();
			}
			if(p_event->note() < lowest_pitch){
				lowest_pitch = p_event->note();
			}
		}
	}
	(void)lowest_pitch;
#define TOP_PADDING_IN_PITCH						(2)
	int drawn_hightest_pitch = highest_pitch;
	do{
		if(G9 - TOP_PADDING_IN_PITCH < drawn_hightest_pitch){
			drawn_hightest_pitch = G9;
			break;
		}
		drawn_hightest_pitch = highest_pitch + TOP_PADDING_IN_PITCH;
	} while(0);

	if((drawn_hightest_pitch - A0 + 1) * ONE_BEAT_HEIGHT < QWidget::height()){
		drawn_hightest_pitch = (QWidget::height()/ONE_BEAT_HEIGHT) + A0 - 1;
	}
	m_p_note_name_widget = new NoteNameWidget(drawn_hightest_pitch, this);
	m_p_note_duration_widget = new NoteDurationWidget(p_tune_manager, drawn_hightest_pitch, audio_out_latency_in_seconds, this);
	p_layout->addWidget(m_p_note_name_widget);
	p_layout->addWidget(m_p_note_duration_widget);
	//p_parent_scroll_area->verticalScrollBar()->setSingleStep(ONE_BEAT_HEIGHT);

	int original_middle_pitch = drawn_hightest_pitch - (p_parent_scroll_area->height()/2/ONE_BEAT_HEIGHT);
	m_vertical_scrolling_shift = (original_middle_pitch - first_note_pitch) * ONE_BEAT_HEIGHT;
	do {
		if(m_vertical_scrolling_shift < 0){
			m_vertical_scrolling_shift = 0;
			break;
		}

		if(m_vertical_scrolling_shift > m_p_note_duration_widget->height() - p_parent_scroll_area->height()){
			m_vertical_scrolling_shift = m_p_note_duration_widget->height() - p_parent_scroll_area->height();
		}
	}while(0);
}

/**********************************************************************************/

SequencerWidget::~SequencerWidget(void)
{
	delete m_p_note_name_widget;
	delete m_p_note_duration_widget;

	QWidget::setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	QWidget::resize(0, 0);
}

/**********************************************************************************/

void SequencerWidget::Prepare(int tick_in_center)
{
	m_p_note_duration_widget->Prepare(tick_in_center);
}

/**********************************************************************************/

void SequencerWidget::Update(void)
{
	if(0 != m_vertical_scrolling_shift){
		m_p_parent_scroll_area->verticalScrollBar()->setValue((0xFFFF & m_vertical_scrolling_shift));
		do {
		// To make the scolling valid, it is necessary to set verticalScrollBar value twice.
		// TODO : to remove the ugly walk-around
#define SHIFT_STAGE_BIT								(30)
			if((0x01 << SHIFT_STAGE_BIT) & m_vertical_scrolling_shift){
				m_vertical_scrolling_shift = 0;
				break;
			}
			m_vertical_scrolling_shift |= (0x01 << SHIFT_STAGE_BIT);
		} while (0);
	}
	m_p_note_duration_widget->Update();
}

/**********************************************************************************/

void SequencerWidget::SetChannelDrawAsEnabled(int channel_index, bool is_enabled)
{
	m_p_note_duration_widget->SetChannelDrawAsEnabled(channel_index, is_enabled);
}
