import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {ToyDrive} from '../web/drive.js';
import {EngineSynth,PRESETS} from '../web/engine.js';
const rows=(await readFile(new URL('../evidence/firmware-drive.csv',import.meta.url),'utf8')).trim().split(/\r?\n/).map(line=>line.split(',').map(Number));
const d=new ToyDrive();d.start();let checked=0,maxRPMError=0;
for(let i=0;i<2000;i++){
  if([100,700,1200].includes(i))d.shift(1);
  d.step(.01,{throttle:i>=150&&i<1600?.8:0,brake:i>=1600,boost:i>=1100&&i<1400});
  if(i%100!==99)continue;
  const [tick,rpm,speed,load,gear]=rows[checked++],s=d.snapshot();
  assert.equal(tick,i);assert.equal(gear,s.gear);
  maxRPMError=Math.max(maxRPMError,Math.abs(rpm-s.rpm));
  assert.ok(Math.abs(rpm-s.rpm)<1,`RPM mismatch at ${i}: ${rpm} vs ${s.rpm}`);
  assert.ok(Math.abs(speed-s.speed)<.02,`Speed mismatch at ${i}`);
  assert.ok(Math.abs(load-s.load)<.002,`Load mismatch at ${i}`);
}
assert.equal(checked,20);
const audioRows=(await readFile(new URL('../evidence/firmware-v8-metrics.csv',import.meta.url),'utf8')).trim().split(/\r?\n/).map(line=>line.split(',').map(Number));
let maxSoundRelativeError=0;
for(const [rpm,load,nativeRms,nativeBrightness] of audioRows){
  const sr=16000,synth=new EngineSynth({...PRESETS[0].patch,rpm,load,intake:0,whine:0,turbo:0,sub:0,reactor:0,space:0,roughness:0},sr);
  const block=new Float32Array(128),alpha=1-Math.exp(-2*Math.PI*1000/sr);
  let low=0,energy=0,upper=0;
  for(let offset=0;offset<sr*2;offset+=128){
    synth.process(block);
    for(const sample of block){const x=sample*.8;low+=alpha*(x-low);if(offset>=sr){energy+=x*x;upper+=(x-low)**2;}}
  }
  for(const [native,web] of [[nativeRms,Math.sqrt(energy/sr)],[nativeBrightness,Math.sqrt(upper/energy)]]){
    const error=Math.abs(native-web)/web;
    maxSoundRelativeError=Math.max(maxSoundRelativeError,error);
    assert.ok(error<.03,`V8 audio mismatch at ${rpm} RPM / ${load} load: ${native} vs ${web}`);
  }
}
assert.equal(audioRows.length,8);
console.log(JSON.stringify({driveParity:true,checkpoints:checked,maxRPMError,v8SoundCheckpoints:audioRows.length,maxSoundRelativeError}));
