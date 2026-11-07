#include "../firmware/ignition/core.h"
#include "../firmware/ignition/presets.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <vector>

using namespace ignition;
static void advance(Drive& d,int ticks,float throttle=0,bool brake=false,bool boost=false) {
  for(int i=0;i<ticks;i++)d.step(.01f,throttle,brake,boost);
}
int main(int argc,char** argv) {
  assert(argc==2);
  Drive d;d.configure(kPatches[0]);assert(d.rpm==0);d.start();
  advance(d,100);assert(d.state==Drive::Running);assert(std::fabs(d.rpm-750)<2);
  advance(d,300,1);assert(d.rpm>6000&&d.speed==0);
  advance(d,300);assert(d.shift(1));advance(d,600,.85f);
  const float before=d.rpm;assert(d.speed>20&&before>3000);
  assert(d.shift(1));advance(d,60,.85f);assert(d.rpm<before*.85f);
  const float speed=d.speed;advance(d,100,0,true);assert(d.speed<speed);
  d.stop();assert(d.rpm==0&&d.speed==0&&d.gear==0);
  Drive a,b;for(auto* x:{&a,&b}){x->start();advance(*x,100);x->shift(1);}
  advance(a,200,.65f);advance(b,200,.65f,false,true);assert(b.speed>a.speed*1.1f);
  d.start();advance(d,100);for(int g=1;g<=6;g++){assert(d.shift(1));advance(d,30);}
  advance(d,6000,1,false,true);assert(!d.shift(-1)&&d.overRev);assert(d.rpm<=7000);

  // Held ignition at boot must not start; switch bounce must not count as shifts.
  Controller c;Inputs in;in.ignition=true;
  uint32_t now=0;
  auto tick=[&](int n){for(int i=0;i<n;i++){now+=10;c.update(now,in);}};
  tick(100);assert(c.drive.state==Drive::Off);
  in.ignition=false;tick(5);in.throttle=1;in.ignition=true;tick(10);assert(c.drive.state==Drive::Off);
  in.throttle=0;tick(10);assert(c.drive.state==Drive::Off);
  in.ignition=false;tick(5);in.ignition=true;tick(100);assert(c.drive.state==Drive::Running);
  for(int i=0;i<4;i++){in.up=!in.up;tick(1);}assert(c.drive.gear==0);
  in.up=true;tick(100);assert(c.drive.gear==1);tick(100);assert(c.drive.gear==1);
  in.up=false;tick(5);in.up=true;tick(5);assert(c.drive.gear==2);
  in.ignition=false;tick(5);assert(c.drive.state==Drive::Off);
  assert(normalizeTrigger(0,150,3900)==0&&normalizeTrigger(4095,150,3900)==1);
  assert(normalizeTrigger(2000,150,3900)>.45f&&normalizeTrigger(2000,150,3900)<.55f);

  // Every shipping patch must produce finite bounded sound, and stop to true silence.
  float buffer[128];double totalEnergy=0;float peak=0;
  std::ofstream raw(std::string(argv[1])+"/firmware-v8.pcm",std::ios::binary);
  for(size_t p=0;p<kPresetCount;p++) {
    Synth s;s.setPatch(kPatches[p]);double energy=0;
    for(int block=0;block<500;block++) {
      s.process(buffer,128,kPatches[p].redline*.65f,.85f,.3f);
      for(float sample:buffer){assert(std::isfinite(sample)&&std::fabs(sample)<=.9f);energy+=sample*sample;peak=std::max(peak,std::fabs(sample));
        if(p==0){const int16_t pcm=static_cast<int16_t>(std::lround(sample*32767));raw.write(reinterpret_cast<const char*>(&pcm),sizeof pcm);}}
    }
    assert(energy/(500*128)>.00001);totalEnergy+=energy;
    for(int block=0;block<100;block++)s.process(buffer,128,750,0,0);
    for(float sample:buffer)assert(sample==0);
  }
  Synth seeded1,seeded2;seeded1.setPatch(kPatches[0]);seeded2.setPatch(kPatches[0]);
  float other[128];seeded1.process(buffer,128,3000,.7f,.3f);seeded2.process(other,128,3000,.7f,.3f);
  for(int i=0;i<128;i++)assert(buffer[i]==other[i]);
  // A restart must crank from rest, not resume the previous RPM/filter state.
  Synth hot,fresh;
  for(int i=0;i<500;i++)hot.process(buffer,128,7000,1,.3f);
  for(int i=0;i<100;i++)hot.process(buffer,128,0,0,0);
  for(int block=0;block<50;block++){
    hot.process(buffer,128,350,.12f,.3f);fresh.process(other,128,350,.12f,.3f);
    for(int i=0;i<128;i++)assert(buffer[i]==other[i]);
  }
  for(int i=0;i<100;i++)hot.process(buffer,128,0,0,0);
  hot.setPatch(kPatches[6]);Synth freshIon(kPatches[6]);
  for(int block=0;block<50;block++){
    hot.process(buffer,128,350,.12f,.3f);freshIon.process(other,128,350,.12f,.3f);
    for(int i=0;i<128;i++)assert(buffer[i]==other[i]);
  }
  assert(pcm32(0)==0&&pcm32(-.5f)<0&&pcm32(.5f)>0);
  assert(pcm32(-.5f)==-pcm32(.5f));
  Patch firing=kPatches[0];firing.rpm=3000;firing.roughness=0;firing.firing=Firing::Even;
  Synth pulse(firing);for(int i=0;i<250;i++)pulse.process(buffer,128,3000,.5f,1);
  assert(pulse.firings>=199&&pulse.firings<=201);
  // A host trace is compared against the independently exercised browser model.
  std::ofstream trace(std::string(argv[1])+"/firmware-drive.csv");
  Drive parity;parity.start();
  for(int i=0;i<2000;i++) {
    if(i==100||i==700||i==1200)parity.shift(1);
    const float t=i>=150&&i<1600?.8f:0;
    parity.step(.01f,t,i>=1600,i>=1100&&i<1400);
    if(i%100==99)trace<<i<<","<<parity.rpm<<","<<parity.speed<<","<<parity.load<<","<<parity.gear<<"\n";
  }
  std::printf("{\"passed\":true,\"patches\":%zu,\"peak\":%.5f,\"rms\":%.5f,\"synth_bytes\":%zu}\n",kPresetCount,peak,std::sqrt(totalEnergy/(kPresetCount*500*128)),sizeof(Synth));
}
