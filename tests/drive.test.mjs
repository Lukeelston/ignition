import test from 'node:test';
import assert from 'node:assert/strict';
import {ToyDrive} from '../web/drive.js';

function advance(drive, seconds, input={}) {
  for(let i=0;i<Math.round(seconds*100);i++) drive.step(.01,input);
  return drive.snapshot();
}
test('ignition cranks, settles to idle, and resets all motion on stop',()=>{
  const d=new ToyDrive();
  assert.equal(d.snapshot().rpm,0);
  d.start();assert.equal(d.snapshot().state,'cranking');
  const cranking=advance(d,.3);assert.ok(cranking.rpm>0&&cranking.starter>.9);
  assert.equal(d.shift(1),false);
  const caught=advance(d,.5);assert.ok(caught.rpm>750*1.3);
  const idle=advance(d,3);
  assert.equal(idle.state,'running');assert.ok(Math.abs(idle.rpm-750)<2);
  d.shift(1);advance(d,5,{throttle:1});d.stop();
  assert.deepEqual([d.snapshot().rpm,d.snapshot().speed,d.snapshot().gear],[0,0,0]);
});
test('neutral revs without moving; first gear accelerates; an upshift lowers RPM',()=>{
  const d=new ToyDrive();d.start();advance(d,1.5);
  let s=advance(d,3,{throttle:1});
  assert.equal(s.speed,0);assert.ok(s.rpm>6000);
  advance(d,3);assert.equal(d.shift(1),true);
  s=advance(d,6,{throttle:.85});
  assert.ok(s.speed>20);assert.ok(s.rpm>3000);
  const before=s.rpm;
  assert.equal(d.shift(1),true);s=advance(d,.6,{throttle:.85});
  assert.equal(s.gear,2);assert.ok(s.rpm<before*.85,`${before} -> ${s.rpm}`);
});
test('brake slows the toy and boost makes it accelerate harder',()=>{
  const a=new ToyDrive(),b=new ToyDrive();
  for(const d of [a,b]){d.start();advance(d,1.5);d.shift(1);}
  const normal=advance(a,2,{throttle:.65});
  const boost=advance(b,2,{throttle:.65,boost:true});
  assert.ok(boost.speed>normal.speed*1.1);
  const braking=advance(b,1,{brake:true});
  assert.ok(braking.speed<boost.speed);assert.ok(braking.speed>=0);
});
test('brake travel is proportional and the uprated engine pulls harder',()=>{
  const a=new ToyDrive(),b=new ToyDrive();
  for(const d of [a,b]){d.start();advance(d,1.5);d.shift(1);advance(d,2,{throttle:.65});}
  assert.ok(a.speed>19,'uprated launch should exceed the measured former 15.00 km/h benchmark');
  const light=advance(a,.2,{brake:.2}),hard=advance(b,.2,{brake:1});
  assert.ok(light.speed>hard.speed+3,'small brake travel must not apply full braking');
});
test('changing to a low-redline engine while moving does not teleport road speed',()=>{
  const d=new ToyDrive();d.start();advance(d,1.5);d.shift(1);advance(d,6,{throttle:1});
  const before=d.speed;d.configure({idle:500,redline:3500,response:.2});
  const s=d.step(.01,{throttle:1});
  assert.ok(Math.abs(s.speed-before)<1,'engine change must preserve vehicle momentum');
  assert.ok(s.rpm<=3500);
});
test('an upshift increases load after torque interruption while preserving road speed',()=>{
  const d=new ToyDrive();d.start();advance(d,1.5);d.shift(1);
  const before=advance(d,5,{throttle:.8});
  assert.equal(d.shift(1),true);
  assert.equal(d.snapshot().speed,before.speed);
  assert.ok(advance(d,.05,{throttle:.8}).load<before.load);
  const after=advance(d,.4,{throttle:.8});
  assert.ok(after.rpm<before.rpm*.85);
  assert.ok(after.load>before.load+.03,`${before.load} -> ${after.load}`);
});
test('gear limits, shift interruption and over-rev protection',()=>{
  const d=new ToyDrive();assert.equal(d.shift(1),false);
  d.start();advance(d,1.5);assert.equal(d.shift(-1),false);
  for(let gear=1;gear<=6;gear++){assert.equal(d.shift(1),true);advance(d,.3);}
  assert.equal(d.shift(1),false);
  advance(d,60,{throttle:1,boost:true});
  assert.equal(d.shift(-1),false);assert.match(d.snapshot().notice,/Over-rev/);
  assert.ok(d.snapshot().rpm<=7000);
});
test('controls remain bounded and invalid timing is rejected',()=>{
  const d=new ToyDrive();d.start();advance(d,1.5);d.shift(1);
  const s=advance(d,60,{throttle:9});
  assert.ok(s.rpm>=750&&s.rpm<=7000);assert.ok(s.load>=0&&s.load<=1);
  assert.throws(()=>d.step(NaN),/time/);assert.throws(()=>d.step(-1),/time/);
  assert.throws(()=>d.step(.01,{throttle:NaN}),/throttle/);
});
test('tuning limits update without losing gear or motion',()=>{
  const d=new ToyDrive();d.start();advance(d,1.5);d.shift(1);advance(d,5,{throttle:1});
  const speed=d.snapshot().speed;d.configure({idle:600,redline:4000,response:.2});
  assert.equal(d.snapshot().gear,1);assert.equal(d.snapshot().speed,speed);
  assert.ok(advance(d,2,{throttle:1}).rpm<=4000);
});
