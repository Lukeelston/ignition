// Portable control model: seconds, km/h, RPM, normalized throttle/load.
// No DOM, timers, audio APIs or random state. Firmware can run this at 100 Hz.
const DRIVE_RATIOS=[0,3.2,2.1,1.55,1.23,1.02,.85];
const driveClamp=(x,lo,hi)=>Math.max(lo,Math.min(hi,x));
export class ToyDrive {
  constructor(config={}) {this.configure(config);this.stop();}
  configure({idle=750,redline=7000,response=.55}={}) {
    if(![idle,redline,response].every(Number.isFinite)||idle<350||redline<=idle||response<.1||response>1)throw new Error('Invalid drive limits.');
    this.idle=idle;this.redline=redline;this.response=response;
    if(this.rpm>redline)this.rpm=redline;
  }
  stop() {
    this.state='off';this.gear=0;this.speed=0;this.rpm=0;this.load=0;
    this.crank=0;this.shiftTime=0;this.elapsed=0;this.limiter=false;this.notice='';
  }
  start() {this.stop();this.state='cranking';this.rpm=350;}
  shift(direction) {
    if(this.state!=='running'||this.shiftTime>0||![-1,1].includes(direction))return false;
    const next=this.gear+direction;
    if(next<0||next>=DRIVE_RATIOS.length)return false;
    if(direction<0&&next>0&&this.speed*DRIVE_RATIOS[next]*48>this.redline*.98){this.notice='Over-rev · brake before downshifting';return false;}
    this.gear=next;this.shiftTime=.18;this.notice='';return true;
  }
  step(seconds,{throttle=0,brake=false,boost=false}={}) {
    if(!Number.isFinite(seconds)||seconds<0)throw new Error('Invalid drive time.');
    if(!Number.isFinite(throttle))throw new Error('Invalid throttle.');
    throttle=driveClamp(throttle,0,1);
    // Cap catch-up after a suspended browser; never jump the toy forward minutes.
    let remaining=Math.min(seconds,.25);
    while(remaining>1e-8){const dt=Math.min(.01,remaining);this.tick(dt,throttle,brake,boost);remaining-=dt;}
    return this.snapshot();
  }
  tick(dt,throttle,brake,boost) {
    if(this.state==='off')return;
    this.elapsed+=dt;
    if(this.state==='cranking') {
      this.crank+=dt;this.rpm=350+70*Math.sin(this.crank*35);this.load=.12;
      if(this.crank>=.65){this.state='running';this.rpm=this.idle;}
      return;
    }
    this.shiftTime=Math.max(0,this.shiftTime-dt);
    const ratio=DRIVE_RATIOS[this.gear], shifting=this.shiftTime>0;
    const driveThrottle=shifting||brake?0:throttle;
    if(ratio) {
      const wheelRPM=this.speed*ratio*48;
      const torque=Math.max(.15,1-.5*wheelRPM/this.redline);
      const acceleration=driveThrottle*(boost?1.5:1)*16*(ratio/3.2)*torque;
      const drag=.35+this.speed*.012+(1-driveThrottle)*2.5;
      this.speed=driveClamp(this.speed+(acceleration-drag-(brake?30:0))*dt,0,this.redline/(ratio*48));
    } else this.speed=Math.max(0,this.speed-(.5+this.speed*.012+(brake?30:0))*dt);
    const target=ratio?Math.max(this.idle,this.speed*ratio*48):this.idle+(this.redline-this.idle)*Math.pow(driveThrottle,.7);
    this.rpm+=(target-this.rpm)*(1-Math.exp(-dt/(shifting?.07:.10+(1-this.response)*.20)));
    this.rpm=driveClamp(this.rpm,this.idle,this.redline);
    this.limiter=this.rpm>this.redline*.975&&throttle>.8;
    const cut=this.limiter&&this.elapsed%.1<.035;
    this.load=shifting||cut?.05:driveClamp(.12+driveThrottle*(ratio?.88:.65),0,1);
  }
  snapshot() {
    return {state:this.state,gear:this.gear,speed:this.speed,rpm:this.rpm,load:this.load,shifting:this.shiftTime>0,limiter:this.limiter,notice:this.notice};
  }
}
