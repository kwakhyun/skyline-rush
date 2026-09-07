const f=Math.fround,DT=f(1/120),clamp=(x,a,b)=>Math.min(b,Math.max(a,x));
const mix=x=>{x=(x^(x>>>16))>>>0;x=Math.imul(x,0x7feb352d)>>>0;x^=x>>>15;x=Math.imul(x,0x846ca68b)>>>0;return (x^(x>>>16))>>>0;};
export function theme(s){const u=Math.max(0,s)%300000;return u<60000?0:u<96000?1:u<126000?2:u<166000?3:u<206000?4:u<236000?5:u<240000?6:u<260000?7:8;}
export function tile(i,seed){
 const lanes=[0,0,0],t={index:i,safe:0,lanes,booster:false,pad:false,flightGap:false,jumpRow:false},local=((i+.5)*600)%300000,th=theme((i+.5)*600);
 if(th===5||th===6||th===7){t.pad=local>=236400&&local<240000;t.flightGap=th===7;if(t.flightGap)lanes.fill(3);return t;}
 if(th===4&&local>=179000&&i%24===12){t.jumpRow=true;lanes.fill(3);return t;}
 if(i<24)return t;t.booster=i>=52&&(i-52)%60===0;
 const level=clamp(1+Math.floor(i*600/30000),1,8),group=Math.floor(i/12),phase=i%12,random=mix((group^seed)>>>0);
 t.safe=level<3?0:[-1,0,1,0][(group+(seed&3))%4];
 if(phase===0||(level>=4&&phase===6)){
  if(level<3){const lane=group%3===2?0:(group%2?-1:1);t.safe=lane===0?1:0;lanes[lane+1]=level>=2&&group%2===0?2:1;}
  else{t.safe=phase===6?0:t.safe;for(let lane=-1;lane<=1;lane++)if(lane!==t.safe)lanes[lane+1]=(((random+lane+3+phase)>>>0)%3)+1;}
 }
 if(level>=5&&(phase===8||phase===9)){t.safe=0;lanes[group%2?0:2]=3;if(level>=7)lanes[group%2?2:0]=3;}
 return t;
}
const baseSpeed=d=>clamp(f(1400+f(f(Math.max(0,d))*f(.05))),1400,3800);
const interp=(cur,target,dt,speed)=>{const delta=f(target-cur),step=f(dt*speed);return Math.abs(delta)>step?f(cur+(delta>0?step:-step)):target;};
export function simulate(seed,ticks,events){
 const r={distance:0,lives:3,hits:0,recovery:0,hitSlow:0,speed:1400,lateral:0,lane:0,height:0,vertical:0,slide:0,jump:0,slideOnLand:false,coins:0,chain:0,bestChain:0,points:0,chainTime:0,feverCharge:0,fever:0,boost:0,power:0,fevers:0,boosters:0,lastBooster:-1,lastOrb:-1,protected:-1,launches:0,lastLaunch:-1,flightStart:-1,flightEnd:-1,crashed:false,failure:0,ticks:0};
 const flying=()=>r.flightStart>=0&&r.distance<r.flightEnd,sliding=()=>r.slide>0&&r.height<1;
 const multiplier=()=> (1+Math.min(3,Math.floor(r.chain/10)))*(r.fever>0?2:1);
 const charge=n=>{if(r.fever>0)return;r.feverCharge=Math.min(24,r.feverCharge+n);if(r.feverCharge>=24){r.feverCharge=0;r.fever=8;r.fevers++;}};
 let at=0,cache=new Map();
 const getTile=i=>{let t=cache.get(i);if(!t){t=tile(i,seed);cache.set(i,t);}return t;};
 for(let step=0;step<ticks;step++){
  if(r.crashed)throw new Error("after_crash");
  while(at<events.length&&events[at].tick===step){
   const a=events[at++].action;
   if((a==="l"||a==="r")&&!flying())r.lane=clamp(r.lane+(a==="l"?-1:1),-1,1);
   if(a==="j"&&!sliding()&&!flying())r.jump=f(.14);
   if(a==="s"&&!sliding()&&!flying()){if(r.height<1)r.slide=f(.82);else{r.vertical=Math.min(r.vertical,-950);r.slideOnLand=true;}}
  }
  r.ticks++;r.recovery=Math.max(0,f(r.recovery-DT));r.hitSlow=Math.max(0,f(r.hitSlow-DT));r.chainTime=Math.max(0,f(r.chainTime-DT));if(r.chainTime<=0){r.chain=0;r.feverCharge=0;}
  r.fever=Math.max(0,f(r.fever-DT));r.boost=Math.max(0,f(r.boost-DT));
  const target=r.boost>0?1:r.fever>0?f(.6):0;
  r.power=interp(r.power,target,DT,target>r.power?2:f(1.5));
  r.speed=f(baseSpeed(r.distance)*f(1+f(.25*r.power)));r.speed=f(r.speed*f(1-f(f(.35)*f(r.hitSlow/f(1.2)))));r.distance+=f(r.speed*DT);
  r.lateral=interp(r.lateral,r.lane*300,DT,1800);r.slide=Math.max(0,f(r.slide-DT));
  if(r.jump>0&&r.height<=0&&!sliding()){r.vertical=1120;r.jump=0;}
  r.jump=Math.max(0,f(r.jump-DT));
  if(r.flightStart>=0){
   if(r.distance>=r.flightEnd){r.flightStart=-1;r.flightEnd=-1;r.height=0;r.vertical=0;}
   else{const u=clamp((r.distance-r.flightStart)/(r.flightEnd-r.flightStart),0,1);r.height=f(6500*4*u*(1-u));r.vertical=0;r.slide=0;r.jump=0;}
  }
  if(!flying()&&(r.height>0||r.vertical>0)){
   r.height=f(r.height+f(f(r.vertical*DT)-f(f(f(.5*2800)*DT)*DT)));r.vertical=f(r.vertical-f(2800*DT));
   if(r.height<=0){r.height=0;r.vertical=0;if(r.slideOnLand){r.slide=f(.82);r.slideOnLand=false;}}
  }
  const current=Math.floor(r.distance/600);
  for(let i=Math.max(0,current-1);i<=current+1;i++){
   const t=getTile(i),along=Math.abs(r.distance-(i+.5)*600),cycle=Math.floor(r.distance/300000);
   if(t.pad&&along<275&&cycle>r.lastLaunch&&r.height<280){r.lastLaunch=cycle;r.launches++;r.flightStart=r.distance;r.flightEnd=cycle*300000+260100;r.lane=0;r.slide=0;r.vertical=0;r.jump=0;r.slideOnLand=false;}
   for(let lane=-1;lane<=1;lane++){
    const h=t.lanes[lane+1];if(!h||Math.abs(f(r.lateral-lane*300))>(h===3?118:142)||along>(h===3?275:112))continue;
    const hit=(h===1&&r.height<130)||(h===2&&!sliding())||(h===3&&r.height<24);
    if(hit&&!flying()){if(r.fever>0||r.recovery>0)r.protected=Math.max(r.protected,i);if(i>r.protected){r.hits++;r.lives--;if(r.lives<=0){r.lives=0;r.crashed=true;r.failure=h;break;}r.recovery=f(2.5);r.hitSlow=f(1.2);r.protected=i;r.chain=0;r.chainTime=0;r.feverCharge=0;r.boost=0;r.power=0;if(h===3){r.height=80;r.vertical=450;r.lane=0;}}}
   }
   if(r.crashed)break;
   if(t.booster&&i>r.lastBooster&&along<140&&Math.abs(r.lateral)<110&&r.height<160){r.lastBooster=i;r.boosters++;r.boost=4;r.chainTime=f(2.5);charge(6);}
   if(i%2===1&&i>r.lastOrb&&along<120&&(r.fever>0||(Math.abs(f(r.lateral-t.safe*300))<105&&r.height<140))){
    r.coins++;r.lastOrb=i;r.chain++;r.bestChain=Math.max(r.bestChain,r.chain);r.chainTime=f(2.5);charge(1);r.points+=20*multiplier();
   }
  }
  if(cache.size>32)for(const key of cache.keys())if(key<current-2)cache.delete(key);
 }
 if(at!==events.length)throw new Error("unused_inputs");
 return {lives:r.lives,hits:r.hits,score:Math.floor(r.distance/100)+r.points,distance:r.distance/100,seconds:ticks*DT,shards:r.coins,fevers:r.fevers,boosters:r.boosters,launches:r.launches,crashed:r.crashed,failure:r.failure,ticks:r.ticks};
}
export function validateReplay(body){
 if(body.rules_version!==6||!Number.isInteger(body.seed)||body.seed<1||body.seed>1e6||!Number.isInteger(body.ticks)||body.ticks<1||body.ticks>144000||!Array.isArray(body.events)||body.events.length>10000)throw new Error("invalid_replay");
 let previous=-1;
 for(const e of body.events){if(!e||!Number.isInteger(e.tick)||e.tick<previous||e.tick<0||e.tick>=body.ticks||!["l","r","j","s"].includes(e.action))throw new Error("invalid_input");previous=e.tick;}
 return simulate(body.seed,body.ticks,body.events);
}
