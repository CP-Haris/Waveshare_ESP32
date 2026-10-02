import { registerRootComponent } from 'expo';
import App from './App';
import { Buffer } from 'buffer';

global.Buffer = Buffer;

registerRootComponent(App);
