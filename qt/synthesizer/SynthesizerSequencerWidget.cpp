#include <QColor>
#include <functional>
#include <QGridLayout>
#include <QList>
#include <QMutableListIterator>
#include <QPainter>
#include <QScrollBar>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QTimer>

#include "chiptune_midi_define.h"

#include "ChiptuneMidiValues.h"
#include "SynthesizerSequencerWidget.h"

#define A0											(21)
#define A4											(69)
#define G9											(127)

#define WATERFALL_ONE_NAME_WIDTH					(20)
#define WATERFALL_ONE_NAME_HEIGHT					(32)

#define ROLL_ONE_NAME_WIDTH							(64)
#define ROLL_ONE_NAME_HEIGHT						(12)
#define ROLL_ONE_SECOND_WIDTH						(128)
#define ROLL_ADDITIONAL_FONT_POINT_SIZE				(2)

#define NOTE_NAME_PAINT_MARGIN						(2)
#define NOTE_DURATION_RECTANGLE_OVERSCAN			(4)

class NoteNameWidget : public QWidget
{
public:
	explicit NoteNameWidget(QWidget * const parent = nullptr);
	~NoteNameWidget(void);
	void SetViewMode(SynthesizerSequencerWidget::ViewMode const view_mode);
private:
	void paintEvent(QPaintEvent * const event) Q_DECL_OVERRIDE;
private:
	SynthesizerSequencerWidget::ViewMode m_view_mode;
};

/**********************************************************************************/
NoteNameWidget::NoteNameWidget(QWidget * const parent)
	: QWidget(parent),
	  m_view_mode(SynthesizerSequencerWidget::ViewModeWaterfall)
{
	QSize const size = QSize((G9 - A0 + 1) * WATERFALL_ONE_NAME_WIDTH,
							 WATERFALL_ONE_NAME_HEIGHT + NOTE_NAME_PAINT_MARGIN);
	setFixedSize(size);
}

/**********************************************************************************/

NoteNameWidget::~NoteNameWidget(void){}

/**********************************************************************************/
void NoteNameWidget::SetViewMode(SynthesizerSequencerWidget::ViewMode const view_mode)
{
	m_view_mode = view_mode;
	QSize note_name_widget_size = QSize(
				(G9 - A0 + 1) * WATERFALL_ONE_NAME_WIDTH,
				WATERFALL_ONE_NAME_HEIGHT + NOTE_NAME_PAINT_MARGIN);
	if(SynthesizerSequencerWidget::ViewModeRoll == m_view_mode){
		note_name_widget_size = QSize(
					ROLL_ONE_NAME_WIDTH + NOTE_NAME_PAINT_MARGIN,
					(G9 - A0 + 1) * ROLL_ONE_NAME_HEIGHT);
	}
	QWidget::setFixedSize(note_name_widget_size);
	QWidget::update();
}

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
void NoteNameWidget::paintEvent(QPaintEvent * const event)
{
	QWidget::paintEvent(event);
	QPainter painter(this);

	int note_name_width = WATERFALL_ONE_NAME_WIDTH;
	int note_name_height = WATERFALL_ONE_NAME_HEIGHT;
	int font_point_size = WATERFALL_ONE_NAME_WIDTH / 2;
	if(SynthesizerSequencerWidget::ViewModeRoll == m_view_mode){
		note_name_width = ROLL_ONE_NAME_WIDTH;
		note_name_height = ROLL_ONE_NAME_HEIGHT;
		font_point_size = ROLL_ONE_NAME_HEIGHT / 2 + ROLL_ADDITIONAL_FONT_POINT_SIZE;
	}

	QBrush const original_brush = painter.brush();
	QColor const parent_background_color
			= QWidget::parentWidget()->palette().color(QWidget::parentWidget()->backgroundRole());
	QFont font = painter.font();

	for(int note_number = A0; note_number <= G9; note_number++){
		int index = note_number - A0;
		if(SynthesizerSequencerWidget::ViewModeRoll == m_view_mode){
			index = G9 - note_number;
		}

		int x = index * note_name_width;
		int y = 0;
		if(SynthesizerSequencerWidget::ViewModeRoll == m_view_mode){
			x = 0;
			y = index * note_name_height;
		}
		QRect note_name_rect = QRect(x, y,
									 note_name_width, note_name_height);

		painter.setBrush(original_brush);
		if(note_number == A4){
			painter.setBrush(QBrush(parent_background_color.lighter(200)));
		}
		painter.drawRect(note_name_rect);

		QString note_name_string = GetNoteNameString(note_number);
		if(SynthesizerSequencerWidget::ViewModeRoll == m_view_mode){
			note_name_string.replace("\n", "/");
		}

		font.setPointSize(font_point_size);
		if((SynthesizerSequencerWidget::ViewModeWaterfall == m_view_mode)
				&& (true == note_name_string.contains("\n"))){
			font.setPointSize(WATERFALL_ONE_NAME_WIDTH / 2 - 3);
		}
		painter.setFont(font);
		painter.drawText(note_name_rect, Qt::AlignCenter, note_name_string);
	}
}

/**********************************************************************************/
#define WATERFALL_ONE_SECOND_HEIGHT					(128)
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
	explicit NoteDurationWidget(QWidget * const parent = nullptr);
	~NoteDurationWidget(void);
	void AddIncomingNoteEvent(SynthesizerSequencerNoteEvent const &note_event);
	void ApplyAllNotesOff(int const channel_index, qint64 const current_timestamp_in_ms);
	void SetViewMode(SynthesizerSequencerWidget::ViewMode const view_mode);
	void Update(void);
	void Prepare(qint64 const current_timestamp_in_ms);
private:
	void ProcessIncomingNoteEvents(void);
	int WaterfallTimestampToY(qint64 const mapped_timestamp_in_ms,
							  qint64 const current_timestamp_in_ms) const;
	int RollTimestampToX(qint64 const mapped_timestamp_in_ms,
						 qint64 const current_timestamp_in_ms) const;
	QRect WaterfallNoteToQRect(draw_note_t const &draw_note,
							   qint64 const current_timestamp_in_ms) const;
	QRect RollNoteToQRect(draw_note_t const &draw_note,
						  qint64 const current_timestamp_in_ms) const;
	void DrawChannelRectangles(QPainter * const p_painter);
	void paintEvent(QPaintEvent * const event) Q_DECL_OVERRIDE;
private:
	QList<SynthesizerSequencerNoteEvent> m_incoming_note_event_list;
	QList<draw_note_t> m_draw_note_list;
	QList<QRect> m_channel_rectangle_list[2][MIDI_MAX_CHANNEL_NUMBER];
	int m_drawing_channel_rectangle_list_index;
	SynthesizerSequencerWidget::ViewMode m_view_mode;
};

/**********************************************************************************/
NoteDurationWidget::NoteDurationWidget(QWidget * const parent)
	: QWidget(parent),
	  m_drawing_channel_rectangle_list_index(0),
	  m_view_mode(SynthesizerSequencerWidget::ViewModeWaterfall)
{
	SetViewMode(m_view_mode);

	for(int j = 0; j < 2; j++){
		for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; channel_index++){
			m_channel_rectangle_list[j][channel_index].append(QList<QRect>());
		}
	}

}

/**********************************************************************************/
NoteDurationWidget::~NoteDurationWidget(void){}

/**********************************************************************************/
int NoteDurationWidget::WaterfallTimestampToY(qint64 const mapped_timestamp_in_ms,
											  qint64 const current_timestamp_in_ms) const
{
	if(DRAW_NOTE_UNBOUNDED_END_TIMESTAMP_IN_MS == mapped_timestamp_in_ms){
		return -NOTE_DURATION_RECTANGLE_OVERSCAN;
	}

	qint64 const elapsed_timestamp_in_ms = current_timestamp_in_ms - mapped_timestamp_in_ms;
	int const y = (int)(elapsed_timestamp_in_ms * WATERFALL_ONE_SECOND_HEIGHT / 1000);
	return y;
}

/**********************************************************************************/
QRect NoteDurationWidget::WaterfallNoteToQRect(
		draw_note_t const &draw_note,
		qint64 const current_timestamp_in_ms) const
{
	int const x = (draw_note.note_number - A0) * WATERFALL_ONE_NAME_WIDTH;
	int const start_timestamp_y = WaterfallTimestampToY(draw_note.start_timestamp_in_ms,
														current_timestamp_in_ms);
	int const end_timestamp_y = WaterfallTimestampToY(draw_note.end_timestamp_in_ms,
													  current_timestamp_in_ms);
	return QRect(x, end_timestamp_y,
				 WATERFALL_ONE_NAME_WIDTH, start_timestamp_y - end_timestamp_y);
}

/**********************************************************************************/
int NoteDurationWidget::RollTimestampToX(qint64 const mapped_timestamp_in_ms,
										 qint64 const current_timestamp_in_ms) const
{
	if(DRAW_NOTE_UNBOUNDED_END_TIMESTAMP_IN_MS == mapped_timestamp_in_ms){
		return QWidget::width() + NOTE_DURATION_RECTANGLE_OVERSCAN;
	}

	qint64 const elapsed_timestamp_in_ms = current_timestamp_in_ms - mapped_timestamp_in_ms;
	int const x = QWidget::width()
			- (int)(elapsed_timestamp_in_ms * ROLL_ONE_SECOND_WIDTH / 1000);
	return x;
}

/**********************************************************************************/
QRect NoteDurationWidget::RollNoteToQRect(
		draw_note_t const &draw_note,
		qint64 const current_timestamp_in_ms) const
{
	int const end_timestamp_x = RollTimestampToX(draw_note.end_timestamp_in_ms,
												 current_timestamp_in_ms);
	int const start_timestamp_x = RollTimestampToX(draw_note.start_timestamp_in_ms,
												   current_timestamp_in_ms);
	int const y = (G9 - draw_note.note_number) * ROLL_ONE_NAME_HEIGHT;
	return QRect(start_timestamp_x, y,
				 end_timestamp_x - start_timestamp_x, ROLL_ONE_NAME_HEIGHT);
}

/**********************************************************************************/
void NoteDurationWidget::AddIncomingNoteEvent(SynthesizerSequencerNoteEvent const &note_event)
{
	m_incoming_note_event_list.append(note_event);
}

/**********************************************************************************/
void NoteDurationWidget::ApplyAllNotesOff(int const channel_index,
										  qint64 const current_timestamp_in_ms)
{
	ProcessIncomingNoteEvents();
	for(int i = 0; i < m_draw_note_list.size(); i++){
		draw_note_t * const p_draw_note = &m_draw_note_list[i];
		if(p_draw_note->channel_index != channel_index){
			continue;
		}
		if(DRAW_NOTE_UNBOUNDED_END_TIMESTAMP_IN_MS != p_draw_note->end_timestamp_in_ms){
			continue;
		}
		p_draw_note->end_timestamp_in_ms = current_timestamp_in_ms;
	}
}

/**********************************************************************************/
void NoteDurationWidget::SetViewMode(SynthesizerSequencerWidget::ViewMode const view_mode)
{
	m_view_mode = view_mode;

	QSize note_duration_widget_size =
			QSize((G9 - A0 + 1) * WATERFALL_ONE_NAME_WIDTH,
				  (G9 - A0 + 1) * ROLL_ONE_NAME_HEIGHT);

	QScrollArea const *p_parent_scroll_area = nullptr;
	QObject const *p_parent_object = QObject::parent();
	while(nullptr != p_parent_object)
	{
		QScrollArea const * const p_scroll_area =
						qobject_cast<QScrollArea const *>(p_parent_object);
		if(nullptr != p_scroll_area){
				p_parent_scroll_area = p_scroll_area;
				break;
		}
		p_parent_object = p_parent_object->parent();
	}

	do
	{
		if(nullptr == p_parent_scroll_area){
			break;
		}

		note_duration_widget_size = QSize(
					(G9 - A0 + 1) * WATERFALL_ONE_NAME_WIDTH,
					p_parent_scroll_area->maximumViewportSize().height()
					  - (WATERFALL_ONE_NAME_HEIGHT + NOTE_NAME_PAINT_MARGIN)
					  - p_parent_scroll_area->horizontalScrollBar()->sizeHint().height());
		if(SynthesizerSequencerWidget::ViewModeRoll == view_mode){
			note_duration_widget_size = QSize(
						p_parent_scroll_area->maximumViewportSize().width()
						  - (ROLL_ONE_NAME_WIDTH + NOTE_NAME_PAINT_MARGIN)
						  - p_parent_scroll_area->verticalScrollBar()->sizeHint().width(),
						(G9 - A0 + 1) * ROLL_ONE_NAME_HEIGHT);
		}
	}while(0);

	QWidget::setFixedSize(note_duration_widget_size);
	QWidget::update();
}

/**********************************************************************************/
void NoteDurationWidget::ProcessIncomingNoteEvents(void)
{
	while(false == m_incoming_note_event_list.isEmpty())
	{
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

			for(int i = m_draw_note_list.size() - 1; i >= 0; i -= 1){
				draw_note_t * const p_draw_note = &m_draw_note_list[i];
				if(DRAW_NOTE_UNBOUNDED_END_TIMESTAMP_IN_MS != p_draw_note->end_timestamp_in_ms){
					continue;
				}
				if(note_event.GetChannelIndex() != p_draw_note->channel_index){
					continue;
				}
				if(note_event.GetNoteNumber() != p_draw_note->note_number){
					continue;
				}
				p_draw_note->end_timestamp_in_ms = note_event.GetTimestampInMilliseconds();
				break;
			}
		}while(0);
	}
}

/**********************************************************************************/
void NoteDurationWidget::Prepare(qint64 const current_timestamp_in_ms)
{
	int const preparing_channel_rectangle_list_index
			= (m_drawing_channel_rectangle_list_index + 1) % 2;
	for(int channel_index = 0; channel_index < MIDI_MAX_CHANNEL_NUMBER; channel_index++){
		m_channel_rectangle_list[preparing_channel_rectangle_list_index][channel_index].clear();
	}

	ProcessIncomingNoteEvents();
	QMutableListIterator<draw_note_t> draw_note_iterator(m_draw_note_list);
	while(draw_note_iterator.hasNext())
	{
		draw_note_t const draw_note = draw_note_iterator.next();
		do
		{
			if(draw_note.note_number < A0 || G9 < draw_note.note_number){
				break;
			}

			bool is_note_out_of_boundary = false;
			QRect note_rect;
			do
			{
				if(SynthesizerSequencerWidget::ViewModeRoll == m_view_mode){
					note_rect = RollNoteToQRect(draw_note, current_timestamp_in_ms);
					if(-NOTE_DURATION_RECTANGLE_OVERSCAN > note_rect.right()){
						is_note_out_of_boundary = true;
					}
					break;
				}
				note_rect = WaterfallNoteToQRect(draw_note, current_timestamp_in_ms);
				if(QWidget::height() + NOTE_DURATION_RECTANGLE_OVERSCAN < note_rect.top()){
					is_note_out_of_boundary = true;
				}
			}while(0);
			if(true == is_note_out_of_boundary){
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
void NoteDurationWidget::DrawChannelRectangles(QPainter * const p_painter)
{
	for(int channel_index = MIDI_MAX_CHANNEL_NUMBER - 1; channel_index >= 0; channel_index -= 1){
		QColor const channel_color = GetChannelColor(channel_index);
		QColor const pen_color = QColor(0xFF, 0xFF, 0xFF, 0xC0);
		p_painter->setBrush(channel_color);
		p_painter->setPen(pen_color);

#if QT_VERSION  >= QT_VERSION_CHECK(6, 0, 0)
		QList<QRect> const &rectangles
				= m_channel_rectangle_list[m_drawing_channel_rectangle_list_index][channel_index];
#else
		QList<QRect> const &ref_rectangles_list =
			m_channel_rectangle_list[m_drawing_channel_rectangle_list_index][channel_index];
		QVector<QRect> const rectangles = QVector<QRect>::fromList(ref_rectangles_list);
#endif
		p_painter->drawRects(rectangles.constData(), rectangles.size());
	}
}

/**********************************************************************************/
void NoteDurationWidget::paintEvent(QPaintEvent * const event)
{
	QWidget::paintEvent(event);

	QPainter painter(this);
	DrawChannelRectangles(&painter);
}

/**********************************************************************************/
SynthesizerSequencerWidget::SynthesizerSequencerWidget(QScrollArea * const p_parent_scroll_area)
	: QWidget(p_parent_scroll_area),
	  m_p_note_name_widget(nullptr),
	  m_p_note_duration_widget(nullptr),
	  m_p_parent_scroll_area(p_parent_scroll_area),
	  m_p_layout(nullptr),
	  m_view_mode(ViewModeWaterfall)
{
	p_parent_scroll_area->setWidget(this);
	m_p_layout = new QGridLayout(this);
	m_p_layout->setContentsMargins(0, 0, 0, 0);
	m_p_layout->setSpacing(0);

	m_p_note_duration_widget = new NoteDurationWidget(this);
	m_p_note_name_widget = new NoteNameWidget(this);

	ApplyViewModeLayout();
	std::function<void()> const set_A4_center_function = [this]() {
		QScrollBar *p_scroll_bar = m_p_parent_scroll_area->horizontalScrollBar();
		int A4_center_position = (A4 - A0) * WATERFALL_ONE_NAME_WIDTH
				+ WATERFALL_ONE_NAME_WIDTH / 2;
		int viewport_size = m_p_parent_scroll_area->viewport()->width();
		if(ViewModeRoll == m_view_mode){
			p_scroll_bar = m_p_parent_scroll_area->verticalScrollBar();
			A4_center_position = (G9 - A4) * ROLL_ONE_NAME_HEIGHT
					+ ROLL_ONE_NAME_HEIGHT / 2;
			viewport_size = m_p_parent_scroll_area->viewport()->height();
		}
		int const A4_scroll_value = A4_center_position - viewport_size / 2;
		p_scroll_bar->setValue(A4_scroll_value);
	};
	QTimer::singleShot(0, m_p_parent_scroll_area, set_A4_center_function);
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
void SynthesizerSequencerWidget::SendAllNotesOffEvent(int const channel_index,
													  qint64 const current_timestamp_in_ms)
{
	m_p_note_duration_widget->ApplyAllNotesOff(channel_index, current_timestamp_in_ms);
}

/**********************************************************************************/
void SynthesizerSequencerWidget::SetViewMode(ViewMode const view_mode)
{
	do
	{
		if(m_view_mode == view_mode){
			break;
		}

		QScrollBar const *p_source_scroll_bar = m_p_parent_scroll_area->horizontalScrollBar();
		double source_center_note_number =
				A0 + ((double)p_source_scroll_bar->value()
					  + (double)m_p_parent_scroll_area->viewport()->width() / 2)
				/ WATERFALL_ONE_NAME_WIDTH;
		if(ViewModeRoll == m_view_mode){
			p_source_scroll_bar = m_p_parent_scroll_area->verticalScrollBar();
			source_center_note_number =
					G9 - ((double)p_source_scroll_bar->value()
						  + (double)m_p_parent_scroll_area->viewport()->height() / 2)
					/ ROLL_ONE_NAME_HEIGHT;
		}
		std::function<void()> const restore_scroll_position_function =
				[this, view_mode, source_center_note_number]() {
			QScrollBar *p_target_scroll_bar = m_p_parent_scroll_area->horizontalScrollBar();
			double target_scroll_value =
					(source_center_note_number - A0) * WATERFALL_ONE_NAME_WIDTH
					- (double)m_p_parent_scroll_area->viewport()->width() / 2;
			if(ViewModeRoll == view_mode){
				p_target_scroll_bar = m_p_parent_scroll_area->verticalScrollBar();
				target_scroll_value =
						(G9 - source_center_note_number) * ROLL_ONE_NAME_HEIGHT
						- (double)m_p_parent_scroll_area->viewport()->height() / 2;
			}
			p_target_scroll_bar->setValue((int)target_scroll_value);
		};

		QScrollBar *p_target_scroll_bar = m_p_parent_scroll_area->horizontalScrollBar();
		if(ViewModeRoll == view_mode){
			p_target_scroll_bar = m_p_parent_scroll_area->verticalScrollBar();
		}
		std::function<void(int const, int const)> const restore_scroll_position_slot_function =
				[p_target_scroll_bar, this,
				 restore_scroll_position_function]
				(int const minimum, int const maximum) {
			Q_UNUSED(minimum);
			Q_UNUSED(maximum);
			QObject::disconnect(p_target_scroll_bar, SIGNAL(rangeChanged(int,int)),
								this, nullptr);
			restore_scroll_position_function();
		};
		QObject::connect(p_target_scroll_bar, &QScrollBar::rangeChanged,
						 this, restore_scroll_position_slot_function);

		m_view_mode = view_mode;
		m_p_note_name_widget->SetViewMode(view_mode);
		m_p_note_duration_widget->SetViewMode(view_mode);
		ApplyViewModeLayout();
	}while(0);
}

/**********************************************************************************/
void SynthesizerSequencerWidget::ApplyViewModeLayout(void)
{
	m_p_layout->removeWidget(m_p_note_name_widget);
	m_p_layout->removeWidget(m_p_note_duration_widget);

	do
	{
		if(ViewModeRoll == m_view_mode){
			m_p_layout->addWidget(m_p_note_duration_widget, 0, 0);
			m_p_layout->addWidget(m_p_note_name_widget, 0, 1);
			break;
		}

		m_p_layout->addWidget(m_p_note_name_widget, 0, 0);
		m_p_layout->addWidget(m_p_note_duration_widget, 1, 0);
	}while(0);
}

/**********************************************************************************/
void SynthesizerSequencerWidget::Update(void)
{
	m_p_note_duration_widget->Update();
}

/**********************************************************************************/
void SynthesizerSequencerWidget::Prepare(qint64 const current_timestamp_in_ms)
{
	m_p_note_duration_widget->Prepare(current_timestamp_in_ms);
}
