#pragma once
#include "core.h"

namespace ignition {
// Eight quiet 660 Hz beeps, independent of the engine synthesizer. Keep the
// existing sample rate, PCM format and I2S driver to isolate the sound source.
class TestTone {
  uint32_t frame=totalFrames;
public:
  static constexpr uint32_t leadFrames=sampleRate/5;
  static constexpr uint32_t periodFrames=sampleRate*3/2;
  static constexpr uint32_t totalFrames=leadFrames+8*periodFrames;
  void start(){frame=0;}
  void stop(){frame=totalFrames;}
  bool active()const{return frame<totalFrames;}
  float next(){
    if(!active())return 0;
    const uint32_t current=frame++;
    if(current<leadFrames)return 0;
    const uint32_t position=(current-leadFrames)%periodFrames;
    if(position>=sampleRate)return 0;
    const float envelope=std::min(1.f,float(std::min(position,sampleRate-1-position))/(sampleRate/50));
    return (768.f/32768.f)*envelope*std::sin(float(6.283185307179586*660*position/sampleRate));
  }
};
}
