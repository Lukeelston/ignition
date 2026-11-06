// Shared by the audio thread, render worker, and Node tests. No browser globals.
const TAU = Math.PI * 2;
const clamp = (x, a, b) => Math.max(a, Math.min(b, x));
export const CONTROLS = [
  {key:'body', label:'Engine body', min:0, max:1, step:.01, group:'Exhaust', hint:'Weight of the low resonances'},
  {key:'exhaust', label:'Exhaust tone', min:60, max:1200, step:1, unit:'Hz', group:'Exhaust', hint:'Tune the exhaust resonance'},
  {key:'rasp', label:'Rasp & grit', min:0, max:1, step:.01, group:'Exhaust', hint:'Sharper combustion and saturation'},
  {key:'roughness', label:'Idle roughness', min:0, max:1, step:.01, group:'Exhaust', hint:'Uneven firing strength and crank flutter'},
  {key:'intake', label:'Intake air', min:0, max:1, step:.01, group:'Mechanical', hint:'Air rushing under load'},
  {key:'whine', label:'Gear whine', min:0, max:1, step:.01, group:'Mechanical', hint:'Mechanical harmonics follow shaft speed'},
  {key:'turbo', label:'Turbo / spool', min:0, max:1, step:.01, group:'Mechanical', hint:'A rising whistle with spool inertia'},
  {key:'response', label:'Rev response', min:.1, max:1, step:.01, group:'Mechanical', hint:'How quickly the engine gains and loses RPM'},
  {key:'sub', label:'Sub rumble', min:0, max:1, step:.01, group:'Beyond combustion', hint:'Low shaft-order weight'},
  {key:'reactor', label:'Reactor field', min:0, max:1, step:.01, group:'Beyond combustion', hint:'Detuned carrier and ring modulation'},
  {key:'modulation', label:'Field motion', min:.1, max:30, step:.1, unit:'Hz', group:'Beyond combustion', hint:'Pulsation rate of the reactor field'},
  {key:'space', label:'Metal resonance', min:0, max:1, step:.01, group:'Beyond combustion', hint:'Short metallic reflections, not room reverb'},
];
const DEFAULT = Object.freeze({mode:'combustion', cylinders:8, stroke:4, firing:'cross', rpm:950, idle:750, redline:7000, load:.28, body:.72, exhaust:160, rasp:.35, roughness:.3, intake:.2, whine:.08, turbo:0, response:.55, sub:.18, reactor:0, modulation:4, space:.1, seed:7});
const ranges = {rpm:[350,18000], idle:[350,4000], redline:[2000,18000], load:[0,1], cylinders:[1,16], seed:[0,4294967295], ...Object.fromEntries(CONTROLS.map(c => [c.key,[c.min,c.max]]))};
const enums = {mode:['combustion','turbine','ion'], firing:['even','cross','uneven'], stroke:[2,4]};

export function validatePatch(input) {
  if (!input || typeof input !== 'object' || Array.isArray(input)) throw new Error('Choose a valid engine patch.');
  for (const key of Object.keys(input)) if (!(key in DEFAULT) || !Object.hasOwn(DEFAULT,key)) throw new Error(`Unknown patch control: ${key}`);
  const patch = {...DEFAULT, ...input};
  for (const [key,[min,max]] of Object.entries(ranges)) {
    const v = patch[key];
    if (typeof v !== 'number' || !Number.isFinite(v) || v < min || v > max) throw new Error(`${key} must be between ${min} and ${max}.`);
  }
  for (const key of ['cylinders','seed']) if (!Number.isInteger(patch[key])) throw new Error(`${key} must be a whole number.`);
  for (const [key,values] of Object.entries(enums)) if (!values.includes(patch[key])) throw new Error(`Invalid ${key}.`);
  if (patch.idle >= patch.redline) throw new Error('Redline must be above idle RPM.');
  if (patch.rpm > patch.redline) throw new Error('Target RPM must not exceed the redline.');
  return patch;
}
export function serializePatch(patch) {
  return JSON.stringify({format:'ignition-patch', version:1, patch:validatePatch(patch)}, null, 2);
}
export function parsePatch(text) {
  if (typeof text !== 'string' || text.length > 32768) throw new Error('Patch file is too large.');
  let doc;
  try { doc = JSON.parse(text); } catch { throw new Error('This file is not valid JSON.'); }
  if (!doc || doc.format !== 'ignition-patch' || doc.version !== 1 || !doc.patch) throw new Error('Use an Ignition v1 patch file.');
  return validatePatch(doc.patch);
}
const preset = (id, name, family, note, values) => Object.freeze({id,name,family,note,patch:Object.freeze(validatePatch(values))});
export const PRESETS = Object.freeze([
  preset('v8','Big-block V8','Road','A heavy, uneven burble. Open the throttle and let it bark.', {}),
  preset('six','Straight six','Road','An even pulse train with a smooth mechanical edge.', {cylinders:6,firing:'even',rpm:1100,idle:800,redline:7500,exhaust:240,roughness:.08,body:.5,rasp:.24,whine:.19}),
  preset('four','Track four','Track','Light, sharp and happiest on the far side of 8,000 RPM.', {cylinders:4,firing:'even',rpm:1400,idle:1200,redline:13000,exhaust:410,body:.22,rasp:.6,roughness:.06,intake:.38,whine:.28,response:.9,sub:.03}),
  preset('twin','V-twin','Road','An uneven two-cylinder beat with a slow, weighty crank.', {cylinders:2,firing:'uneven',rpm:850,idle:650,redline:6500,exhaust:105,body:.88,rasp:.34,roughness:.48,response:.35,sub:.32}),
  preset('diesel','Heavy diesel','Utility','Low-speed combustion, mechanical clatter and a whistling turbo.', {cylinders:6,firing:'even',rpm:650,idle:500,redline:3500,exhaust:90,body:1,rasp:.64,roughness:.45,intake:.36,whine:.23,turbo:.4,response:.2,sub:.42}),
  preset('turbine','Jet turbine','Flight','A spinning core, a broad air roar and a slow spool-up.', {mode:'turbine',cylinders:12,firing:'even',rpm:2400,idle:1800,redline:16000,body:.45,exhaust:520,rasp:.12,roughness:.04,intake:.8,whine:.65,turbo:.8,response:.22,sub:.3,space:.25}),
  preset('ion','Ion drive','Off-world','A clear carrier with a restless, modulated field underneath.', {mode:'ion',cylinders:4,firing:'even',rpm:1800,idle:900,redline:11000,body:.3,exhaust:350,rasp:.08,roughness:.08,intake:.15,whine:.32,turbo:.1,response:.6,sub:.5,reactor:.8,modulation:7.2,space:.55}),
  preset('freighter','Deep-space hauler','Off-world','An enormous powerplant, slightly overdue for its last service.', {mode:'ion',cylinders:12,firing:'cross',rpm:700,idle:450,redline:5000,body:.95,exhaust:70,rasp:.25,roughness:.65,intake:.4,whine:.26,turbo:.2,response:.15,sub:.9,reactor:.65,modulation:1.8,space:.75}),
]);

export class EngineSynth {
  constructor(patch = DEFAULT, sampleRate = 48000) {
    if (!Number.isFinite(sampleRate) || sampleRate < 8000 || sampleRate > 192000) throw new Error('Unsupported sample rate.');
    this.sr = sampleRate;
    this.target = validatePatch(patch); this.p = {...this.target};
    this.rpm = this.p.rpm; this.load = this.p.load; this.spool = 0;
    this.rng = this.p.seed || 0x6d2b79f5;
    this.phase = 0; this.shaft = 0; this.carrier = 0; this.field = 0; this.turboPhase = 0;
    this.pulse = 0; this.edge = 0; this.low = 0; this.air = 0; this.bodyLow = 0;
    this.prev = 0; this.dc = 0; this.firings = 0;
    this.delay = new Float32Array(Math.round(sampleRate * .027)); this.delayIndex = 0;
  }
  set(patch) {
    const next=validatePatch({...this.target,...patch});
    if(next.seed!==this.target.seed) { this.rng=next.seed||0x6d2b79f5;this.p.seed=next.seed; }
    this.target=next;
  }
  noise() {
    let x = this.rng; x ^= x << 13; x ^= x >>> 17; x ^= x << 5; this.rng = x >>> 0;
    return this.rng / 2147483648 - 1;
  }
  process(output) {
    const p = this.p, t = this.target, sr = this.sr;
    const ease = 1 - Math.exp(-output.length / (sr * .04));
    for (const key of Object.keys(ranges)) if (!['rpm','load','seed','cylinders'].includes(key)) p[key] += (t[key] - p[key]) * ease;
    p.mode=t.mode; p.firing=t.firing; p.cylinders=t.cylinders; p.stroke=t.stroke;
    const rpmEase = 1 - Math.exp(-1 / (sr * (.045 + (1-p.response)*.52)));
    const loadEase = 1 - Math.exp(-1/(sr*.04));
    const pulseDecay = Math.exp(-1/(sr*(.002 + p.body*.005)));
    const edgeDecay = Math.exp(-1/(sr*.00045));
    const lowA = 1 - Math.exp(-TAU*(p.exhaust*2.5 + p.rasp*2400)/sr);
    const bodyA = 1 - Math.exp(-TAU*p.exhaust/sr);
    const airA = 1 - Math.exp(-TAU*1500/sr);
    const dcA = Math.exp(-TAU*18/sr);
    const crossGains = [1,.52,.75,.52,.85,1,.52,.75];
    for (let i=0; i<output.length; i++) {
      this.rpm += (t.rpm-this.rpm)*rpmEase;
      this.load += (t.load-this.load)*loadEase;
      const n = this.noise();
      this.shaft = (this.shaft + this.rpm/(60*sr)) % 1;
      this.field = (this.field + p.modulation/sr) % 1;
      const flutter = 1 + p.roughness*.025*Math.sin(TAU*this.field);
      this.phase += this.rpm*p.cylinders / ((p.stroke===4 ? 120 : 60)*sr) * flutter;
      const spacing = p.firing==='uneven' ? (this.firings%2 ? 1.38:.62) : 1;
      if (this.phase >= spacing) {
        this.phase -= spacing;
        const weight = (p.firing==='cross' ? crossGains[this.firings%8] : 1) * (1+p.roughness*n*.32);
        this.pulse += weight; this.edge += weight; this.firings++;
      }
      this.pulse *= pulseDecay; this.edge *= edgeDecay;
      const combustion = this.pulse - this.edge;
      this.low += lowA*(combustion-this.low);
      this.bodyLow += bodyA*(this.low-this.bodyLow);
      this.air += airA*(n-this.air);
      this.spool += ((this.rpm/p.redline)*(.25+.75*this.load)-this.spool)/(sr*.38);
      this.turboPhase = (this.turboPhase + (650+this.spool*4500)/sr)%1;
      const shaftHz = this.rpm/60;
      this.carrier = (this.carrier + Math.min(sr*.18, 80+shaftHz*2.7)/sr)%1;
      const mech = Math.sin(TAU*this.shaft*6)*.65 + Math.sin(TAU*this.shaft*12)*.2;
      const field = Math.sin(TAU*this.carrier) * (.65+.35*Math.sin(TAU*this.field)) + .3*Math.sin(TAU*(this.carrier*2+this.field));
      const roar = this.air * (.15+this.load*.65) * (p.mode==='turbine' ? 1.3 : .5);
      const engine = (this.low*.6+this.bodyLow*p.body*.8+combustion*p.rasp*.2)*(p.mode==='combustion' ? 1 : p.mode==='ion' ? .14 : .07);
      let signal = engine*(.2+this.load*.8) + p.intake*roar + mech*p.whine*.12;
      signal += Math.sin(TAU*this.turboPhase)*p.turbo*this.spool*.14;
      signal += Math.sin(TAU*this.shaft)*p.sub*.22*(.45+this.load*.55);
      signal += field*(p.reactor+(p.mode==='ion' ? .14 : 0))*.22*(.55+this.load*.45);
      if (p.mode==='turbine') signal += Math.sin(TAU*this.carrier*4)*.07 + roar*.2;
      const delayed = this.delay[this.delayIndex];
      this.delay[this.delayIndex] = signal + delayed*.24*p.space;
      this.delayIndex = (this.delayIndex+1)%this.delay.length;
      signal += delayed*p.space*.45;
      // DC removal before the saturator; the pulse train has a substantial DC component.
      const high = signal-this.prev+dcA*this.dc; this.prev=signal; this.dc=high;
      output[i] = .9*Math.tanh(high*(1.4+p.rasp*1.5));
    }
    return output;
  }
}

export function trajectory(patch, motion, progress) {
  let fraction, load;
  const u=clamp(progress,0,1);
  if (motion==='steady') return {rpm:patch.rpm,load:patch.load};
  if (motion==='rev') { fraction=Math.pow(Math.max(0,Math.sin(Math.PI*u)),1.7); load=.12+.88*fraction; }
  else if (motion==='sweep') { fraction=u; load=.8; }
  else if (motion==='gears') {
    const gear = Math.min(3,Math.floor(u*4)), part=(u*4)%1;
    fraction = gear===0 ? .05+part*.92 : .48+part*.49;
    load=part<.06 && gear>0 ? .12 : .92;
  } else throw new Error('Choose a valid motion.');
  return {rpm:patch.idle+(patch.redline-patch.idle)*fraction,load};
}
export function renderClip(input, {duration=8,motion='rev',sampleRate=48000}={}) {
  const patch=validatePatch(input);
  if (!Number.isFinite(duration) || duration<2 || duration>30) throw new Error('Duration must be 2–30 seconds.');
  if (![44100,48000].includes(sampleRate)) throw new Error('Export at 44.1 or 48 kHz.');
  const initial = trajectory(patch,motion,0);
  const synth=new EngineSynth({...patch,...initial},sampleRate);
  // Warm up filters for a steady tone; output edges are still faded below.
  synth.process(new Float32Array(Math.round(sampleRate*.15)));
  const samples=new Float32Array(Math.round(duration*sampleRate));
  for(let offset=0;offset<samples.length;offset+=128) {
    synth.set(trajectory(patch,motion,offset/samples.length));
    synth.process(samples.subarray(offset,Math.min(offset+128,samples.length)));
  }
  const fade=Math.round(sampleRate*.015);
  for(let i=0;i<fade;i++) {
    const gain=.5-.5*Math.cos(Math.PI*i/fade);
    samples[i]*=gain; samples[samples.length-1-i]*=gain;
  }
  return samples;
}
export function encodeWav(samples, sampleRate=48000) {
  if (!(samples instanceof Float32Array) || !Number.isInteger(sampleRate) || sampleRate<8000 || sampleRate>192000) throw new Error('Invalid audio buffer.');
  const bytes=samples.length*3, buffer=new ArrayBuffer(44+bytes+(bytes%2)), view=new DataView(buffer);
  const tag=(offset,text)=>[...text].forEach((ch,i)=>view.setUint8(offset+i,ch.charCodeAt(0)));
  tag(0,'RIFF');view.setUint32(4,buffer.byteLength-8,true);tag(8,'WAVE');tag(12,'fmt ');
  view.setUint32(16,16,true);view.setUint16(20,1,true);view.setUint16(22,1,true);
  view.setUint32(24,sampleRate,true);view.setUint32(28,sampleRate*3,true);view.setUint16(32,3,true);view.setUint16(34,24,true);
  tag(36,'data');view.setUint32(40,bytes,true);
  for(let i=0;i<samples.length;i++) {
    if (!Number.isFinite(samples[i])) throw new Error('Audio contains an invalid sample.');
    const x=clamp(samples[i],-1,1), q=Math.round(x*(x<0 ? 8388608 : 8388607)), at=44+i*3;
    view.setUint8(at,q&255);view.setUint8(at+1,(q>>8)&255);view.setUint8(at+2,(q>>16)&255);
  }
  return buffer;
}
