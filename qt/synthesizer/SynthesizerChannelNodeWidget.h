#ifndef _SYNTHESIZERCHANNELNODEWIDGET_H_
#define _SYNTHESIZERCHANNELNODEWIDGET_H_

#include <QString>

#include "ChannelNodeWidget.h"

class QLabel;

namespace Ui {
class SynthesizerChannelNodeWidget;
}

class SynthesizerChannelNodeWidget : public ChannelNodeWidget
{
	Q_OBJECT
public:
	explicit SynthesizerChannelNodeWidget(int const channel_index, int const instrument_code,
							   bool const is_displayed_channel_index_start_from_one = false,
							   QWidget *parent = nullptr);
	~SynthesizerChannelNodeWidget() Q_DECL_OVERRIDE;

	void SetIndicator(bool const is_to_highlight) Q_DECL_OVERRIDE;
private slots:
	void on_InstrumentNameComboBox_currentIndexChanged(int const index);
protected:
	int GetDisplayedChannelIndex(void) const Q_DECL_OVERRIDE;
	void SetTitleText(QString const &text) Q_DECL_OVERRIDE;
	void SetTitleStyleSheet(QString const &style_sheet) Q_DECL_OVERRIDE;
	QString GetTitleStyleSheet(void) const Q_DECL_OVERRIDE;
private:
	bool const m_is_displayed_channel_index_start_from_one;
	QString m_channel_indicator_widget_plain_style_sheet;
	QString m_channel_indicator_widget_highlight_style_sheet;
	QLabel *m_p_percussion_name_label;
private:
	Ui::SynthesizerChannelNodeWidget *ui;
};

#endif // _SYNTHESIZERCHANNELNODEWIDGET_H_
