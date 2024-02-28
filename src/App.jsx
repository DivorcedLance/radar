import './App.css';
import { useEffect, useState } from 'react';

import Chart from './components/Chart';

import {report, settings} from './mocks/mocks';

const App = () => {

  console.log(report)

  const [bulk, setBulk] = useState(report["report"])
  const [showConfig, setShowConfig] = useState(false);
  const [connected, setconnected] = useState(false)
  const [lightState, setLightState] = useState(false)

  const [detectionThreshold, setDetectionThreshold] = useState(18)
  const [sendReportInterval, setSendReportInterval] = useState(15000);

  const [ws, setWs] = useState(null)

  console.log(report)
  console.log(settings)


  const connectWs = () => {

    setWs(new WebSocket("ws://" + "192.168.1.78" + ":81"))
    ws.onopen = () => {
      console.log('connected')
      setconnected(true)
    }

    ws.onmessage = (e_msg) => {
      e_msg = e_msg || window.event;
      let data = e_msg.data;
      if (Object.keys(data).includes('report')) {
        setBulk(data['report'])
      }
      if (Object.keys(data).includes('settings')) {
        setDetectionThreshold(data['settings']['detectionThreshold'])
        setSendReportInterval(data['settings']['sendReportIntervalMillis'])
      }
    }

    ws.onclose = () => {
      console.log('closed')
      setconnected(false)
    }
  }
  
  useEffect(() => {
    connectWs();
    return () => {
      ws.close();
    }
  }, [ws])

  useEffect(() => {
    ws.send(detectionThreshold + 't');
    ws.send(sendReportInterval + 'i');

  }, [detectionThreshold, sendReportInterval, ws])
  
  useEffect(() => {
    if (lightState) {
      // ws.send('1')
    } else {
      // ws.send('0')
    }
  }, [lightState])


  return (
    <div>
      <button className='light-button' style={lightState ? {backgroundColor: 'yellow', 'color': 'black'} : {backgroundColor: 'grey', 'color': 'black'}}
      onClick={() => setLightState(!lightState)}>Smart Light</button>
      <div style={{ display: 'flex', flexDirection: 'column', backgroundColor: '#fff'}}>
        <Chart data={bulk}  id='charts'/>
        
      </div>

      <button onClick={() => setShowConfig(!showConfig)}>Config</button>

      <div className='config-menu' style={ showConfig ? {display:'flex'} : {display:'none'} }>

        <label style={{ fontSize: '40px' }} htmlFor="detectionThreshold">Umbral de Detección: {detectionThreshold}</label>
        <input style={{ width:'630px' }} type="range" id="detectionThreshold" name="detectionThreshold" min="0" max="50" value={detectionThreshold} onChange={(e) => setDetectionThreshold(e.target.value)} />

        <label style={{ fontSize: '40px' }} htmlFor="sendReportInterval">Intervalo de Envío de Reportes: {sendReportInterval/1000}s</label>
        <input style={{ width:'630px' }} type="range" id="sendReportInterval" name="sendReportInterval" min="0" max="20000" value={sendReportInterval} onChange={(e) => setSendReportInterval(e.target.value)} />

        <button onClick={() => setShowConfig(!showConfig)}>Close</button>
      </div>
    </div>
  );
};

export default App;
