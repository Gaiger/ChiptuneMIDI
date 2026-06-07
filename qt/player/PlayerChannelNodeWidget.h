#ifndef _PLAYERCHANNELNODEWIDGET_H_
#define _PLAYERCHANNELNODEWIDGET_H_

#include <QString>

#include "ChannelNodeWidget.h"

namespace Ui {
class PlayerChannelNodeWidget;
}

class PlayerChannelNodeWidget : public ChannelNodeWidget
{
	Q_OBJECT
public:
	explicit PlayerChannelNodeWidget(int const channel_index, int const instrument_code, QWidget *parent = nullptr);
	~PlayerChannelNodeWidget() Q_DECL_OVERRIDE;

public :
	void SetOutputEnabled(bool const is_to_enabled) Q_DECL_OVERRIDE;
	bool IsOutputEnabled(void) const Q_DECL_OVERRIDE;
private slots:
	void on_OutputEnabledCheckBox_stateChanged(int const state);
protected:
	void SetTitleText(QString const &text) Q_DECL_OVERRIDE;
	void SetTitleStyleSheet(QString const &style_sheet) Q_DECL_OVERRIDE;
	QString GetTitleStyleSheet(void) const Q_DECL_OVERRIDE;
private:
	Ui::PlayerChannelNodeWidget *ui;
};

#endif // _PLAYERCHANNELNODEWIDGET_H_
