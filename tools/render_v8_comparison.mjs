// Repeatable listening fixture: identical RPM/load inputs for both revisions.
// Usage: node tools/render_v8_comparison.mjs before|after [engine-module-path]
import {writeFileSync,mkdirSync} from 'node:fs';
import {pathToFileURL} from 'node:url';
import {resolve} from 'node:path';
const tag=process.argv[2]||'after';
if(!['before','after'].includes(tag))throw new Error('Use before or after.');
const {EngineSynth,PRESETS,encodeWav}=await import(pathToFileURL(resolve(process.argv[3]||'web/engine.js')));
const patch=PRESETS[0].patch;
mkdirSync('samples',{recursive:true});
function audition(t) {
  if(t<2)return {rpm:750,load:.12};
  if(t<4){const gas=Math.sin(Math.PI*(t-2)/2)**2;return {rpm:750+3500*gas,load:.12+.78*gas};}
  if(t<5)return {rpm:750,load:.12};
  if(t<9)return {rpm:750+(t-5)/4*5000,load:.85};
  if(t<9.22)return {rpm:3900,load:.06};
  if(t<11)return {rpm:3900+(t-9.22)/1.78*2000,load:.9};
  if(t<12.5)return {rpm:5900,load:.9};
  return {rpm:Math.max(750,5900-(t-12.5)*2600),load:.08};
}
const reports=[];
for(const sr of [16000,48000]){
  const synth=new EngineSynth({...patch,...audition(0)},sr),samples=new Float32Array(sr*15);
  for(let i=0;i<sr;i+=128)synth.process(new Float32Array(Math.min(128,sr-i)));
  for(let i=0;i<samples.length;i+=128){synth.set(audition(i/sr));synth.process(samples.subarray(i,Math.min(i+128,samples.length)));}
  const fade=Math.round(sr*.02);
  for(let i=0;i<fade;i++){const g=.5-.5*Math.cos(Math.PI*i/fade);samples[i]*=g;samples[samples.length-1-i]*=g;}
  const rms=Math.sqrt(samples.reduce((s,x)=>s+x*x,0)/samples.length),peak=samples.reduce((m,x)=>Math.max(m,Math.abs(x)),0);
  // Both auditions target -18 dBFS RMS, with a peak guard. Report any guard use.
  const gain=Math.min(10**(-18/20)/rms,.92/peak),matched=samples.map(x=>x*gain);
  const file=`samples/v8-${tag}-${sr}.wav`;
  writeFileSync(file,Buffer.from(encodeWav(matched,sr)));
  if(tag==='after'&&sr===16000)writeFileSync('samples/v8-demo.wav',Buffer.from(encodeWav(matched,sr)));
  reports.push({file,sampleRate:sr,rawRms:rms,rawPeak:peak,comparisonGain:gain,matchedRms:rms*gain,matchedPeak:peak*gain});
}
writeFileSync(`evidence/v8-${tag}.json`,JSON.stringify(reports,null,2));
console.log(JSON.stringify(reports,null,2));
