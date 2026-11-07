#pragma once
#include <algorithm>
#include <cmath>

namespace ignition {
// Transparent gain below the knee, gently rounded peaks above it. Stateless:
// no compressor release envelope that could pump or shake a steady engine note.
inline float speakerSample(float sample,float makeup) {
  if(!std::isfinite(sample)||!std::isfinite(makeup))return 0;
  const float boosted=std::max(-8.f,std::min(8.f,sample))*std::max(0.f,std::min(4.f,makeup));
  const float magnitude=std::fabs(boosted);
  if(magnitude<=.8f)return boosted;
  const float excess=magnitude-.8f;
  const float rounded=.8f+.16f*excess/(.16f+excess);
  return boosted<0?-rounded:rounded;
}
}
