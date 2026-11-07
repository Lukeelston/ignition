// Generated from web/engine.js by tools/export_firmware_patch.mjs.
#pragma once
#include "core.h"
namespace ignition {
inline const Patch kPatches[]={
  {Mode::Combustion,Firing::Cross,8,4,7u,950.0f,750.0f,7000.0f,0.28f,0.72f,160.0f,0.35f,0.3f,0.2f,0.08f,0.0f,0.55f,0.18f,0.0f,4.0f,0.1f},
  {Mode::Combustion,Firing::Even,6,4,7u,1100.0f,800.0f,7500.0f,0.28f,0.5f,240.0f,0.24f,0.08f,0.2f,0.19f,0.0f,0.55f,0.18f,0.0f,4.0f,0.1f},
  {Mode::Combustion,Firing::Even,4,4,7u,1400.0f,1200.0f,13000.0f,0.28f,0.22f,410.0f,0.6f,0.06f,0.38f,0.28f,0.0f,0.9f,0.03f,0.0f,4.0f,0.1f},
  {Mode::Combustion,Firing::Uneven,2,4,7u,850.0f,650.0f,6500.0f,0.28f,0.88f,105.0f,0.34f,0.48f,0.2f,0.08f,0.0f,0.35f,0.32f,0.0f,4.0f,0.1f},
  {Mode::Combustion,Firing::Even,6,4,7u,650.0f,500.0f,3500.0f,0.28f,1.0f,90.0f,0.64f,0.45f,0.36f,0.23f,0.4f,0.2f,0.42f,0.0f,4.0f,0.1f},
  {Mode::Turbine,Firing::Even,12,4,7u,2400.0f,1800.0f,16000.0f,0.28f,0.45f,520.0f,0.12f,0.04f,0.8f,0.65f,0.8f,0.22f,0.3f,0.0f,4.0f,0.25f},
  {Mode::Ion,Firing::Even,4,4,7u,1800.0f,900.0f,11000.0f,0.28f,0.3f,350.0f,0.08f,0.08f,0.15f,0.32f,0.1f,0.6f,0.5f,0.8f,7.2f,0.55f},
  {Mode::Ion,Firing::Cross,12,4,7u,700.0f,450.0f,5000.0f,0.28f,0.95f,70.0f,0.25f,0.65f,0.4f,0.26f,0.2f,0.15f,0.9f,0.65f,1.8f,0.75f}
};
inline constexpr const char* kPresetNames[]={"Big-block V8","Straight six","Track four","V-twin","Heavy diesel","Jet turbine","Ion drive","Deep-space hauler"};
inline constexpr size_t kPresetCount=sizeof(kPatches)/sizeof(kPatches[0]);
}
