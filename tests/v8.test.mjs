import test from 'node:test';
import assert from 'node:assert/strict';
import {EngineSynth,PRESETS} from '../web/engine.js';

const base=PRESETS[0].patch;
function measure(patch,sr){
  const synth=new EngineSynth({...base,...patch,intake:0,whine:0,turbo:0,sub:0,reactor:0,space:0,roughness:0},sr);
  const buffer=new Float32Array(128);let energy=0,upper=0,low=0,peak=0;
  const alpha=1-Math.exp(-2*Math.PI*1000/sr);
  for(let offset=0;offset<sr*2;offset+=128){
    const count=Math.min(128,sr*2-offset),block=buffer.subarray(0,count);synth.process(block);
    for(const x of block){low+=alpha*(x-low);if(offset>=sr){energy+=x*x;upper+=(x-low)**2;peak=Math.max(peak,Math.abs(x));}}
  }
  return {rms:Math.sqrt(energy/sr),brightness:Math.sqrt(upper/energy),peak};
}

test('V8 load opens the exhaust tone as well as raising its level',()=>{
  for(const sr of [16000,48000])for(const rpm of [1800,3600]){
    const coast=measure({rpm,load:.1},sr),pull=measure({rpm,load:.9},sr);
    assert.ok(pull.brightness>coast.brightness*1.25,`${sr} Hz / ${rpm} RPM: brightness ${coast.brightness} -> ${pull.brightness}`);
    assert.ok(pull.rms>coast.rms*1.4,'loaded exhaust must bark louder than coast');
    assert.ok(pull.peak<.9,'leave digital headroom');
  }
});

test('V8 keeps its voice across the road-speed range',()=>{
  for(const sr of [8000,16000,48000,96000]){
    const values=[750,1800,3600,6500].map(rpm=>measure({rpm,load:.8},sr));
    for(const result of values){
      assert.ok(Number.isFinite(result.rms)&&result.rms>.025);
      assert.ok(result.peak<.9);
    }
    assert.ok(values.at(-1).rms>values[1].rms*.5,'high revs must retain exhaust weight');
  }
});

test('loaded V8 keeps its bank rhythm instead of one dominant singing tone',()=>{
  for(const rpm of [3600,6000]){
    const sr=16000,synth=new EngineSynth({...base,rpm,load:.85,roughness:0,intake:0,whine:0,sub:0,space:0},sr);
    for(let i=0;i<125;i++)synth.process(new Float32Array(128));
    const audio=new Float32Array(sr);
    for(let i=0;i<sr;i+=128)synth.process(audio.subarray(i,i+128));
    let energy=0,re=0,im=0;
    for(let i=0;i<sr;i++){
      const x=audio[i],angle=2*Math.PI*(rpm/15)*i/sr;
      energy+=x*x;re+=x*Math.cos(angle);im+=x*Math.sin(angle);
    }
    const tonalFraction=2*(re*re+im*im)/(sr*energy);
    assert.ok(tonalFraction<.55,`${rpm} RPM: one firing harmonic contains ${Math.round(tonalFraction*100)}% of all energy`);
  }
});

test('rapid V8 tone and character edits stay finite through their filter tails',()=>{
  const synth=new EngineSynth(base,16000),block=new Float32Array(128);
  for(let n=0;n<500;n++){
    if(n%13===0)synth.set({...PRESETS[Math.floor(n/13)%PRESETS.length].patch});
    if(n%7===0)synth.set({body:n%2,rasp:n%2,exhaust:n%2?1200:60});
    synth.process(block);
    assert.ok(block.every(x=>Number.isFinite(x)&&Math.abs(x)<=.9));
  }
});
