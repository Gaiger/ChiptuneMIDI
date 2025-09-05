#include <QtCharts>
#include <QDebug>
#include "WaveChartView.h"

/**********************************************************************************/

WaveChartView::WaveChartView(int const number_of_channels,
							 int const sampling_rate, int const sampling_size, QWidget *parent)
	: QChartView(new QChart(), parent),
	  m_sample_size_in_bytes(sampling_size/8)
{
	do
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		if( false != QMetaType::fromName("WaveChartView::SamplingSize").isValid()){
			break;
		}
#else
		if( QMetaType::UnknownType != QMetaType::type("WaveChartView::SamplingSize")){
			break;
		}
#endif
		qRegisterMetaType<WaveChartView::SamplingSize>("WaveChartView::SamplingSize");
	} while(0);

	QChart * p_chart = QChartView::chart();
	p_chart->setTheme(QChart::ChartThemeDark);
	p_chart->legend()->hide();

	p_chart->layout()->setContentsMargins(0, 0, 0, 0);
	p_chart->setMargins(QMargins(0,0,0,0));
	p_chart->setBackgroundRoundness(0);

	p_chart->addAxis(new QValueAxis(), Qt::AlignBottom);
#define NOTE_A0_FREQUENCY							(27.50)
	int series_length = (int)(sampling_rate/NOTE_A0_FREQUENCY + 0.5);
	//qDebug() << Q_FUNC_INFO << " series_length = " << series_length;
	p_chart->axes(Qt::Horizontal).at(0)->setRange(0, series_length - 1);
	p_chart->axes(Qt::Horizontal).at(0)->setVisible(false);
	p_chart->addAxis( new QValueAxis(), Qt::AlignLeft);
	do {
		if(1 == m_sample_size_in_bytes){
			p_chart->axes(Qt::Vertical).at(0)->setRange(-10, UINT8_MAX + 10);
			break;
		}
		p_chart->axes(Qt::Vertical).at(0)->setRange(INT16_MIN - 256 * 10, INT16_MAX + 10 * 256 );
	} while(0);

	p_chart->axes(Qt::Vertical).at(0)->setVisible(false);

	for(int i = 0; i < number_of_channels; i++){
		QLineSeries *p_series = new QLineSeries();
		p_chart->addSeries(p_series);
		p_series->attachAxis(p_chart->axes(Qt::Horizontal).at(0));
		p_series->attachAxis(p_chart->axes(Qt::Vertical).at(0));
		//p_series->setUseOpenGL(true);
		QPen pen = p_series->pen();
		pen.setWidth(3);
		p_series->setPen(pen);
	}
	QChartView::setRenderHint(QPainter::Antialiasing);
	WaveChartView::Reset();
}

/**********************************************************************************/

template <typename T> void  WaveChartView::ReplaceSeries(void)
{
	int number_of_channels = QChartView::chart()->series().size();
	int length = ((QXYSeries*)QChartView::chart()->series().at(0))->points().length();

	for(int k = 0; k < number_of_channels; k++){
		QList<QPointF> points_vector;
		for(int i = 0; i < length ; i++){
			T * p_value = (T*)(m_remain_wave_bytearray.data() +
										  (number_of_channels * sizeof(T)) * i
										  + sizeof(T) * k);
			points_vector.append( QPointF((double)(i), (double)*p_value));
		}
		QXYSeries *p_series = (QXYSeries*)QChartView::chart()->series().at(k);
		p_series->replace(points_vector);
	}
}

/**********************************************************************************/

void WaveChartView::UpdateWave(QByteArray const &wave_bytearray)
{
	m_remain_wave_bytearray += wave_bytearray;

	do {
		int number_of_channels = QChartView::chart()->series().size();
		int length = ((QXYSeries*)QChartView::chart()->series().at(0))->points().length();
		if(number_of_channels * m_sample_size_in_bytes * length> m_remain_wave_bytearray.size()){
			break;
		}

		while( 2 * number_of_channels * m_sample_size_in_bytes * length < m_remain_wave_bytearray.size() )
		{
			m_remain_wave_bytearray.remove(0, number_of_channels * m_sample_size_in_bytes * length);
		}

		do {
			if(1 == m_sample_size_in_bytes){
				ReplaceSeries<uint8_t>();
				break;
			}
			ReplaceSeries<int16_t>();
		} while(0);
		m_remain_wave_bytearray.remove(0, number_of_channels * m_sample_size_in_bytes  * length);
	} while(0);

}

/**********************************************************************************/

void WaveChartView::Reset(void)
{
	m_remain_wave_bytearray.clear();
	QList<QPointF> points_vector;
	QValueAxis *p_x_axis  = (QValueAxis*)QChartView::chart()->axes(Qt::Horizontal).at(0);
	int length = p_x_axis->max() + 1;

	double value = (UINT8_MAX + 1)/2;
	if(4 * UINT8_MAX < ((QValueAxis*)QChartView::chart()->axes(Qt::Vertical).at(0))->max()){
		value = 0;
	}

	for(int i = 0; i < length ; i++){
		points_vector.append( QPointF((double)i, value) );
	}

	int number_of_channels = QChartView::chart()->series().size();
	for(int k = 0; k < number_of_channels; k++){
		QXYSeries *p_series = (QXYSeries*)QChartView::chart()->series().at(k);
		p_series->replace(points_vector);
	}
}
