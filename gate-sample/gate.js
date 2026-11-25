import {craftGate} from './craft.js?v=3';
import {detailGate} from './detail.js?v=2';
// The study page and the museum install the same gate. Its controls belong only to the study page.
export async function installGate(T,app) {
  const loader = new T.TextureLoader();
  const cache = new Map();
  function texture(name) {
    if (!cache.has(name)) cache.set(name, new Promise((resolve,reject) => {
      loader.load(new URL('./maps/'+name+'.jpg', import.meta.url).href, t => {
        t.encoding = /color$|beams/.test(name) ? T.sRGBEncoding : T.LinearEncoding;
        t.wrapS = t.wrapT = T.RepeatWrapping;
        t.anisotropy = Math.min(8,app.renderer.capabilities.getMaxAnisotropy());
        resolve(t);
      }, undefined, reject);
    }));
    return cache.get(name);
  }
  const surfaces = {};
  await Promise.all(Object.entries({roof:.88, ridge:.58, wall:.48, lacquer:.39, base:.86, cutstone:.82}).map(async ([id,strength])=>{
    const [map,normalMap,roughnessMap] = await Promise.all(['color','normal','rough'].map(k=>texture(id+'-'+k)));
    surfaces[id] = new T.MeshStandardMaterial({map,normalMap,roughnessMap,roughness:1,normalScale:new T.Vector2(strength,strength),side:T.DoubleSide,envMapIntensity:id==='lacquer'?.62:.36});
  }));
  const paintMap = await texture('beams');
  surfaces.beams = new T.MeshStandardMaterial({map:paintMap,roughness:.72,side:T.DoubleSide});
  for(const [id,color] of Object.entries({indigo:0x325369,jade:0x3e7365,brass:0xbc9459,verge:0x674530})){
    surfaces[id]=new T.MeshStandardMaterial({color:new T.Color(color).convertSRGBToLinear(),roughness:id==='brass'?.54:.73,metalness:id==='brass'?.28:0,side:T.DoubleSide});
  }
  // The museum's warm silk treatment lifts shadows. Carry a portion of that
  // diffuse fill into the new materials so fine pigment remains legible under the eaves.
  for (const [id,mat] of Object.entries(surfaces)){
    mat.emissive.copy(mat.color);mat.emissiveMap=mat.map;
    mat.emissiveIntensity=({roof:.10,ridge:.12,base:.24,cutstone:.24,wall:.22,lacquer:.17,beams:.32})[id] ?? .18;
  }
  surfaces.verge.map=surfaces.lacquer.map;surfaces.verge.emissiveMap=surfaces.verge.map;
  surfaces.verge.color.setRGB(.88,.76,.64);surfaces.verge.emissive.copy(surfaces.verge.color);
  const changes=[], objects=[];
  function surface(name,isRoof,c) {
    if(Math.abs(c.x)>10.3 || c.z<62.18 || c.z>73.5)return;
    if(isRoof && name==='r_tile' && (c.y>9.35 || (c.y>8.4 && Math.abs(c.z-67.75)<.35)))return 'ridge';
    if(isRoof && c.y>5.45) return {r_tile:'roof',r_cyan:'jade',r_blue:'indigo',r_gold:'brass',r_wood:'verge'}[name];
    if(isRoof || Math.abs(c.x)>8.5 || c.z<63.27 || c.z>72.86)return;
    if(name==='b_stone' || name==='b_carve')return c.z>71.43||c.y<.63||name==='b_carve'?'cutstone':'base';
    if(c.z<64.9 || c.z>70.9 || Math.abs(c.x)>7.7)return;
    return {b_wood:'lacquer',b_fang:'beams',b_redwall:'wall',r_cyan:'jade',r_blue:'indigo'}[name];
  }
  function read(attr,idx,axis) {
    const raw=axis===0?attr.getX(idx):axis===1?attr.getY(idx):attr.getZ(idx);
    if(!attr.normalized)return raw;
    const signed=/^(Int16Array|Int8Array)$/.test(attr.array.constructor.name);
    const bits=8*attr.array.BYTES_PER_ELEMENT;
    return Math.max(signed?-1:0,raw/(2**(bits-(signed?1:0))-1));
  }
  function uvAt(id,v,face,centre) {
    const side=Math.abs(face.x)>Math.abs(face.z);
    const across=side?v.z:v.x;
    if(id==='roof'){
      // Fix grain direction by roof plane, keeping near-horizontal courses on each pitch.
      const hip=Math.abs(centre.x)>5 && Math.abs(face.x)>Math.abs(face.z)+.08;
      return [(hip?v.z:v.x)/2.04,(hip?Math.abs(v.x)-5:Math.abs(v.z-67.75))/1.92];
    }
    if(id==='ridge')return [across/.95,(Math.abs(face.y)>.8?v.z:v.y)/.95];
    if(id==='verge')return [v.y/.72,across/2.4];
    if(id==='wall'||id==='lacquer')return [across/(id==='wall'?2.8:1.12),v.y/(id==='wall'?2.8:2.6)];
    if(id==='base'||id==='cutstone')return Math.abs(face.y)>.7?[v.x/2.72,v.z/1.44]:[across/2.72,(v.y-.60)/1.44];
    return [v.x,v.y];
  }
  app.scene.updateMatrixWorld(true);
  for(const groupName of ['museum_base','museum_roof']){
    const group=app.scene.getObjectByName(groupName);
    group.traverse(o=>{
      if(!o.isMesh||!o.geometry.index|| !['b_wood','b_redwall','b_fang','b_stone','b_carve','r_tile','r_cyan','r_blue','r_gold','r_wood'].includes(o.material.name))return;
      const g=o.geometry,p=g.attributes.position,n=g.attributes.normal,uv=g.attributes.uv,ix=g.index,rest=[],parts=new Map();
      const normalMatrix=new T.Matrix3().getNormalMatrix(o.matrixWorld);
      const vertices=new Map();
      const at=i=>{
        if(!vertices.has(i))vertices.set(i,new T.Vector3(read(p,i,0),read(p,i,1),read(p,i,2)).applyMatrix4(o.matrixWorld));
        return vertices.get(i);
      };
      const c=new T.Vector3(),e=new T.Vector3(),face=new T.Vector3(),normal=new T.Vector3(),textureUV=new T.Vector2();
      for(let j=0;j<ix.count;j+=3){
        const a=ix.getX(j),b=ix.getX(j+1),d=ix.getX(j+2),va=at(a);
        // Each selected triangle must belong entirely to the gate, including its raised eaves.
        if(Math.abs(va.x)>10.9||va.z<61.75||va.z>73.65){rest.push(a,b,d);continue;}
        const vb=at(b),vd=at(d);c.copy(va).add(vb).add(vd).multiplyScalar(1/3);
        const id=surface(o.material.name,groupName==='museum_roof',c);
        if(!id || [vb,vd].some(v=>Math.abs(v.x)>10.9||v.z<61.75||v.z>73.65)){rest.push(a,b,d);continue;}
        if(!parts.has(id))parts.set(id,{position:[],normal:[],uv:[],color:[]});
        const part=parts.get(id);face.crossVectors(e.subVectors(vb,va),normal.subVectors(vd,va)).normalize();
        for(const i of [a,b,d]){
          const v=at(i);part.position.push(v.x,v.y,v.z);
          if(n)normal.set(read(n,i,0),read(n,i,1),read(n,i,2)).applyNormalMatrix(normalMatrix);else normal.copy(face);
          part.normal.push(normal.x,normal.y,normal.z);
          if(id==='beams'){
            textureUV.set(read(uv,i,0),read(uv,i,1));o.material.map.updateMatrix();
            textureUV.applyMatrix3(o.material.map.matrix);part.uv.push(textureUV.x,1-textureUV.y);
          }else part.uv.push(...uvAt(id,v,face,c));
          if(id==='wall'){const shade=.78+.22*Math.min(1,Math.max(0,(v.y-1.45)/1.25));part.color.push(shade,shade,shade);}
        }
      }
      if(!parts.size)return;
      const trimmed=new T.BufferGeometry();
      for(const [key,attribute] of Object.entries(g.attributes))trimmed.setAttribute(key,attribute);
      trimmed.setIndex(rest);
      changes.push({node:o,key:'geometry',original:g,sample:trimmed});
      for(const [id,part] of parts){
        const geom=new T.BufferGeometry();
        for(const [key,array] of Object.entries(part))if(array.length)geom.setAttribute(key,new T.Float32BufferAttribute(array,key==='uv'?2:3));
        if(id==='wall')surfaces.wall.vertexColors=true;
        const patch=new T.Mesh(geom,surfaces[id]);patch.name='gate-sample-'+id;patch.userData.underRoof=groupName==='museum_roof';patch.castShadow=o.castShadow;patch.receiveShadow=true;app.scene.add(patch);objects.push(patch);
      }
    });
  }
  // The leaves, jambs and recent courtyard treads were modelled separately from the compound.
  const gateJoinery=app.scene.children.find(g=>g.children.some(c=>Math.abs(c.position.z-70.52)<.015 && Math.abs(c.position.y-4.8)<.01));
  gateJoinery?.traverse(o=>{
    if(!o.isMesh)return;
    const m=o.material, source=m.map?.image?.src||'';
    if(source.includes('lacquer_base')){
      const paint=surfaces.lacquer.clone();paint.color.copy(m.color);
      changes.push({node:o,key:'material',original:m,sample:paint});
    } else if(!m.map && m.normalMap?.image?.src?.includes('plaster_normal')){
      changes.push({node:o,key:'material',original:m,sample:surfaces.cutstone});
    }
  });
  app.scene.getObjectByName('museum_base_gate_steps')?.traverse(o=>{if(o.isMesh)changes.push({node:o,key:'material',original:o.material,sample:surfaces.cutstone});});
  const applyDetail=detailGate(T,app.scene,objects,surfaces,gateJoinery,app.renderer.capabilities.getMaxAnisotropy());
  const showCraft=craftGate(T,app.scene,objects,gateJoinery);
  function show(value){
    const sample=value!=='original';
    for(const item of changes)item.node[item.key]=sample?item.sample:item.original;
    for(const patch of objects)patch.visible=sample;
    applyDetail(value==='deep'||value==='craft');
    showCraft(value==='craft');
  }
  return {show, surfaces, objects};
}
