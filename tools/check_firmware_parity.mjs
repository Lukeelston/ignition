import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {ToyDrive} from '../web/drive.js';
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
console.log(JSON.stringify({driveParity:true,checkpoints:checked,maxRPMError}));
