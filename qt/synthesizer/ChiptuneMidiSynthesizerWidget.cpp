#include "ui_ChiptuneMidiSynthesizerWidgetForm.h"

#include "ChiptuneMidiSynthesizerWidget.h"

/**********************************************************************************/

ChiptuneMidiSynthesizerWidget::ChiptuneMidiSynthesizerWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::ChiptuneMidiSynthesizerWidget)
{
	ui->setupUi(this);
}

/**********************************************************************************/

ChiptuneMidiSynthesizerWidget::~ChiptuneMidiSynthesizerWidget()
{
	delete ui;
}
