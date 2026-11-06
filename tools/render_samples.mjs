import {mkdirSync,writeFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import {PRESETS,renderClip,encodeWav} from '../web/engine.js';
const dir=new URL('../samples/',import.meta.url);mkdirSync(dir,{recursive:true});
for(const id of ['v8','ion']) {
  const preset=PRESETS.find(p=>p.id===id);
  const samples=renderClip(preset.patch,{duration:8,motion:'rev'});
  const dest=new URL(`${id}-rev.wav`,dir);writeFileSync(dest,Buffer.from(encodeWav(samples)));
  const peak=samples.reduce((m,x)=>Math.max(m,Math.abs(x)),0);
  const rms=Math.sqrt(samples.reduce((s,x)=>s+x*x,0)/samples.length);
  console.log(JSON.stringify({file:fileURLToPath(dest),seconds:samples.length/48000,peak,rms}));
}
