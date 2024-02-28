import { ChartComponent, SeriesCollectionDirective, SeriesDirective, Inject, ColumnSeries, Legend, DateTime, Tooltip, DataLabel, LineSeries } from '@syncfusion/ej2-react-charts';

function Chart({data}) {
    // const primaryxAxis = { valueType: 'DateTime', title: 'Time', labelFormat: 'HH:mm:ss' };
    const primaryxAxis = { title: 'Time' };
    const primaryyAxis = { title: 'Input' };

    return (<ChartComponent id='charts' primaryXAxis={primaryxAxis} primaryYAxis={primaryyAxis} title='Input Registrado' width='1200px' height='800px'>
      <Inject services={[ColumnSeries, Legend, Tooltip, DataLabel, LineSeries, DateTime]}/>
      <SeriesCollectionDirective>
        <SeriesDirective dataSource={data} xName='millis' yName={'inputADC'} name={'inputADC'} type='Line'/>
        <SeriesDirective dataSource={data} xName='millis' yName={'filteredInputADC'} name={'filteredInputADC'} type='Line'/>
      </SeriesCollectionDirective>
    </ChartComponent>)
}
export default Chart;