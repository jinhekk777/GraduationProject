import {installGate} from '../gate-sample/gate.js';
import {beamAtlas} from '../gate-sample/ornament.js';
import {paintBracketEnds,limeFinish,prepareLime} from './pigment.js';
import {installOrnaments} from './ornaments.js';

function worldGroup(T,parent,name){
  const group=new T.Group();group.name=name;group.matrixAutoUpdate=false;
  group.matrix.copy(parent.matrixWorld).invert();parent.add(group);group.updateMatrixWorld(true);
  return group;
}
const stoneUV='vec2(abs(n.y)>.72 ? w.x : run, abs(n.y)>.72 ? w.z : w.y) / vec2(2.72,1.44)';
const wallUV='vec2(run,w.y)/2.8';
const lacquerUV='vec2(run/1.12,w.y/2.6)';
const clayUV='vec2(dot(w.xz,crossCourse)/2.04, dot(w.xz,fall)/1.92)';
const ridgeUV='vec2(run,abs(n.y)>.75?w.z:w.y)/.95';
const woodUV='vec2(abs(n.x)>.65 ? w.z : w.x, abs(n.y)>.72 ? w.z : w.y)/vec2(2.8,.70)';

// The imported meshes are quantised and share UV atlases. Model-space metres
// retain one tile/stone scale across differently scaled and rotated GLB nodes.
function projected(src,uv,shade=''){
  const material=src.clone();
  material.vertexColors=false;
  material.onBeforeCompile=shader=>{
    shader.vertexShader='varying vec3 exteriorPoint;\n'+shader.vertexShader.replace('#include <begin_vertex>','#include <begin_vertex>\nexteriorPoint=(modelMatrix*vec4(transformed,1.)).xyz;');
    shader.fragmentShader='varying vec3 exteriorPoint;\n'+shader.fragmentShader.replace('#include <map_fragment>',`
      vec3 w=exteriorPoint;
      vec3 n=normalize(cross(dFdx(w),dFdy(w)));
      float run=abs(n.x)>abs(n.z)?w.z:w.x;
      vec2 fall=vec2(0.,1.);
      if (dot(n.xz,n.xz)>.006) {
        float a=floor(atan(n.z,n.x)*1.273239545+.5)*.785398163;
        fall=vec2(cos(a),sin(a));
        if(fall.x<-.001 || (abs(fall.x)<.001 && fall.y<0.))fall=-fall;
      }
      vec2 crossCourse=vec2(fall.y,-fall.x);
      vec2 facadeUv=${uv};
      #define vUv facadeUv
      #include <map_fragment>
      ${shade}
    `);
  };
  material.customProgramCacheKey=()=>uv+shade;
  return material;
}

function beamFinish(T,original,gateSurfaces,anisotropy){
  const art=beamAtlas(T,false,anisotropy),central=beamAtlas(T,true,anisotropy);
  const canvas=document.createElement('canvas');canvas.width=2048;canvas.height=1024;
  const c=canvas.getContext('2d');c.drawImage(central.image,0,0);c.drawImage(art.image,0,512);
  const map=new T.CanvasTexture(canvas);map.flipY=false;map.encoding=T.sRGBEncoding;map.anisotropy=anisotropy;
  // Preserve the GLB's scale/offset and the bay divisions of the original painting.
  map.offset.copy(original.map.offset);map.repeat.copy(original.map.repeat);
  const m=gateSurfaces.beams.clone();m.map=m.emissiveMap=map;
  m.onBeforeCompile=sh=>{
    sh.vertexShader='varying vec3 beamWorld;\n'+sh.vertexShader.replace('#include <begin_vertex>','#include <begin_vertex>\nbeamWorld=(modelMatrix*vec4(transformed,1.)).xyz;');
    sh.fragmentShader='varying vec3 beamWorld;\n'+sh.fragmentShader.replace('#include <map_fragment>',`
      bool formal=abs(beamWorld.x)<25. && beamWorld.z<30. && beamWorld.z> -17.;
      vec2 panelUV=vec2(fract(vUv.x),.5*fract(vUv.y)+(formal?0.:.5));
      #define vUv panelUV
      #include <map_fragment>
    `);
  };
  art.dispose();central.dispose();return m;
}
// Keep interior wall faces, exhibit partitions and furniture on their own finishes.
function onEnvelope(x,z){
  const halls=[[0,4,21,15],[-41,17.5,7,14.5],[41,17.5,7,14.5],[0,-29,12,6],[-32,-64,10,14],[32,-64,10,14],[0,-66,7,7]];
  return !halls.some(([cx,cz,rx,rz])=>Math.abs(x-cx)<rx-.75&&Math.abs(z-cz)<rz-.75);
}
function applyByTriangle(T,mesh,classify,choices){
  const g=mesh.geometry,p=g.attributes.position,ix=g.index,parts=choices.map(()=>[]);
  const active=Array(p.count),matrix=mesh.matrixWorld;
  const divisor=p.normalized ? 2**(8*p.array.BYTES_PER_ELEMENT-(p.array.constructor.name.startsWith('Int')?1:0))-1 : 1;
  const at=i=>active[i]||(active[i]=new T.Vector3(p.getX(i),p.getY(i),p.getZ(i)).multiplyScalar(1/divisor).applyMatrix4(matrix));
  for(let i=0;i<ix.count;i+=3){
    const a=ix.getX(i),b=ix.getX(i+1),c=ix.getX(i+2),v=at(a),w=at(b),t=at(c);
    const id=classify((v.x+w.x+t.x)/3,(v.y+w.y+t.y)/3,(v.z+w.z+t.z)/3,v,w,t);
    parts[id].push(a,b,c);
  }
  const geom=new T.BufferGeometry();for(const [key,attr] of Object.entries(g.attributes))geom.setAttribute(key,attr);
  const next=new ix.array.constructor(ix.count);let start=0;
  parts.forEach((indices,id)=>{if(indices.length){next.set(indices,start);geom.addGroup(start,indices.length,id);start+=indices.length;}});
  geom.setIndex(new T.BufferAttribute(next,1));mesh.geometry=geom;mesh.material=choices;
}
export async function finishMuseum(T,scene,renderer){
  scene.updateMatrixWorld(true);
  const base=scene.getObjectByName('museum_base'),roof=scene.getObjectByName('museum_roof');
  const gate=await installGate(T,{scene,renderer});gate.show('craft');
  const baseDetails=worldGroup(T,base,'院落外观 · 柱础与梁枋'),roofDetails=worldGroup(T,roof,'院落外观 · 檐口与风铎');
  const attached=[];scene.traverse(o=>{if(o.userData.underRoof)attached.push(o);});
  attached.forEach(o=>roofDetails.add(o));
  installOrnaments(T,base,roof,baseDetails,roofDetails);
  paintBracketEnds(T,roof,roofDetails);
  paintBracketEnds(T,base,baseDetails,true);
  const s=gate.surfaces;
  const roofMat=projected(s.roof,clayUV),ridge=projected(s.ridge,ridgeUV),stone=projected(s.base,stoneUV),cut=projected(s.cutstone,stoneUV);
  const wall=projected(s.wall,wallUV,'diffuseColor.rgb*=mix(vec3(.85,.80,.75),vec3(1.),smoothstep(.8,2.8,w.y));');
  const paint=projected(s.lacquer,lacquerUV);
  const beamMesh=base.children.find(o=>o.material?.name==='b_fang');
  const beams=beamFinish(T,beamMesh.material,s,Math.min(8,renderer.capabilities.getMaxAnisotropy()));
  const woodSource=base.children.find(o=>o.material?.name==='b_wood_light').material;
  const wood=projected(woodSource,woodUV);wood.color.setRGB(.71,.54,.37);wood.emissive.copy(wood.color);wood.emissiveMap=wood.map;wood.emissiveIntensity=.14;
  const mark=projected(s.verge,'vec2(w.y/.72,run/2.4)');mark.color.setRGB(.51,.60,.48);mark.emissive.copy(mark.color);
  const e1=new T.Vector3(),e2=new T.Vector3();
  for(const group of [base,roof])for(const obj of group.children){
    if(!obj.isMesh||!obj.geometry.index)continue;
    const old=obj.material,key=old.name;
    if(key==='r_tile')applyByTriangle(T,obj,(_x,_y,_z,a,b,c)=>Math.abs(e1.subVectors(b,a).cross(e2.subVectors(c,a)).normalize().y)<.18?1:0,[roofMat,ridge]);
    if(key==='b_plaster'){
      prepareLime(T,obj);
      applyByTriangle(T,obj,(x,y,z)=>onEnvelope(x,z)||(y>7&&Math.abs(x)<10.5&&Math.abs(z+66)<10.5)?1:0,[old,limeFinish(T,old,projected)]);
    }
    if(key==='b_fang')obj.material=beams;
    if(key==='r_cyan'||key==='r_blue'||key==='r_gold')obj.material=({r_cyan:s.jade,r_blue:s.indigo,r_gold:s.brass})[key];
    if(key==='b_redwall'||key==='b_wood'||key==='b_stone'||key==='b_carve'||key==='b_wood_light')
      applyByTriangle(T,obj,(x,y,z)=>!onEnvelope(x,z)?0:z< -89&&(key==='b_wood_light'||(key==='b_wood'&&y<4.))?2:1,[old,({b_redwall:wall,b_wood:paint,b_stone:stone,b_carve:cut,b_wood_light:wood})[key],wood]);
    if(key==='r_wood_light')obj.material=wood;
    // r_wood also forms interior ceilings and has caisson cut-outs. Keep those faces intact.
    if(key==='r_wood')applyByTriangle(T,obj,(x,y,z)=>onEnvelope(x,z)?1:0,[old,mark]);
  }
}
