#pragma once
#include <algorithm>
#include <cmath>

namespace ignition {
// Constant-peak-gain bandpass, RBJ biquad form. Coefficients are updated once
// per audio block; each sample needs only multiply/add operations.
class ExhaustResonator {
  float z1=0,z2=0,b=0,a1=0,a2=0;
public:
  void tune(float hz,float q,float sr){
    const float w=6.28318530718f*std::min(hz,sr*.2f)/sr;
    const float alpha=std::sin(w)/(2*q),a0=1+alpha;
    b=alpha/a0;a1=-2*std::cos(w)/a0;a2=(1-alpha)/a0;
  }
  float next(float x){
    const float y=b*x+z1;z1=-a1*y+z2;z2=-b*x-a2*y;return y;
  }
};

class V8ExhaustBank {
  float pulse=0,edge=0,prev=0,dc=0,mouth=0;
  float decay=0,openDecay=0,edgeDecay=0,dcA=0,closedA=0,openA=0;
  ExhaustResonator body,throat;
public:
  void tune(float bodyAmount,float exhaust,float rasp,float rpm,float redline,float sr,float detune){
    const float tau=6.28318530718f;
    const float width=(.0016f+bodyAmount*.0018f)/(1+rpm/redline*1.2f);
    decay=std::exp(-1/(sr*width));openDecay=std::exp(-1/(sr*width*.45f));
    edgeDecay=std::exp(-1/(sr*.00022f));dcA=std::exp(-tau*28/sr);
    closedA=1-std::exp(-tau*(180+rasp*200)/sr);
    openA=1-std::exp(-tau*std::min(sr*.22f,2200+rasp*2800)/sr);
    body.tune(exhaust*.72f*detune,.85f+bodyAmount*.65f,sr);
    throat.tune(exhaust*4.5f*detune,.9f,sr);
  }
  void fire(float weight){pulse+=weight;edge+=weight;}
  float next(float load,float bodyAmount,float rasp){
    pulse*=decay+(openDecay-decay)*load;edge*=edgeDecay;
    const float pressure=pulse-edge,high=pressure-prev+dcA*dc;prev=pressure;dc=high;
    mouth+=(closedA+(openA-closedA)*load)*(high-mouth);
    const float chest=body.next(high),bark=throat.next(high);
    const float voice=mouth*(.1f+load*.95f)+chest*bodyAmount*(1.15f-load*.3f)+bark*(.03f+rasp*load*1.8f);
    const float driven=voice*(.9f+load*.8f);
    return driven/(1+.65f*std::fabs(driven));
  }
};
}
