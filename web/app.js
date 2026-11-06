import {PRESETS,CONTROLS,validatePatch,serializePatch,parsePatch,trajectory} from './engine.js';
const $=id=>document.getElementById(id);
const STORAGE='elston-ignition-patch-v1';
let patch={...PRESETS[0].patch}, presetId=PRESETS[0].id, patchName=PRESETS[0].name;
let context=null, node=null, gain=null, analyser=null, busy=false, running=false, muted=false;
let held=false, revStart=null, lastSent=0, currentRPM=0, frame=0, worker=null, renderTimer=null, previewUrl=null;
let scopeData=new Float32Array(1024);
const status=(message,error=false)=>{ $('status').textContent=message; $('status').dataset.error=String(error); };
const format=(value,control)=>control.unit ? `${value.toFixed(control.step<1 ? 1:0)} ${control.unit}` : `${Math.round(value*100)}%`;

for(const preset of PRESETS) {
  const button=document.createElement('button');button.className='preset';button.setAttribute('aria-label',preset.name);button.dataset.id=preset.id;
  const name=document.createTextNode(preset.name), family=document.createElement('span');family.textContent=preset.family;
  button.append(name,family);button.addEventListener('click',()=>selectPreset(preset));$('presets').append(button);
}
for(const group of ['Exhaust','Mechanical','Beyond combustion']) {
  const section=document.createElement('section');section.className='control-group';
  const title=document.createElement('div');title.className='group-heading';
  const heading=document.createElement('h3');heading.textContent=group;title.append(heading);
  const hint=document.createElement('span');hint.textContent=group==='Exhaust'?'Pulse & texture':group==='Mechanical'?'Air & moving parts':'Bend the laws a little';title.append(hint);
  const grid=document.createElement('div');grid.className='control-grid';
  for(const control of CONTROLS.filter(c=>c.group===group)) {
    const box=document.createElement('div');box.className='sound-control';box.title=control.hint;
    const label=document.createElement('label');label.className='slider-label';label.htmlFor=control.key;
    const text=document.createElement('span');text.textContent=control.label;
    const output=document.createElement('output');output.id=`${control.key}-value`;output.htmlFor=control.key;label.append(text,output);
    const input=document.createElement('input');input.type='range';input.id=control.key;input.min=control.min;input.max=control.max;input.step=control.step;
    input.addEventListener('input',()=>change(control.key,Number(input.value)));box.append(label,input);grid.append(box);
  }
  section.append(title,grid);$('sound-controls').append(section);
}

function persist() {
  try { localStorage.setItem(STORAGE,serializePatch(patch)); } catch { /* File export remains available if storage is disabled. */ }
}
function renderDial() {
  const root=$('dial-ticks');root.replaceChildren();
  const svg=(tag,attrs)=>{const el=document.createElementNS('http://www.w3.org/2000/svg',tag);for(const [k,v] of Object.entries(attrs))el.setAttribute(k,v);return el;};
  for(let i=0;i<=40;i++) {
    const angle=(-130+i*6.5)*Math.PI/180, major=i%5===0;
    const point=r=>[220+Math.sin(angle)*r,170-Math.cos(angle)*r];
    const [x1,y1]=point(major?119:126),[x2,y2]=point(136);
    root.append(svg('line',{x1,y1,x2,y2,stroke:i>=34?'#e37c65':major?'#b6c9d2':'#506c7b','stroke-width':major?2:1}));
    if(major){const [x,y]=point(153);const text=svg('text',{x,y:y+4,'text-anchor':'middle',class:'tick-label'});text.textContent=Number((patch.redline*i/40/1000).toFixed(1));root.append(text);}
  }
  $('dial-caption').textContent=patch.mode==='combustion' ? `${patch.cylinders} cylinders · ${patch.stroke}-stroke` : patch.mode==='turbine' ? 'Turbine shaft speed' : 'Reactor drive speed';
}
function syncUI() {
  $('rpm').max=patch.redline;
  for(const [key,value] of Object.entries(patch)) if($(key)) $(key).value=value;
  $('rpm-value').textContent=Math.round(patch.rpm).toLocaleString();$('load-value').textContent=`${Math.round(patch.load*100)}%`;
  for(const c of CONTROLS) $(`${c.key}-value`).textContent=format(patch[c.key],c);
  document.querySelectorAll('.preset').forEach(button=>button.setAttribute('aria-pressed',String(button.dataset.id===presetId)));
  $('patch-state').textContent=patchName;renderDial();
}
function sendLive() {
  if(!node)return;
  let motion={rpm:patch.rpm,load:patch.load};
  if(held)motion={rpm:patch.redline,load:1};
  else if(revStart!==null&&context)motion=trajectory(patch,'rev',Math.min(1,(context.currentTime-revStart)/4));
  node.port.postMessage({patch:{...patch,...motion}});
}
function change(key,value) {
  try {
    const candidate={...patch,[key]:value};
    if(key==='redline')candidate.rpm=Math.min(candidate.rpm,value);
    patch=validatePatch(candidate);patchName='Custom engine';
    syncUI();sendLive();persist();
  } catch(error) { status(error.message,true);syncUI(); }
}
function selectPreset(preset) {
  patch={...preset.patch};presetId=preset.id;patchName=preset.name;releaseRev();
  $('engine-name').textContent=preset.name;$('engine-family').textContent=preset.family;$('preset-note').textContent=preset.note;
  syncUI();sendLive();persist();status(`${preset.name} loaded. All controls are yours to change.`);
}
for(const key of ['rpm','load']) $(key).addEventListener('input',()=>change(key,Number($(key).value)));
for(const key of ['mode','firing','stroke','cylinders','idle','redline','seed']) $(key).addEventListener('change',()=>change(key,['mode','firing'].includes(key)?$(key).value:Number($(key).value)));
$('reset').addEventListener('click',()=>selectPreset(PRESETS.find(p=>p.id===presetId)||PRESETS[0]));

function stateUI() {
  document.body.dataset.running=String(running);$('ignition').textContent=running?'Stop engine':'Start engine';$('ignition').disabled=busy;
  $('rev').disabled=!running||busy;$('sweep').disabled=!running||busy;
  $('state-label').textContent=running?'Running':'Engine off';
}
async function stop() {
  releaseRev();running=false;cancelAnimationFrame(frame);currentRPM=0;
  const old=context;context=null;
  if(old&&old.state!=='closed') {
    gain?.gain.setTargetAtTime(0,old.currentTime,.008);
    await new Promise(resolve=>setTimeout(resolve,45));
    await old.close();
  }
  node=null;gain=null;analyser=null;stateUI();draw();
}
async function toggleEngine() {
  if(busy)return;busy=true;stateUI();
  try {
    if(running) { await stop();status('Engine stopped. Your patch is saved on this browser.'); }
    else {
      context=new AudioContext({sampleRate:48000,latencyHint:'interactive'});
      if(!context.audioWorklet)throw new Error('Open Ignition through its launcher in Chrome.');
      await context.audioWorklet.addModule('worklet.js');
      node=new AudioWorkletNode(context,'ignition-engine',{numberOfInputs:0,numberOfOutputs:1,outputChannelCount:[1],processorOptions:{patch}});
      gain=context.createGain();gain.gain.value=0;analyser=context.createAnalyser();analyser.fftSize=2048;
      scopeData=new Float32Array(analyser.fftSize);node.connect(gain);gain.connect(analyser);analyser.connect(context.destination);
      node.port.onmessage=({data})=>{if(data.error)status(data.error,true);else currentRPM=data.rpm;};
      node.onprocessorerror=()=>{status('The audio engine stopped. Restart the engine to recover.',true);stop().catch(()=>{});};
      await context.resume();running=true;gain.gain.setTargetAtTime(muted?0:Number($('volume').value),context.currentTime,.035);
      status('Engine running. Hold to rev, or shape the sound live.');frame=requestAnimationFrame(animate);
    }
  } catch(error) { await stop();status(`Could not start audio: ${error.message}`,true); }
  finally {busy=false;stateUI();}
}
$('ignition').addEventListener('click',toggleEngine);
function holdRev() {if(!running)return;held=true;revStart=null;$('rev').setAttribute('aria-pressed','true');sendLive();}
function releaseRev() {held=false;revStart=null;$('rev').setAttribute('aria-pressed','false');sendLive();}
$('rev').addEventListener('pointerdown',event=>{event.preventDefault();$('rev').focus();$('rev').setPointerCapture(event.pointerId);holdRev();});
for(const type of ['pointerup','pointercancel','lostpointercapture','blur']) $('rev').addEventListener(type,releaseRev);
$('rev').addEventListener('keydown',event=>{if([' ','Enter'].includes(event.key)){event.preventDefault();if(!event.repeat)holdRev();}});
$('rev').addEventListener('keyup',event=>{if([' ','Enter'].includes(event.key)){event.preventDefault();releaseRev();}});
window.addEventListener('blur',releaseRev);document.addEventListener('visibilitychange',()=>{if(document.hidden)releaseRev();});
$('sweep').addEventListener('click',()=>{if(context){held=false;revStart=context.currentTime;sendLive();}});
function monitor() {const level=Number($('volume').value);$('volume-value').textContent=`${Math.round(level*100)}%`;if(context&&gain)gain.gain.setTargetAtTime(muted?0:level,context.currentTime,.02);}
$('volume').addEventListener('input',monitor);$('mute').addEventListener('click',()=>{muted=!muted;$('mute').setAttribute('aria-pressed',String(muted));$('mute').textContent=muted?'Unmute':'Mute';monitor();});

function draw() {
  const rpm=running?currentRPM:0;$('rpm-readout').textContent=Math.round(rpm).toLocaleString();
  $('needle').setAttribute('transform',`rotate(${-130+Math.min(1,rpm/patch.redline)*260} 220 170)`);
  const canvas=$('scope'),ctx=canvas.getContext('2d'),w=canvas.width,h=canvas.height;
  ctx.clearRect(0,0,w,h);ctx.strokeStyle='#35505f';ctx.lineWidth=1;ctx.beginPath();ctx.moveTo(0,h/2);ctx.lineTo(w,h/2);ctx.stroke();
  if(analyser&&running)analyser.getFloatTimeDomainData(scopeData);else scopeData.fill(0);
  let energy=0;ctx.strokeStyle='#83bac9';ctx.lineWidth=1.6;ctx.beginPath();
  for(let i=0;i<scopeData.length;i++){energy+=scopeData[i]**2;const x=i/(scopeData.length-1)*w,y=h/2-scopeData[i]*h*1.7;if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}ctx.stroke();
  const db=20*Math.log10(Math.sqrt(energy/scopeData.length));$('output-level').textContent=Number.isFinite(db)&&db>-90?`${db.toFixed(1)} dBFS`:'— dBFS';
}
function animate(now) {
  if(!running)return;
  if(revStart!==null&&context) {if(context.currentTime-revStart>=4)releaseRev();else if(now-lastSent>40){sendLive();lastSent=now;}}
  draw();frame=requestAnimationFrame(animate);
}

function downloadBlob(blob,name) {
  const url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
}
$('save').addEventListener('click',()=>{downloadBlob(new Blob([serializePatch(patch)],{type:'application/json'}),`ignition-${presetId||'custom'}-patch.json`);status('Patch saved, including its variation seed.');});
$('load-patch').addEventListener('click',()=>$('patch-file').click());
$('patch-file').addEventListener('change',async()=>{
  const file=$('patch-file').files[0];if(!file)return;
  try {
    if(file.size>32768)throw new Error('Patch files must be smaller than 32 KB.');
    const loaded=parsePatch(await file.text());
    patch=loaded;presetId=null;patchName='Loaded patch';releaseRev();$('engine-name').textContent='Custom engine';$('engine-family').textContent='Your patch';$('preset-note').textContent='A saved engine, ready for another experiment.';
    syncUI();sendLive();persist();status('Patch loaded.');
  } catch(error) {status(`Patch not loaded: ${error.message}`,true);}
  finally {$('patch-file').value='';}
});
function finishRender() {clearTimeout(renderTimer);worker?.terminate();worker=null;$('render').disabled=false;$('render').textContent='Render WAV';}
$('render').addEventListener('click',()=>{
  if(worker)return;
  const duration=Number($('duration').value),motion=$('motion').value;
  if(!Number.isFinite(duration)||duration<2||duration>30){status('Choose a render length from 2 to 30 seconds.',true);return;}
  const snapshot={...patch},name=`ignition-${presetId||'custom'}-${motion}-${snapshot.seed}.wav`;
  $('render').disabled=true;$('render').textContent='Rendering…';status('Rendering your engine locally…');
  try {
    worker=new Worker('render-worker.js',{type:'module'});
    worker.onmessage=({data})=>{
      finishRender();if(data.error){status(data.error,true);return;}
      const blob=new Blob([data.wav],{type:'audio/wav'});
      if(previewUrl)URL.revokeObjectURL(previewUrl);previewUrl=URL.createObjectURL(blob);
      $('preview').pause();$('preview').src=previewUrl;$('preview').volume=.5;
      $('download').href=previewUrl;$('download').download=name;$('render-result').hidden=false;
      $('render-info').textContent=`${duration}s · ${(blob.size/1024/1024).toFixed(1)} MB`;
      downloadBlob(blob,name);status('WAV rendered and downloaded. Audition it below or download it again.');
    };
    worker.onerror=event=>{finishRender();status(`Render failed: ${event.message}`,true);};
    renderTimer=setTimeout(()=>{finishRender();status('Render timed out. Try a shorter clip.',true);},60000);
    worker.postMessage({patch:snapshot,options:{duration,motion,sampleRate:48000}});
  } catch(error) {finishRender();status(`Could not render: ${error.message}`,true);}
});
$('preview').addEventListener('play',()=>{if(running)toggleEngine();});
$('quit').addEventListener('click',async()=>{
  if(busy)return;
  await stop();finishRender();$('preview').pause();
  try {const response=await fetch('/__quit',{method:'POST',headers:{'X-Ignition-Control':'quit'}});if(!response.ok)throw new Error('Server refused the quit request.');status('Ignition is closed. You can close this window.');$('ignition').disabled=true;$('render').disabled=true;$('quit').disabled=true;}
  catch(error){status(`Could not close the server: ${error.message} Close its console window instead.`,true);}
});
window.addEventListener('pagehide',()=>{worker?.terminate();if(context)context.close().catch(()=>{});if(previewUrl)URL.revokeObjectURL(previewUrl);});

try {
  const saved=localStorage.getItem(STORAGE);
  if(saved) {
    patch=parsePatch(saved);presetId=null;patchName='Restored patch';
    $('engine-name').textContent='Custom engine';$('engine-family').textContent='Your patch';
    $('preset-note').textContent='Your last engine, right where you left it.';
    syncUI();status('Your last patch is restored. Start the engine when you’re ready.');
  } else selectPreset(PRESETS[0]);
} catch {
  selectPreset(PRESETS[0]);status('The saved patch could not be restored. Loaded the V8 starting point.',true);
}
draw();
