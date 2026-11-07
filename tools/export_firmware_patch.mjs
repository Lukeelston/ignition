// Regenerate shipping presets, optionally adding one saved browser patch.
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {fileURLToPath} from 'node:url';
import {PRESETS,parsePatch} from '../web/engine.js';
const folder=new URL('../firmware/ignition/',import.meta.url);
const patches=PRESETS.map(p=>({name:p.name,patch:p.patch}));
if(process.argv[2])patches.push({name:'Custom engine',patch:parsePatch(await readFile(process.argv[2],'utf8'))});
const mode={combustion:'Combustion',turbine:'Turbine',ion:'Ion'},firing={even:'Even',cross:'Cross',uneven:'Uneven'};
const fields=['rpm','idle','redline','load','body','exhaust','rasp','roughness','intake','whine','turbo','response','sub','reactor','modulation','space'];
const float=n=>`${Number.isInteger(n)?n.toFixed(1):n}f`;
const bodies=patches.map(({patch:p})=>`  {Mode::${mode[p.mode]},Firing::${firing[p.firing]},${p.cylinders},${p.stroke},${p.seed}u,${fields.map(k=>float(p[k])).join(',')}}`);
const source=`// Generated from web/engine.js by tools/export_firmware_patch.mjs.\n#pragma once\n#include "core.h"\nnamespace ignition {\ninline const Patch kPatches[]={\n${bodies.join(',\n')}\n};\ninline constexpr const char* kPresetNames[]={${patches.map(p=>JSON.stringify(p.name)).join(',')}};\ninline constexpr size_t kPresetCount=sizeof(kPatches)/sizeof(kPatches[0]);\n}\n`;
await mkdir(folder,{recursive:true});await writeFile(new URL('presets.h',folder),source);
console.log(`Exported ${patches.length} patches to ${fileURLToPath(folder)}presets.h`);
