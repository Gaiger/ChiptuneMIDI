#include <QColor>
#include <functional>
#include <QList>
#include <QPainter>
#include <QScrollBar>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

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

NoteNameWidget::~NoteNameWidget(void)
{
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

class NoteDurationWidget : public QWidget
{
public:
	explicit NoteDurationWidget(QWidget *parent = nullptr);
	~NoteDurationWidget(void);
};

/**********************************************************************************/

NoteDurationWidget::NoteDurationWidget(QWidget *parent)
	: QWidget(parent)
{
	QSize size = QSize((G9 - A0 + 1) * ONE_NAME_WIDTH, NOTE_DURATION_WIDGET_HEIGHT);
	setFixedSize(size);
}

/**********************************************************************************/

NoteDurationWidget::~NoteDurationWidget(void)
{
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

	p_layout->addWidget(m_p_note_duration_widget);
	p_layout->addWidget(m_p_note_name_widget);

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
	delete m_p_note_name_widget;
	delete m_p_note_duration_widget;

	QWidget::setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	QWidget::resize(0, 0);
}
