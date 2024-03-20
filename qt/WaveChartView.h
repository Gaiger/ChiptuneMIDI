#ifndef WAVECHARTVIEW_H
#define WAVECHARTVIEW_H

#include <QChartView>
using namespace QtCharts;

class WaveChartView : public QChartView
{
	Q_OBJECT
public:
	enum SamplingSize
	{
		SamplingSize8Bit			= 8,
		SamplingSize16Bit			= 16,

		SamplingSizeMax				= 255,
	}; Q_ENUM(SamplingSize)

	WaveChartView(int const number_of_channels, int const sampling_rate, int const sampling_size,
				  QWidget *parent = nullptr);

	void GiveWave(QByteArray wave_bytearray);
	void Reset(void);
private :
	void CleanUndrawnWave(void);
	template <typename T> void ReplaceSeries(void);

private :
	QByteArray m_remain_wave_bytearray;
	int m_sampling_size_in_bytes;
};

#endif // WAVECHARTVIEW_H
