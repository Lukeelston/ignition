import {EngineSynth} from './engine.js';
class EngineProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.engine=new EngineSynth(options.processorOptions.patch,sampleRate);
    this.frames=0;
    this.port.onmessage=event=>{
      try { this.engine.set(event.data.patch); }
      catch(error) { this.port.postMessage({error:error.message}); }
    };
  }
  process(_inputs,outputs) {
    const channel=outputs[0]?.[0];
    if (!channel) return true;
    this.engine.process(channel);
    this.frames+=channel.length;
    if(this.frames>=sampleRate/20) {
      this.frames=0;
      this.port.postMessage({rpm:this.engine.rpm});
    }
    return true;
  }
}
registerProcessor('ignition-engine',EngineProcessor);
