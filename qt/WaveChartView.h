#ifndef _WAVECHARTVIEW_H_
#define _WAVECHARTVIEW_H_

#include <QChartView>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using namespace QtCharts;
#endif

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

	explicit WaveChartView(int number_of_channels, int sampling_rate, int sampling_size,
				  QWidget *parent = nullptr);
	void Reset(void);
public slots:
	void UpdateWave(QByteArray const &wave_bytearray);

private :
	template <typename T> void ReplaceSeries(void);
private :
	QByteArray m_remain_wave_bytearray;
	int m_sample_size_in_bytes;
};

#endif // _WAVECHARTVIEW_H_
