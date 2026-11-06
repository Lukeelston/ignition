import {renderClip,encodeWav} from './engine.js';
self.onmessage=({data})=>{
  try {
    const samples=renderClip(data.patch,data.options);
    const wav=encodeWav(samples,48000);
    self.postMessage({wav},[wav]);
  } catch(error) { self.postMessage({error:error.message}); }
};
