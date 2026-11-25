import {installGate} from './gate.js';
const frame = document.querySelector('#museum');
const nav = document.querySelector('nav');
const status = document.querySelector('#status');
const shots = {
  lantern: [[6.4,4.75,74.8],[4.60,5.22,70.94],39],
  stone: [[4.57,2.45,71.72],[2.57,1.68,71.14],34],
  knocker: [[3.47,2.57,73.24],[1.49,2.26,71.72],31],
  front: [[-5.3, 3.5, 91], [0, 5.6, 67.8], 39],
  roof: [[8.4, 12.8, 81.3], [.5, 8.3, 67.7], 41],
  beam: [[2.8, 4.85, 76.6], [1, 5.75, 70], 47],
  eave: [[3.5,7.9,75.8],[2,7,72.42],44],
  window: [[6.65,2.9,74.4],[4.72,2.52,70.32],45],
  wall: [[5.6, 2.25, 76.3], [3.1, 2.38, 70.1], 51]
};

async function start() {
  const win = frame.contentWindow, doc = frame.contentDocument;
  const T = win.THREE, app = win.__t;
  const style = doc.createElement('style');
  style.textContent = '#app > :not(canvas){visibility:hidden!important}';
  doc.head.append(style);
  const {show}=await installGate(T,app);
  const version=document.querySelector('#version');
  function aim(key){
    const [pos,target,fov]=shots[key];
    const more=document.querySelector('#more');more.value=[...more.options].some(o=>o.value===key)?key:'';
    const aspect=app.camera.aspect;
    app.camera.fov=aspect<1.25?2*Math.atan(Math.tan(fov*Math.PI/360)*1.25/aspect)*180/Math.PI:fov;app.camera.updateProjectionMatrix();app.aim(pos,target,420);
    for(const btn of nav.querySelectorAll('[data-shot]'))btn.setAttribute('aria-pressed',String(btn.dataset.shot===key));
  }
  nav.addEventListener('click',e=>{
    if(e.target.dataset.shot)aim(e.target.dataset.shot);
  });
  document.querySelector('#more').addEventListener('change',e=>{if(e.target.value)aim(e.target.value);});
  version.addEventListener('change',()=>show(version.value));
  app.setMode('ext');app.controls.minDistance=2;app.controls.maxDistance=80;app.controls.maxPolarAngle=2.15;show(version.value);
  status.hidden=true;nav.hidden=false;aim('front');
}
async function ready(){
  try{const doc=frame.contentDocument;if(!doc?.querySelector('#loading.done')||!frame.contentWindow.__t){setTimeout(ready,200);return;}await start();}
  catch(err){status.textContent='样板载入失败，请刷新重试';console.error(err);}
}
ready();
