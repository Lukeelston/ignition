#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace ignition {
constexpr float tau=6.28318530718f;
constexpr uint32_t sampleRate=16000; // bench comparison with Iris's audio clock
inline float bounded(float x,float lo,float hi){return std::max(lo,std::min(hi,x));}
enum class Mode {Combustion,Turbine,Ion};
enum class Firing {Even,Cross,Uneven};
struct Patch {
  Mode mode=Mode::Combustion;Firing firing=Firing::Cross;
  int cylinders=8,stroke=4;uint32_t seed=7;
  float rpm=950,idle=750,redline=7000,load=.28f;
  float body=.72f,exhaust=160,rasp=.35f,roughness=.3f,intake=.2f,whine=.08f,turbo=0,response=.55f,sub=.18f,reactor=0,modulation=4,space=.1f;
};

// Same expressive driving model as web/drive.js; no device dependencies.
class Drive {
  const float ratios[7]={0,3.2f,2.1f,1.55f,1.23f,1.02f,.85f};
  float idle=750,redline=7000,response=.55f,crank=0,elapsed=0;
public:
  enum State {Off,Cranking,Running};
  State state=Off;int gear=0;float speed=0,rpm=0,load=0,shiftTime=0;
  bool limiter=false,overRev=false;
  void configure(const Patch& p){idle=p.idle;redline=p.redline;response=p.response;rpm=std::min(rpm,redline);}
  void stop(){state=Off;gear=0;speed=rpm=load=crank=elapsed=shiftTime=0;limiter=overRev=false;}
  void start(){stop();state=Cranking;rpm=350;}
  bool shift(int direction){
    if(state!=Running||shiftTime>0||(direction!=1&&direction!=-1))return false;
    const int next=gear+direction;if(next<0||next>6)return false;
    if(direction<0&&next>0&&speed*ratios[next]*48>redline*.98f){overRev=true;return false;}
    gear=next;shiftTime=.18f;overRev=false;return true;
  }
  void step(float seconds,float throttle,bool brake=false,bool boost=false){
    if(!std::isfinite(seconds)||!std::isfinite(throttle)||seconds<=0)return;
    float remaining=std::min(seconds,.25f);throttle=bounded(throttle,0,1);
    while(remaining>1e-7f){const float dt=std::min(.01f,remaining);tick(dt,throttle,brake,boost);remaining-=dt;}
  }
private:
  void tick(float dt,float throttle,bool brake,bool boost){
    if(state==Off)return;
    elapsed+=dt;
    if(state==Cranking){crank+=dt;rpm=350+70*std::sin(crank*35);load=.12f;if(crank>=.65f){state=Running;rpm=idle;}return;}
    shiftTime=std::max(0.f,shiftTime-dt);
    const float ratio=ratios[gear],driveThrottle=(shiftTime>0||brake)?0:throttle;
    if(ratio){
      const float torque=std::max(.15f,1-.5f*speed*ratio*48/redline);
      const float acceleration=driveThrottle*(boost?1.5f:1.f)*16*(ratio/3.2f)*torque;
      const float drag=.35f+speed*.012f+(1-driveThrottle)*2.5f;
      speed=bounded(speed+(acceleration-drag-(brake?30:0))*dt,0,redline/(ratio*48));
    }else speed=std::max(0.f,speed-(.5f+speed*.012f+(brake?30:0))*dt);
    const float target=ratio?std::max(idle,speed*ratio*48):idle+(redline-idle)*std::pow(driveThrottle,.7f);
    rpm+=(target-rpm)*(1-std::exp(-dt/(shiftTime>0?.07f:.10f+(1-response)*.20f)));
    rpm=bounded(rpm,idle,redline);limiter=rpm>redline*.975f&&throttle>.8f;
    const bool cut=limiter&&std::fmod(elapsed,.1f)<.035f;
    load=(shiftTime>0||cut)?.05f:bounded(.12f+driveThrottle*(ratio?.88f:.65f),0,1);
  }
};

class Debounced {
  bool initialized=false,candidate=false;uint32_t since=0;
public:
  bool value=false,rise=false,fall=false;
  void update(bool raw,uint32_t now){
    rise=fall=false;
    if(!initialized){initialized=true;candidate=value=raw;since=now;return;}
    if(raw!=candidate){candidate=raw;since=now;}
    if(candidate!=value&&uint32_t(now-since)>=20){value=candidate;rise=value;fall=!value;}
  }
};
struct Inputs {float throttle=0;bool up=false,down=false,brake=false,ignition=false,boost=false,character=false;};
inline float normalizeThrottle(int raw,int rest,int full,int deadZone){
  if(deadZone<0)return 0;
  const float travel=float(full)-rest,span=std::fabs(travel);
  if(span<=deadZone)return 0;
  const float movement=(float(raw)-rest)*(travel>0?1.f:-1.f);
  return bounded((movement-deadZone)/(span-deadZone),0,1);
}
class Controller {
  Debounced up,down,brake,key,boost,character;
  const bool momentary;
  bool initialized=false,armed=false;uint32_t last=0;
public:
  explicit Controller(bool ignitionIsButton=true):momentary(ignitionIsButton){}
  Drive drive;bool nextCharacter=false,boostOn=false,braking=false;
  void update(uint32_t now,const Inputs& in){
    float dt=initialized?std::min(.05f,float(uint32_t(now-last))*.001f):.01f;
    initialized=true;last=now;
    up.update(in.up,now);down.update(in.down,now);brake.update(in.brake,now);key.update(in.ignition,now);boost.update(in.boost,now);character.update(in.character,now);
    const float throttle=std::isfinite(in.throttle)?bounded(in.throttle,0,1):0;
    if(momentary){
      if(!key.value)armed=true;
      else if(key.rise&&armed){
        armed=false;
        if(drive.state!=Drive::Off)drive.stop();
        else if(throttle<.04f)drive.start();
      }
    }else{
      if(!key.value){drive.stop();armed=throttle<.04f;}
      else if(key.rise){if(armed&&throttle<.04f)drive.start();armed=false;}
    }
    if(up.rise&&!down.value)drive.shift(1);
    if(down.rise&&!up.value)drive.shift(-1);
    nextCharacter=character.rise;boostOn=boost.value;braking=brake.value;
    drive.step(dt,throttle,braking,boostOn);
  }
};

// Native port of the browser synthesizer. Preallocated delay and lookup tables;
// no allocations or per-sample sin/tanh in the audio path.
class Synth {
  static constexpr int tableSize=1024,delaySize=864;
  float sineTable[tableSize+1],clipTable[tableSize+1],delay[delaySize]={};
  Patch p,target;uint32_t rng=7;int delayIndex=0;
  float rpm=950,load=.28f,spool=0,phase=0,shaft=0,carrier=0,fieldPhase=0,turboPhase=0;
  float pulse=0,edge=0,low=0,air=0,bodyLow=0,prev=0,dc=0,gain=0;
  bool audible=false;
  void restart(float startRPM,float startLoad){
    p=target;rng=target.seed?target.seed:0x6d2b79f5;rpm=startRPM;load=startLoad;
    spool=phase=shaft=carrier=fieldPhase=turboPhase=0;
    pulse=edge=low=air=bodyLow=prev=dc=gain=0;firings=0;delayIndex=0;
    std::fill(delay,delay+delaySize,0.f);
  }
  float noise(){uint32_t x=rng;x^=x<<13;x^=x>>17;x^=x<<5;rng=x;return float(double(rng)/2147483648.0-1);}
  float sinCycle(float cycle) const {
    cycle-=std::floor(cycle);const float x=cycle*tableSize;const int i=int(x);
    return sineTable[i]+(sineTable[i+1]-sineTable[i])*(x-i);
  }
  float clip(float value) const {
    const float x=(bounded(value,-8,8)+8)*(tableSize/16.f);const int i=std::min(tableSize-1,int(x));
    return clipTable[i]+(clipTable[i+1]-clipTable[i])*(x-i);
  }
  static float wrap(float x){return x>=1?x-1:x;}
public:
  uint32_t firings=0;
  explicit Synth(const Patch& patch=Patch()):p(patch),target(patch),rng(patch.seed?patch.seed:0x6d2b79f5),rpm(patch.rpm),load(patch.load){
    for(int i=0;i<=tableSize;i++){sineTable[i]=std::sin(tau*i/tableSize);clipTable[i]=std::tanh(-8+16.f*i/tableSize);}
  }
  void setPatch(const Patch& patch){if(patch.seed!=target.seed)rng=patch.seed?patch.seed:0x6d2b79f5;target=patch;}
  float outputGain() const {return gain;}
  void process(float* output,size_t count,float requestedRPM,float requestedLoad,float requestedGain){
    const float wantedGain=bounded(requestedGain,0,.8f);
    const float wantedRPM=bounded(requestedRPM,280,target.redline),wantedLoad=bounded(requestedLoad,0,1);
    if(wantedGain>0&&!audible)restart(wantedRPM,wantedLoad);
    audible=wantedGain>0;
    if(wantedGain==0&&gain<.00001f){gain=0;std::fill(output,output+count,0.f);return;}
    const float ease=1-std::exp(-float(count)/(sampleRate*.04f));
    float Patch::* parameters[]={&Patch::body,&Patch::exhaust,&Patch::rasp,&Patch::roughness,&Patch::intake,&Patch::whine,&Patch::turbo,&Patch::response,&Patch::sub,&Patch::reactor,&Patch::modulation,&Patch::space,&Patch::redline};
    for(auto member:parameters)p.*member+=(target.*member-p.*member)*ease;
    p.mode=target.mode;p.firing=target.firing;p.cylinders=target.cylinders;p.stroke=target.stroke;
    const float rpmEase=1-std::exp(-1/(sampleRate*(.045f+(1-p.response)*.52f)));
    const float loadEase=1-std::exp(-1/(sampleRate*.04f));
    const float pulseDecay=std::exp(-1/(sampleRate*(.002f+p.body*.005f))),edgeDecay=std::exp(-1/(sampleRate*.00045f));
    const float lowA=1-std::exp(-tau*(p.exhaust*2.5f+p.rasp*2400)/sampleRate),bodyA=1-std::exp(-tau*p.exhaust/sampleRate);
    const float airA=1-std::exp(-tau*1500/sampleRate),dcA=std::exp(-tau*18/sampleRate);
    const float crossGains[8]={1,.52f,.75f,.52f,.85f,1,.52f,.75f};
    for(size_t i=0;i<count;i++){
      rpm+=(wantedRPM-rpm)*rpmEase;load+=(wantedLoad-load)*loadEase;gain+=(wantedGain-gain)*.002f;
      const float n=noise();shaft=wrap(shaft+rpm/(60*sampleRate));fieldPhase=wrap(fieldPhase+p.modulation/sampleRate);
      phase+=rpm*p.cylinders/((p.stroke==4?120:60)*sampleRate)*(1+p.roughness*.025f*sinCycle(fieldPhase));
      const float spacing=p.firing==Firing::Uneven?(firings%2?1.38f:.62f):1;
      if(phase>=spacing){phase-=spacing;const float weight=(p.firing==Firing::Cross?crossGains[firings%8]:1)*(1+p.roughness*n*.32f);pulse+=weight;edge+=weight;firings++;}
      pulse*=pulseDecay;edge*=edgeDecay;const float combustion=pulse-edge;
      low+=lowA*(combustion-low);bodyLow+=bodyA*(low-bodyLow);air+=airA*(n-air);
      spool+=((rpm/p.redline)*(.25f+.75f*load)-spool)/(sampleRate*.38f);
      turboPhase=wrap(turboPhase+(650+spool*4500)/sampleRate);
      carrier=wrap(carrier+std::min(sampleRate*.18f,80+rpm/60*2.7f)/sampleRate);
      const float mech=sinCycle(shaft*6)*.65f+sinCycle(shaft*12)*.2f;
      const float field=sinCycle(carrier)*(.65f+.35f*sinCycle(fieldPhase))+.3f*sinCycle(carrier*2+fieldPhase);
      const float roar=air*(.15f+load*.65f)*(p.mode==Mode::Turbine?1.3f:.5f);
      const float engine=(low*.6f+bodyLow*p.body*.8f+combustion*p.rasp*.2f)*(p.mode==Mode::Combustion?1.f:p.mode==Mode::Ion?.14f:.07f);
      float signal=engine*(.2f+load*.8f)+p.intake*roar+mech*p.whine*.12f;
      signal+=sinCycle(turboPhase)*p.turbo*spool*.14f;
      signal+=sinCycle(shaft)*p.sub*.22f*(.45f+load*.55f);
      signal+=field*(p.reactor+(p.mode==Mode::Ion?.14f:0))*.22f*(.55f+load*.45f);
      if(p.mode==Mode::Turbine)signal+=sinCycle(carrier*4)*.07f+roar*.2f;
      const float delayed=delay[delayIndex];delay[delayIndex]=signal+delayed*.24f*p.space;delayIndex=(delayIndex+1)%delaySize;
      signal+=delayed*p.space*.45f;const float high=signal-prev+dcA*dc;prev=signal;dc=high;
      output[i]=.9f*clip(high*(1.4f+p.rasp*1.5f))*gain;
    }
  }
};
// Iris's proven output representation: signed PCM16, MSB-aligned in a 32-bit
// slot. Multiplication also avoids undefined left-shifting of negative values.
inline int32_t pcm32(float sample){return int32_t(std::lround(bounded(sample,-.99f,.99f)*32768.f))*65536;}
}
