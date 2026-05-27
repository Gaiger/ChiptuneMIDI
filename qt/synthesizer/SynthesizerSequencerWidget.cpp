#include <QColor>
#include <QDateTime>
#include <functional>
#include <QList>
#include <QMutableListIterator>
#include <QPainter>
#include <QScrollBar>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

#include "chiptune_midi_define.h"

#include "ChiptuneMidiValues.h"
#include "SynthesizerSequencerWidget.h"

#define A0											(21)
#define A4											(69)
#define G9											(127)

#define ONE_NAME_WIDTH								(20)
#define ONE_NAME_HEIGHT								(32)
#define NOTE_DURATION_WIDGET_HEIGHT					(552)

class NoteNameWidget : public QWidget
{
public:
	explicit NoteNameWidget(QWidget *parent = nullptr);
	~NoteNameWidget(void);
private:
	void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
};

/**********************************************************************************/
NoteNameWidget::NoteNameWidget(QWidget *parent)
	: QWidget(parent)
{
	QSize size = QSize((G9 - A0 + 1) * ONE_NAME_WIDTH, ONE_NAME_HEIGHT + 2);
	setFixedSize(size);
}

/**********************************************************************************/

NoteNameWidget::~NoteNameWidget(void){}

/**********************************************************************************/
static QString GetNoteNameString(int const note_number)
{
	QList<QString> note_name_string_list;
	note_name_string_list.append("C%1");
	note_name_string_list.append("C#%1\nDb%1");
	note_name_string_list.append("D%1");
	note_name_string_list.append("D#%1\nEb%1");
	note_name_string_list.append("E%1");
	note_name_string_list.append("F%1");
	note_name_string_list.append("F#%1\nGb%1");
	note_name_string_list.append("G%1");
	note_name_string_list.append("G#%1\nAb%1");
	note_name_string_list.append("A%1");
	note_name_string_list.append("A#%1\nBb%1");
	note_name_string_list.append("B%1");

	int const note_index = note_number % 12;
	int const octave_index = note_number / 12 - 1;
	return note_name_string_list.at(note_index).arg(octave_index);
}

/**********************************************************************************/
void NoteNameWidget::paintEvent(QPaintEvent *event)
{
	QWidget::paintEvent(event);
	QPainter painter(this);

	int const index_of_A4 = A4 - A0;
	QBrush const original_brush = painter.brush();
	QColor const parent_background_color
			= QWidget::parentWidget()->palette().color(QWidget::parentWidget()->backgroundRole());
	for(int i = 0; i < (G9 - A0 + 1); i++){
		QRect const note_name_rect(i * ONE_NAME_WIDTH, 0, ONE_NAME_WIDTH, ONE_NAME_HEIGHT);
		if(i == index_of_A4){
			painter.setBrush(QBrush(parent_background_color.lighter(200)));
			painter.drawRect(note_name_rect);
			painter.setBrush(original_brush);
			continue;
		}
		painter.drawRect(note_name_rect);
	}

	QFont font = painter.font();
	font.setPointSize(ONE_NAME_WIDTH / 2);
	painter.setFont(font);

	for(int note_number = A0; note_number <= G9; note_number++){
		int const note_index = note_number - A0;
		QString const note_name_string = GetNoteNameString(note_number);
		if(true == note_name_string.contains("\n")){
			font.setPointSize(ONE_NAME_WIDTH / 2 - 3);
			painter.setFont(font);
		}
		painter.drawText(QRect(note_index * ONE_NAME_WIDTH, 0, ONE_NAME_WIDTH, ONE_NAME_HEIGHT),
						 Qt::AlignCenter,
						 note_name_string);
		if(true == note_name_string.contains("\n")){
			font.setPointSize(ONE_NAME_WIDTH / 2);
			painter.setFont(font);
		}
	}
}

/**********************************************************************************/
#define ONE_SECOND_HEIGHT							(128)
#define DRAW_NOTE_UNBOUNDED_END_TIMESTAMP_IN_MS		(INT64_MAX)

typedef struct
{
	qint64 start_timestamp_in_ms;
	qint64 end_timestamp_in_ms;
	int note_number;
	int channel_index;
}draw_note_t;

class NoteDurationWidget : public QWidget
{
public:
	explicit NoteDurationWidget(QWidget *parent = nullptr);
	~NoteDurationWidget(void);
	void AddIncomingNoteEvent(SynthesizerSequencerNoteEvent const &note_event);
	void ApplyAllNotesOff(int channel_index, qint64 timestamp_in_ms);
	void Update(void);
	void Prepare(void);
private:
	int TimestampToY(qint64 timestamp_in_ms) const;
	QRect NoteToQRect(draw_note_t const &draw_note) const;
	void DrawChannelRectangles(QPainter *p_painter);
	void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
private:
	QList<SynthesizerSequencerNoteEvent> m_incoming_note_event_list;
	QList<draw_note_t> m_draw_note_list;
	QList<QRect> m_channel_rectangle_list[2][MIDI_MAX_CHANNEL_NUMBER];
	int m_drawing_channel_rectangle_list_index;
};

/**********************************************************************************/
NoteDurationWidget::NoteDurationWidget(QWidget *parent)
	: QWidget(parent),
	  m_drawing_channel_rectangle_list_index(0)
{
	QSize size = QSize((G9 - A0 + 1) * ONE_NAME_WIDTH, NOTE_DURATION_WIDGET_HEIGHT);
	setFixedSize(size);

	for(int j = 0; j < 2; j++){
		for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; channel_index++){
			m_channel_rectangle_list[j][channel_index].append(QList<QRect>());
		}
	}

}

/**********************************************************************************/
NoteDurationWidget::~NoteDurationWidget(void){}

/**********************************************************************************/
int NoteDurationWidget::TimestampToY(qint64 timestamp_in_ms) const
{
#define DRAW_NOTE_UNBOUNDED_TOP_Y					(-4)
	if(DRAW_NOTE_UNBOUNDED_END_TIMESTAMP_IN_MS == timestamp_in_ms){
		return DRAW_NOTE_UNBOUNDED_TOP_Y;
	}

	qint64 const current_timestamp_in_ms = QDateTime::currentMSecsSinceEpoch();
	qint64 const elapsed_timestamp_in_ms = current_timestamp_in_ms - timestamp_in_ms;
	int const y = (int)(elapsed_timestamp_in_ms * ONE_SECOND_HEIGHT / 1000);
	return y;
}

/**********************************************************************************/
QRect NoteDurationWidget::NoteToQRect(draw_note_t const &draw_note) const
{
	int const x = (draw_note.note_number - A0) * ONE_NAME_WIDTH;
	int const start_y = TimestampToY(draw_note.start_timestamp_in_ms);
	int const end_y = TimestampToY(draw_note.end_timestamp_in_ms);
	return QRect(x, end_y, ONE_NAME_WIDTH, start_y - end_y);
}

/**********************************************************************************/
void NoteDurationWidget::AddIncomingNoteEvent(SynthesizerSequencerNoteEvent const &note_event)
{
	m_incoming_note_event_list.append(note_event);
}

/**********************************************************************************/
void NoteDurationWidget::ApplyAllNotesOff(int channel_index, qint64 timestamp_in_ms)
{
	Prepare();

	for(int draw_note_index = 0; draw_note_index < m_draw_note_list.size(); draw_note_index++){
		draw_note_t * const p_draw_note = &m_draw_note_list[draw_note_index];
		if(p_draw_note->channel_index != channel_index){
			continue;
		}
		if(DRAW_NOTE_UNBOUNDED_END_TIMESTAMP_IN_MS != p_draw_note->end_timestamp_in_ms){
			continue;
		}
		p_draw_note->end_timestamp_in_ms = timestamp_in_ms;
	}
}

/**********************************************************************************/
void NoteDurationWidget::Prepare(void)
{
	int const preparing_channel_rectangle_list_index
			= (m_drawing_channel_rectangle_list_index + 1) % 2;
	for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; channel_index++){
		m_channel_rectangle_list[preparing_channel_rectangle_list_index][channel_index].clear();
	}

	while(false == m_incoming_note_event_list.isEmpty()){
		SynthesizerSequencerNoteEvent const note_event = m_incoming_note_event_list.takeFirst();
		do
		{
			if(SynthesizerSequencerNoteEvent::NoteOn == note_event.GetNoteState()){
				draw_note_t draw_note;
				draw_note.start_timestamp_in_ms = note_event.GetTimestampInMilliseconds();
				draw_note.end_timestamp_in_ms = DRAW_NOTE_UNBOUNDED_END_TIMESTAMP_IN_MS;
				draw_note.note_number = note_event.GetNoteNumber();
				draw_note.channel_index = note_event.GetChannelIndex();
				m_draw_note_list.append(draw_note);
				break;
			}

			for(int draw_note_index = m_draw_note_list.size() - 1;
				draw_note_index >= 0;
				draw_note_index -= 1){
				draw_note_t * const p_draw_note = &m_draw_note_list[draw_note_index];
				if(DRAW_NOTE_UNBOUNDED_END_TIMESTAMP_IN_MS != p_draw_note->end_timestamp_in_ms){
					continue;
				}
				if(p_draw_note->channel_index != note_event.GetChannelIndex()){
					continue;
				}
				if(p_draw_note->note_number != note_event.GetNoteNumber()){
					continue;
				}
				p_draw_note->end_timestamp_in_ms = note_event.GetTimestampInMilliseconds();
				break;
			}
		}while(0);
	}

	QMutableListIterator<draw_note_t> draw_note_iterator(m_draw_note_list);
	while(draw_note_iterator.hasNext()){
		draw_note_t const draw_note = draw_note_iterator.next();

		do
		{
			if(draw_note.note_number < A0 || G9 < draw_note.note_number){
				break;
			}

			QRect const note_rect = NoteToQRect(draw_note);
			if(note_rect.top() > QWidget::height()){
				draw_note_iterator.remove();
				break;
			}
			m_channel_rectangle_list[preparing_channel_rectangle_list_index][draw_note.channel_index]
					.append(note_rect);
		}while(0);
	}
	m_drawing_channel_rectangle_list_index = preparing_channel_rectangle_list_index;
}

/**********************************************************************************/
void NoteDurationWidget::Update(void)
{
	do
	{
		QWidget::repaint();
	}while(0);
}

/**********************************************************************************/
void NoteDurationWidget::DrawChannelRectangles(QPainter *p_painter)
{
	for(int channel_index = MIDI_MAX_CHANNEL_NUMBER - 1; channel_index >= 0; channel_index -= 1){
		QColor channel_color = GetChannelColor(channel_index);
		QColor const pen_color = QColor(0xFF, 0xFF, 0xFF, 0xC0);
		p_painter->setBrush(channel_color);
		p_painter->setPen(pen_color);

#if QT_VERSION  >= QT_VERSION_CHECK(6, 0, 0)
		const QList<QRect> &rectangles
				= m_channel_rectangle_list[m_drawing_channel_rectangle_list_index][channel_index];
#else
		const QList<QRect> &ref_rectangles_list =
			m_channel_rectangle_list[m_drawing_channel_rectangle_list_index][channel_index];
		const QVector<QRect> rectangles = QVector<QRect>::fromList(ref_rectangles_list);
#endif
		p_painter->drawRects(rectangles.constData(), rectangles.size());
	}
}

/**********************************************************************************/
void NoteDurationWidget::paintEvent(QPaintEvent *event)
{
	QWidget::paintEvent(event);

	QPainter painter(this);
	DrawChannelRectangles(&painter);
}

/**********************************************************************************/
SynthesizerSequencerWidget::SynthesizerSequencerWidget(QScrollArea *p_parent_scroll_area)
	: QWidget(p_parent_scroll_area),
	  m_p_note_name_widget(nullptr),
	  m_p_note_duration_widget(nullptr)
{
	p_parent_scroll_area->setWidget(this);
	QVBoxLayout *p_layout = new QVBoxLayout(this);
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->setSpacing(0);

	m_p_note_duration_widget = new NoteDurationWidget(this);
	m_p_note_name_widget = new NoteNameWidget(this);

	p_layout->addWidget(m_p_note_name_widget);
	p_layout->addWidget(m_p_note_duration_widget);

	std::function<void()> set_A4_center_function = [p_parent_scroll_area]() {
		int const A4_center_x = (A4 - A0) * ONE_NAME_WIDTH + ONE_NAME_WIDTH / 2;
		int const horizontal_scroll_value = A4_center_x - p_parent_scroll_area->viewport()->width() / 2;
		p_parent_scroll_area->horizontalScrollBar()->setValue(horizontal_scroll_value);
	};
	QTimer::singleShot(0, p_parent_scroll_area, set_A4_center_function);
}

/**********************************************************************************/
SynthesizerSequencerWidget::~SynthesizerSequencerWidget(void)
{
	delete m_p_note_name_widget; m_p_note_name_widget = nullptr;
	delete m_p_note_duration_widget; m_p_note_duration_widget = nullptr;

	QWidget::setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	QWidget::resize(0, 0);
}

/**********************************************************************************/
void SynthesizerSequencerWidget::SendNoteEvent(SynthesizerSequencerNoteEvent const &note_event)
{
	m_p_note_duration_widget->AddIncomingNoteEvent(note_event);
}

/**********************************************************************************/
void SynthesizerSequencerWidget::SendAllNotesOffEvent(int channel_index, qint64 timestamp_in_ms)
{
	m_p_note_duration_widget->ApplyAllNotesOff(channel_index, timestamp_in_ms);
}

/**********************************************************************************/
void SynthesizerSequencerWidget::Update(void)
{
	m_p_note_duration_widget->Update();
}

/**********************************************************************************/
void SynthesizerSequencerWidget::Prepare(void)
{
	m_p_note_duration_widget->Prepare();
}
