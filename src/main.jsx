import ReactDOM from 'react-dom/client'
import App from './App.jsx'
import './index.css'
import { registerLicense } from '@syncfusion/ej2-base';

registerLicense('Ngo9BigBOggjHTQxAR8/V1NAaF1cXmhKYVFzWmFZfVpgdVRMY1hbRHJPMyBoS35RckVhW3tfcXBVRmFcV0J2');

ReactDOM.createRoot(document.getElementById('root')).render(<App />)
