#include "../firmware/ignition/core.h"
#include "../firmware/ignition/presets.h"
#include "../firmware/ignition/test_tone.h"
#include "../firmware/ignition/output_stage.h"
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
  // A loaded V8 changes spectral balance, not only amplitude. Use the same
  // observable high-band/total-energy ratio as the browser acceptance test.
  struct VoiceMeasurement {double rms,brightness;};
  auto measureV8=[](float rpm,float load){
    Patch p=kPatches[0];p.rpm=rpm;p.load=load;
    p.intake=p.whine=p.turbo=p.sub=p.reactor=p.space=p.roughness=0;
    Synth synth(p);float block[128],low=0;double energy=0,upper=0;
    const float alpha=1-std::exp(-tau*1000/sampleRate);
    for(uint32_t offset=0;offset<sampleRate*2;offset+=128){
      const int count=std::min(128u,sampleRate*2-offset);synth.process(block,count,rpm,load,.8f);
      for(int i=0;i<count;i++){
        const float x=block[i];low+=alpha*(x-low);
        if(offset>=sampleRate){energy+=x*x;upper+=(x-low)*(x-low);}
      }
    }
    return VoiceMeasurement{std::sqrt(energy/sampleRate),std::sqrt(upper/energy)};
  };
  for(float rpm:{1800.f,3600.f}){
    const auto coast=measureV8(rpm,.1f),pull=measureV8(rpm,.9f);
    assert(pull.brightness>coast.brightness*1.25&&pull.rms>coast.rms*1.4);
  }
  assert(measureV8(6500,.8f).rms>measureV8(1800,.8f).rms*.5);
  std::ofstream soundTrace(std::string(argv[1])+"/firmware-v8-metrics.csv");
  for(float rpm:{750.f,1800.f,3600.f,6000.f})for(float load:{.1f,.85f}){
    const auto result=measureV8(rpm,load);
    soundTrace<<rpm<<","<<load<<","<<result.rms<<","<<result.brightness<<"\n";
  }
  for(float rpm:{3600.f,6000.f}){
    Patch p=kPatches[0];p.rpm=rpm;p.load=.85f;
    p.roughness=p.intake=p.whine=p.sub=p.space=0;
    Synth synth(p);float block[128];double energy=0,re=0,im=0;
    for(int n=0;n<125;n++)synth.process(block,128,rpm,.85f,.8f);
    for(uint32_t i=0;i<sampleRate;i+=128){
      const auto count=std::min(128u,sampleRate-i);synth.process(block,count,rpm,.85f,.8f);
      for(uint32_t j=0;j<count;j++){
        const double x=block[j],angle=6.283185307179586*(rpm/15)*(i+j)/sampleRate;
        energy+=x*x;re+=x*std::cos(angle);im+=x*std::sin(angle);
      }
    }
    assert(2*(re*re+im*im)/(sampleRate*energy)<.55);
  }
  assert(speakerSample(0,2)==0);
  assert(std::fabs(speakerSample(.2f,2)-.4f)<1e-6f);
  assert(std::fabs(speakerSample(-.2f,2)+.4f)<1e-6f);
  float lastOutput=0;
  for(int i=0;i<=10000;i++){
    const float x=float(i)/1000,y=speakerSample(x,2);
    assert(std::isfinite(y)&&y<.96f&&y>=lastOutput);
    assert(std::fabs(y+speakerSample(-x,2))<1e-6f);lastOutput=y;
  }
  assert(speakerSample(NAN,2)==0&&speakerSample(.1f,INFINITY)==0);
  assert(speakerSample(.5f,0)==0);
  float speakerPeak=0;double v8LevelRatio=0;
  for(size_t index=0;index<kPresetCount;index++){
    Synth synth(kPatches[index]);float block[128];double original=0,louder=0;
    for(int n=0;n<500;n++){
      synth.process(block,128,kPatches[index].redline*.65f,.85f,.8f);
      for(float x:block){const float y=speakerSample(x,2);original+=x*x;louder+=y*y;
        speakerPeak=std::max(speakerPeak,std::fabs(y));assert(std::isfinite(y)&&std::fabs(y)<.96f);}
    }
    assert(louder>original*1.7*1.7);
    if(index==0)v8LevelRatio=std::sqrt(louder/original);
  }
  // A press is undecided until release/hold: long press must never toggle power.
  {
    Controller c;Inputs in;uint32_t now=0;int cycles=0;
    auto tick=[&](int n){for(int i=0;i<n;i++){now+=10;c.update(now,in);if(c.nextCharacter)cycles++;}};
    tick(5);in.ignition=true;tick(5);assert(c.drive.state==Drive::Off);
    in.ignition=false;tick(5);assert(c.drive.state==Drive::Cranking);
    tick(150);in.ignition=true;tick(70);
    assert(c.drive.state==Drive::Running&&cycles==1);
    tick(90);assert(cycles==2);in.ignition=false;tick(5);
    assert(c.drive.state==Drive::Running&&cycles==2);
    in.ignition=true;tick(5);in.ignition=false;tick(5);assert(c.drive.state==Drive::Off);
    in.ignition=true;tick(70);assert(cycles==3&&c.drive.state==Drive::Off);
    in.ignition=false;tick(5);assert(c.drive.state==Drive::Off);
    // A start press rejected under throttle cannot become valid on release.
    in.throttle=1;in.ignition=true;tick(5);in.throttle=0;tick(5);
    in.ignition=false;tick(5);assert(c.drive.state==Drive::Off);
    // Pressing at idle then moving the throttle also rejects the whole tap.
    in.ignition=true;tick(5);in.throttle=1;tick(5);in.throttle=0;
    in.ignition=false;tick(5);assert(c.drive.state==Drive::Off);
    // Short taps still cancel the starter.
    in.ignition=true;tick(5);in.ignition=false;tick(5);assert(c.drive.state==Drive::Cranking);
    in.ignition=true;tick(5);in.ignition=false;tick(5);assert(c.drive.state==Drive::Off);
    for(int i=0;i<8;i++){in.ignition=!in.ignition;tick(1);}tick(5);assert(c.drive.state==Drive::Off);
  }
  {
    Controller c;Inputs in;in.ignition=true;int cycles=0;
    for(uint32_t t=0;t<2000;t+=10){c.update(t,in);cycles+=c.nextCharacter;}
    assert(c.drive.state==Drive::Off&&cycles==0);
    in.ignition=false;for(uint32_t t=2000;t<2100;t+=10)c.update(t,in);
    in.ignition=true;for(uint32_t t=2100;t<2150;t+=10)c.update(t,in);
    in.ignition=false;for(uint32_t t=2150;t<2200;t+=10)c.update(t,in);
    assert(c.drive.state==Drive::Cranking);
  }
  {
    Controller c;Inputs in;uint32_t now=UINT32_MAX-500;int cycles=0;
    for(int i=0;i<210;i++){
      in.ignition=i>=5&&i<200;now+=10;c.update(now,in);cycles+=c.nextCharacter;
    }
    assert(cycles==2&&c.drive.state==Drive::Off); // hold/repeat survives millis wrap
  }
  {
    Drive light,hard;
    for(auto* d:{&light,&hard}){d->start();advance(*d,150);d->shift(1);advance(*d,200,.65f);}
    assert(light.speed>19); // measured old profile: 15.00 km/h at this point
    for(int i=0;i<20;i++){light.step(.01f,0,.2f);hard.step(.01f,0,1);}
    assert(light.speed>hard.speed+3);
    advance(light,500,1);const float before=light.speed;
    light.configure(kPatches[4]);light.step(.01f,1);
    assert(std::fabs(light.speed-before)<1&&light.rpm<=kPatches[4].redline);
    Controller c;Inputs in;c.update(0,in);in.brakeLevel=.25f;c.update(10,in);
    assert(c.braking&&c.brakeAmount==.25f);
    in.brake=true;c.update(20,in);c.update(40,in);assert(c.brakeAmount==1);
  }
  // Idle roughness must not bend firing pitch throughout a steady high-RPM run.
  for(float rpm:{750.f,3000.f,6000.f}){
    Patch pa=kPatches[0];pa.rpm=rpm;pa.roughness=.5f;Patch pb=pa;pb.roughness=0;
    Synth a(pa),b(pb);float x[128],y[128];double energy=0;
    for(int i=0;i<125;i++){
      a.process(x,128,rpm,.5f,.8f);b.process(y,128,rpm,.5f,.8f);
      for(int j=0;j<128;j++)energy+=(x[j]-y[j])*(x[j]-y[j]);
    }
    if(rpm==750)assert(energy/sampleRate>1e-6);else assert(energy/sampleRate<1e-12);
  }
  // Bench gain commands must stay bounded; unrelated USB input leaves the gain
  // alone. Meter the actual packed stereo words, including silence and signs.
  assert(benchGain('0',.22f)==0&&benchGain('8',.22f)==.8f);
  for(char command:{'9','t','\n','-','x'})assert(benchGain(command,.22f)==.22f);
  for(char command='0';command<='8';command++)assert(benchGain(command,.22f)>=0&&benchGain(command,.22f)<=.8f);
  const int32_t meterSamples[]={0,0,1073741824,1073741824,-1073741824,-1073741824,0,0};
  const auto metered=measurePCM(meterSamples,8);
  assert(metered.peak==.5f&&std::fabs(metered.rms-std::sqrt(.125f))<1e-6f);
  const int32_t zeros[]={0,0};assert(measurePCM(zeros,2).rms==0);
  assert(measurePCM(nullptr,0).peak==0&&measurePCM(nullptr,0).rms==0);
  // Diagnostic signal: eight finite, low-level 660 Hz beeps, soft edges and
  // exact silence between notes and after the bounded test completes.
  TestTone tone;assert(!tone.active()&&tone.next()==0);tone.start();
  int crossings=0;float previous=0,tonePeak=0;double toneEnergy=0;
  for(uint32_t i=0;i<TestTone::totalFrames;i++){
    assert(tone.active());const float value=tone.next();
    assert(std::isfinite(value)&&std::fabs(value)<=.023438f);
    assert(std::fabs(value-previous)<.0031f*32000/sampleRate);
    if(i<TestTone::leadFrames)assert(value==0);
    else {
      const uint32_t position=(i-TestTone::leadFrames)%TestTone::periodFrames;
      if(position>=sampleRate)assert(value==0);
      if(value>0&&previous<=0)crossings++;
    }
    previous=value;tonePeak=std::max(tonePeak,std::fabs(value));toneEnergy+=value*value;
  }
  assert(!tone.active()&&tone.next()==0&&tonePeak>.023f&&toneEnergy/TestTone::totalFrames>.00015);
  assert(crossings==8*660);
  tone.start();assert(tone.active()&&tone.next()==0);
  tone.stop();assert(!tone.active()&&tone.next()==0);
  Drive d;d.configure(kPatches[0]);assert(d.rpm==0);d.start();
  advance(d,30);assert(d.state==Drive::Cranking&&d.starter>.9f);assert(!d.shift(1));
  advance(d,50);assert(d.rpm>750*1.3f);
  advance(d,70);assert(d.state==Drive::Running&&d.starter==0);assert(std::fabs(d.rpm-750)<2);
  advance(d,300,1);assert(d.rpm>6000&&d.speed==0);
  advance(d,300);assert(d.shift(1));advance(d,600,.85f);
  const float before=d.rpm;assert(d.speed>20&&before>3000);
  assert(d.shift(1));advance(d,60,.85f);assert(d.rpm<before*.85f);
  Drive loaded;loaded.start();advance(loaded,150);loaded.shift(1);advance(loaded,500,.8f);
  const float loadBefore=loaded.load,roadSpeed=loaded.speed;
  assert(loaded.shift(1)&&loaded.speed==roadSpeed);advance(loaded,5,.8f);assert(loaded.load<loadBefore);
  advance(loaded,40,.8f);assert(loaded.load>loadBefore+.03f);
  const float speed=d.speed;advance(d,100,0,true);assert(d.speed<speed);
  d.stop();assert(d.rpm==0&&d.speed==0&&d.gear==0);
  Drive a,b;for(auto* x:{&a,&b}){x->start();advance(*x,150);x->shift(1);}
  advance(a,200,.65f);advance(b,200,.65f,false,true);assert(b.speed>a.speed*1.1f);
  d.start();advance(d,150);for(int g=1;g<=6;g++){assert(d.shift(1));advance(d,30);}
  advance(d,6000,1,false,true);assert(!d.shift(-1)&&d.overRev);assert(d.rpm<=7000);

  // Maintained-switch compatibility: held ignition at boot must not start.
  Controller c(false);Inputs in;in.ignition=true;
  uint32_t now=0;
  auto tick=[&](int n){for(int i=0;i<n;i++){now+=10;c.update(now,in);}};
  tick(100);assert(c.drive.state==Drive::Off);
  in.ignition=false;tick(5);in.throttle=1;in.ignition=true;tick(10);assert(c.drive.state==Drive::Off);
  in.throttle=0;tick(10);assert(c.drive.state==Drive::Off);
  in.ignition=false;tick(5);in.ignition=true;tick(150);assert(c.drive.state==Drive::Running);
  for(int i=0;i<4;i++){in.up=!in.up;tick(1);}assert(c.drive.gear==0);
  in.up=true;tick(100);assert(c.drive.gear==1);tick(100);assert(c.drive.gear==1);
  in.up=false;tick(5);in.up=true;tick(5);assert(c.drive.gear==2);
  in.ignition=false;tick(5);assert(c.drive.state==Drive::Off);
  // A centred thumbstick must idle, including rest jitter and opposite travel.
  for(int raw:{0,150,1748,2048,2200,2348})assert(normalizeThrottle(raw,2048,3900,300)==0);
  assert(normalizeThrottle(3124,2048,3900,300)==.5f);
  assert(normalizeThrottle(3900,2048,3900,300)==1&&normalizeThrottle(4095,2048,3900,300)==1);
  // Reversing the endpoint preserves the centre dead zone, rather than inverting idle.
  for(int raw:{1748,2048,2348,3900,4095})assert(normalizeThrottle(raw,2048,150,300)==0);
  assert(normalizeThrottle(949,2048,150,300)==.5f);
  assert(normalizeThrottle(150,2048,150,300)==1&&normalizeThrottle(0,2048,150,300)==1);
  assert(normalizeThrottle(4095,2048,2048,300)==0);
  assert(normalizeThrottle(4095,2048,2200,300)==0);
  assert(normalizeThrottle(4095,2048,3900,-1)==0);
  // A full-travel trigger/pot can still use calibrated endpoints and a smaller dead zone.
  assert(normalizeThrottle(150,150,3900,100)==0&&normalizeThrottle(3900,150,3900,100)==1);
  Controller stick(false);Inputs stickIn;uint32_t stickNow=0;
  auto stickTick=[&](int raw,int n){for(int i=0;i<n;i++){stickNow+=10;stickIn.throttle=normalizeThrottle(raw,2048,3900,300);stick.update(stickNow,stickIn);}};
  stickTick(2200,5);stickIn.ignition=true;stickTick(2200,150);
  assert(stick.drive.state==Drive::Running&&std::fabs(stick.drive.rpm-750)<2);
  stickTick(3900,300);assert(stick.drive.rpm>6000);
  stickTick(2048,300);assert(std::fabs(stick.drive.rpm-750)<2);
  stickIn.ignition=false;stickTick(3900,5);stickIn.ignition=true;stickTick(3900,100);
  assert(stick.drive.state==Drive::Off);

  // Isolate random texture by comparing two seeds through the real synthesizer.
  // It should swell during acceleration and decay even at held high throttle.
  {
    Patch pa=kPatches[0];pa.rpm=750;pa.load=.12f;pa.seed=7;
    Patch pb=pa;pb.seed=99;Synth sa(pa),sb(pb);
    auto difference=[&](int frames,float rpm,float load){
      double energy=0;int count=0;float x[128],y[128];
      while(frames>0){const int n=std::min(128,frames);sa.process(x,n,rpm,load,.8f);sb.process(y,n,rpm,load,.8f);
        for(int i=0;i<n;i++){const double diff=x[i]-y[i];energy+=diff*diff;++count;}frames-=n;}
      return std::sqrt(energy/count);
    };
    assert(difference(sampleRate,750,.12f)<1e-6);
    const double swell=difference(sampleRate,6000,.8f);assert(swell>.0001);
    difference(4*sampleRate,6000,.8f);
    assert(difference(sampleRate,6000,.8f)<swell*.05);
    difference(3*sampleRate,750,.12f);
    assert(difference(sampleRate,750,.12f)<swell*.05);
  }
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
  // Match the reference driver: signed 16-bit PCM in the high half of each
  // 32-bit slot. Quantization must preserve level/sign within half a PCM16 step.
  for(int i=-40000;i<=40000;i++){
    const float input=float(i)/40000;
    const int32_t packed=pcm32(input);
    assert((uint32_t(packed)&0xffffu)==0);
    const double decoded=double(packed)/2147483648.0;
    assert(std::fabs(decoded-bounded(input,-.99f,.99f))<=1.0/65536+1e-7);
  }
  assert(pcm32(768.f/32768)==768*65536);
  assert(pcm32(-768.f/32768)==-768*65536);
  Patch firing=kPatches[0];firing.rpm=3000;firing.roughness=0;firing.firing=Firing::Even;
  Synth pulse(firing);for(uint32_t i=0;i<sampleRate;i+=128)pulse.process(buffer,std::min(128u,sampleRate-i),3000,.5f,1);
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
  std::printf("{\"passed\":true,\"sample_rate\":%u,\"patches\":%zu,\"peak\":%.5f,\"rms\":%.5f,\"speaker_peak\":%.5f,\"speaker_v8_rms_ratio\":%.5f,\"synth_bytes\":%zu}\n",sampleRate,kPresetCount,peak,std::sqrt(totalEnergy/(kPresetCount*500*128)),speakerPeak,v8LevelRatio,sizeof(Synth));
}
