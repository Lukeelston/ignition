import test from 'node:test';
import assert from 'node:assert/strict';

let api;
try { api = await import('../web/engine.js'); } catch {}
test('engine synthesis is available', () => assert.ok(api?.EngineSynth, 'Engine synthesis module must exist'));

const rms = a => Math.sqrt(a.reduce((s, x) => s + x*x, 0) / a.length);
const peak = a => a.reduce((v, x) => Math.max(v, Math.abs(x)), 0);
if (api) {
  const {EngineSynth, PRESETS, renderClip, encodeWav, parsePatch, serializePatch, validatePatch} = api;
  const base = PRESETS[0].patch;
  test('all presets produce finite, bounded and non-silent audio', () => {
    for (const preset of PRESETS) {
      const a = renderClip(preset.patch, {duration: 2, motion: 'rev'});
      assert.equal(a.length, 96000);
      assert.ok(a.every(Number.isFinite), preset.name);
      assert.ok(peak(a) < 0.96 && rms(a) > 0.005, preset.name);
      assert.ok(Math.abs(a[0]) < 1e-6 && Math.abs(a.at(-1)) < 1e-6);
    }
  });
  test('same patch and seed reproduce exactly; different seed changes texture', () => {
    const a = renderClip(base, {duration: 2, motion: 'steady'});
    assert.deepEqual(a, renderClip(base, {duration: 2, motion: 'steady'}));
    assert.notDeepEqual(a, renderClip({...base, seed: 99}, {duration: 2, motion: 'steady'}));
  });
  test('four-stroke V8 at 3000 RPM fires 200 times per second', () => {
    const synth = new EngineSynth({...base, rpm: 3000, cylinders: 8, stroke: 4, roughness: 0}, 48000);
    synth.process(new Float32Array(48000));
    assert.ok(Math.abs(synth.firings - 200) <= 1, `fired ${synth.firings}`);
  });
  test('two-stroke four at 3000 RPM fires 200 times per second', () => {
    const synth = new EngineSynth({...base, rpm: 3000, cylinders: 4, stroke: 2, roughness: 0}, 48000);
    synth.process(new Float32Array(48000));
    assert.ok(Math.abs(synth.firings - 200) <= 1, `fired ${synth.firings}`);
  });
  test('doubling RPM doubles the actual firing count', () => {
    const a = new EngineSynth({...base, rpm: 1800, roughness: 0});
    const b = new EngineSynth({...base, rpm: 3600, roughness: 0});
    a.process(new Float32Array(48000)); b.process(new Float32Array(48000));
    assert.ok(Math.abs(b.firings - 2 * a.firings) <= 2);
  });
  test('load changes level and sound; RPM changes smoothly', () => {
    const a = renderClip({...base, load: 0}, {duration: 2, motion: 'steady'});
    const b = renderClip({...base, load: 1}, {duration: 2, motion: 'steady'});
    assert.ok(rms(b) > rms(a) * 1.15);
    const synth = new EngineSynth({...base, rpm: 1000});
    synth.set({...base, rpm: 8000, redline:8500}); synth.process(new Float32Array(128));
    assert.ok(synth.rpm > 1000 && synth.rpm < 2000);
  });
  test('extreme layer mixes stay finite at minimum and maximum speed', () => {
    for (const rpm of [350, 18000]) {
      const a = renderClip({...base, rpm, redline: 18000, cylinders: 16, load: 1, body: 1, rasp: 1, intake: 1, whine: 1, turbo: 1, roughness: 1, sub: 1, reactor: 1}, {duration: 2, motion: 'steady'});
      assert.ok(a.every(Number.isFinite)); assert.ok(peak(a) < 0.96);
    }
  });
  test('changing seed affects a running engine without resetting on unchanged patch messages', () => {
    const a=new EngineSynth(base), b=new EngineSynth(base);
    a.process(new Float32Array(4800));b.process(new Float32Array(4800));
    a.set({...base});
    assert.deepEqual(a.process(new Float32Array(4800)), b.process(new Float32Array(4800)));
    a.set({seed:99});
    assert.notDeepEqual(a.process(new Float32Array(4800)), b.process(new Float32Array(4800)));
  });
  test('imported target RPM cannot exceed the engine redline', () => {
    assert.throws(()=>parsePatch('{"format":"ignition-patch","version":1,"patch":{"rpm":18000,"redline":2000}}'),/redline/i);
  });
  test('render trajectories differ and invalid render requests are rejected', () => {
    const steady = renderClip(base, {duration: 2, motion: 'steady'});
    for (const motion of ['rev', 'sweep', 'gears']) assert.notDeepEqual(steady, renderClip(base, {duration: 2, motion}));
    for (const duration of [0, -1, 31, NaN, Infinity]) assert.throws(() => renderClip(base, {duration}));
    assert.throws(() => renderClip(base, {duration: 2, motion: 'nonsense'}));
  });
  test('WAV stores exact 24-bit signed PCM frames with a valid header', () => {
    const wav = encodeWav(new Float32Array([0, 0.5, -0.5, 1, -1]), 48000);
    const view = new DataView(wav);
    const ascii = (a,b) => String.fromCharCode(...new Uint8Array(wav, a, b));
    assert.equal(ascii(0,4), 'RIFF'); assert.equal(ascii(8,4), 'WAVE');
    assert.equal(view.getUint16(22, true), 1); assert.equal(view.getUint32(24, true), 48000);
    assert.equal(view.getUint16(34, true), 24); assert.equal(view.getUint32(40, true), 15);
    assert.equal(wav.byteLength, 60); // odd data length includes RIFF pad byte
    assert.deepEqual([...new Uint8Array(wav, 44, 15)], [0,0,0,0,0,64,0,0,192,255,255,127,0,0,128]);
  });
  test('patch roundtrip preserves parameters and rejects damaged or hostile files', () => {
    assert.deepEqual(parsePatch(serializePatch(base)), validatePatch(base));
    for (const text of ['oops', '{}', '{"version":99,"patch":{}}', '{"format":"ignition-patch","version":1,"patch":{"rpm":"loud"}}', '{"format":"ignition-patch","version":1,"patch":{"surprise":1}}']) assert.throws(() => parsePatch(text));
    assert.throws(() => validatePatch({...base, rpm: Infinity}));
    assert.throws(() => validatePatch({...base, cylinders: 2.5}));
    assert.throws(() => validatePatch({...base, mode: 'explosion'}));
  });
}
