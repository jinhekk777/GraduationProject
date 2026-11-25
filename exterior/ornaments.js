import {clayStamp} from '../gate-sample/ornament.js';
import {columns,tileEnds} from './geometry.js';

export function installOrnaments(T,base,roof,baseDetails,roofDetails){
  const named=(group,name)=>group.children.find(o=>o.material?.name===name);
  const posts=columns(T,named(base,'b_wood')), ends=tileEnds(T,named(roof,'r_tile'));
  const batches=[],pose=new T.Object3D();
  function metal(hex,roughness,metalness=0){
    const color=new T.Color(hex).convertSRGBToLinear();
    return new T.MeshStandardMaterial({color,roughness,metalness,emissive:color,emissiveIntensity:.17});
  }
  function batch(label,geo,mat,parent){
    const items=[];batches.push({label,geo,mat,parent,items});
    return (x,y,z,sx,sy,sz,rotation=0)=>{
      pose.position.set(x,y,z);pose.scale.set(sx,sy,sz);pose.rotation.set(0,rotation,0);pose.updateMatrix();items.push(pose.matrix.clone());
    };
  }
  const lathe=outline=>new T.LatheGeometry(outline.map(([x,y])=>new T.Vector2(x,y)),24);
  const plinth=batch('柱础 · 束腰与覆盆',lathe([[1.32,0],[1.32,.12],[1.21,.17],[1.19,.24],[1.28,.31],[1.27,.41],[1.15,.5],[1.04,.52]]),metal('#a79b84',.82),baseDetails);
  const fillet=lathe([[.985,0],[1.03,.02],[1.044,.055],[1.03,.08],[.985,.10]]);
  const gold=batch('柱头 · 箍线',fillet,metal('#af8853',.50,.43),baseDetails);
  const band=batch('柱头 · 青绿箍带',lathe([[.99,0],[1.02,.015],[1.02,.33],[.99,.345]]),metal('#395c51',.67),baseDetails);
  const shoe=batch('桥柱 · 木护脚',lathe([[1.13,0],[1.13,.21],[1.06,.28],[1.035,.51]]),metal('#654536',.73),baseDetails);
  const bridge=posts.filter(p=>p.z< -90&&Math.abs(p.x)<5);
  const braceMat=metal('#794e33',.69), braces=[];
  for(const p of posts){
    const {x,z,lo,hi,foot,head}=p;
    if(z < -90)shoe(x,lo+.015,z,foot,foot,foot);
    else plinth(x,lo+.012,z,foot,foot,foot);
    const y=hi-Math.max(head*1.85,.34);
    band(x,y,z,head,head,head);
    gold(x,y,z,head,head,head);gold(x,y+head*.27,z,head,head,head);
  }
  const bar=(a,b,w,d)=>{
    const v=b.clone().sub(a);pose.position.copy(a).lerp(b,.5);pose.scale.set(d,v.length(),w);
    pose.quaternion.setFromUnitVectors(new T.Vector3(0,1,0),v.normalize());pose.updateMatrix();braces.push(pose.matrix.clone());
  };
  for(const p of bridge)for(const sign of [-1,1]){
    const near=bridge.some(q=>Math.abs(q.x-p.x)<.06&&(q.z-p.z)*sign>1&&(q.z-p.z)*sign<5.6&&Math.abs(q.hi-p.hi)<.12);
    if(!near)continue;
    const tip=(run,drop)=>new T.Vector3(p.x,p.hi-.07-drop,p.z+sign*(p.head*.84+run));
    bar(tip(0,.78),tip(.81,0),.088,.115);
    bar(tip(0,.48),tip(.48,0),.043,.084);
    bar(tip(.14,.34),tip(.28,.51),.037,.077);
    bar(tip(.28,.51),tip(.43,.35),.037,.077);
  }
  batches.push({label:'廊桥 · 柱间斜撑花格',geo:new T.BoxGeometry(1,1,1),mat:braceMat,parent:baseDetails,items:braces});
  const stamp=clayStamp(T),wash=document.createElement('canvas');wash.width=wash.height=256;
  const ctx=wash.getContext('2d');ctx.fillStyle='#bdc1ad';ctx.fillRect(0,0,256,256);ctx.globalAlpha=.41;ctx.drawImage(stamp.image,0,0);
  const clay=metal('#788074',.75);clay.map=new T.CanvasTexture(wash);clay.map.encoding=T.sRGBEncoding;clay.bumpMap=stamp;clay.bumpScale=.012;clay.emissiveMap=clay.map;
  // A single capped profile carries both the moulded lip and the lotus stamp.
  const verts=[0,0,0],uv=[.5,.5],faces=[];
  const sides=16;
  for(const [ring,[radius,z]] of [[.88,0],[.95,.055],[1,.008]].entries()){
    for(let k=0;k<sides;k++){
      const a=2*Math.PI*k/sides,x=radius*Math.cos(a),y=radius*Math.sin(a);
      verts.push(x,y,z);uv.push(.5+x*.5,.5+y*.5);
      const i=1+ring*sides+k,j=1+ring*sides+(k+1)%sides;
      if(!ring)faces.push(0,i,j);else faces.push(i-sides,i,j,j,j-sides,i-sides);
    }
  }
  const disk=new T.BufferGeometry();disk.setAttribute('position',new T.Float32BufferAttribute(verts,3));
  disk.setAttribute('uv',new T.Float32BufferAttribute(uv,2));disk.setIndex(faces);disk.computeVertexNormals();
  const cap=batch('莲瓣瓦当',disk,clay,roofDetails);
  // Interpolate along a single curved eave only. Original barrel ends anchor
  // both the position and scale; different tiers and adjoining roofs stay separate.
  const clayBody=metal('#62695f',.78);
  const barrel=batch('檐口 · 筒瓦',new T.CylinderGeometry(1,1,1,12,1,true).rotateX(Math.PI/2),clayBody,roofDetails);
  const infill=[];
  for(const a of ends){
    const across=a.dx?'z':'x',along=a.dx?'x':'z';let neighbour,span=2.3;
    for(const b of ends){
      const delta=b[across]-a[across];
      if(b.dx!==a.dx||delta<=.05||delta>=span||Math.abs(b[along]-a[along])>.035||
         Math.abs(b.r-a.r)>.011||Math.abs(b.y-a.y)>.36*delta)continue;
      neighbour=b;span=delta;
    }
    if(!neighbour)continue;
    const steps=Math.round(span/Math.max(.255,2.18*a.r));
    for(let j=1;j<steps;j++){
      const t=j/steps,b=neighbour,e={...a,x:a.x+(b.x-a.x)*t,y:a.y+(b.y-a.y)*t,z:a.z+(b.z-a.z)*t};
      infill.push(e);barrel(e.x,e.y,e.z,e.r,e.r,e.depth,e.dx?Math.PI*.5:0);
    }
  }
  for(const e of [...ends,...infill])for(const face of [-1,1]){
    const rot=e.dx?face*Math.PI/2:face>0?0:Math.PI,offset=face*(e.depth*.5+.004);
    cap(e.x+e.dx*offset,e.y,e.z+e.dz*offset,e.r*.94,e.r*.94,e.r*.94,rot);

  }
  // Pick actual tiles nearest the four swept-up corners of each tower eave.
  const tower=ends.filter(e=>Math.abs(e.x)<12&&Math.abs(e.z+66)<12&&e.y>7).sort((a,b)=>a.y-b.y),tiers=[];
  for(const e of tower){if(!tiers.length||e.y-tiers[tiers.length-1].at(-1).y>1.1)tiers.push([]);tiers.at(-1).push(e);}
  const bronze=metal('#987849',.48,.52),dark=metal('#67503e',.62,.36);
  const bell=batch('木塔 · 角檐风铎',lathe([[.20,.01],[.29,.045],[.29,.075],[.23,.12],[.205,.26],[.145,.33],[.055,.37],[.02,.39]]),bronze,roofDetails);
  const chain=batch('风铎吊杆',new T.CylinderGeometry(.014,.014,1,8),dark,roofDetails);
  const tongue=batch('风铎风板',new T.OctahedronGeometry(1),bronze,roofDetails);
  for(const tier of tiers)for(const sx of [-1,1])for(const sz of [-1,1]){
    const e=tier.reduce((a,b)=>sx*b.x+sz*b.z>sx*a.x+sz*a.z?b:a);
    const scale=.95-Math.min(.32,e.y*.006),bottom=e.y-.75*scale;
    chain(e.x,e.y-.19*scale,e.z,1,.4*scale,1);bell(e.x,bottom,e.z,scale,scale,scale);
    chain(e.x,bottom-.075*scale,e.z,.7,.17*scale,.7);tongue(e.x,bottom-.21*scale,e.z,.073*scale,.13*scale,.021*scale,.5);
  }
  for(const {label,geo,mat,parent,items} of batches){
    if(!items.length)continue;
    // r128 normally draws every instance even inside an unrelated hall. Give
    // each courtyard block a conservative bound while sharing its GPU buffers.
    const blocks=new Map();
    for(const matrix of items){
      const p=matrix.elements,key=Math.floor((p[12]+13)/26)+','+Math.floor((p[14]+13)/26);
      if(!blocks.has(key))blocks.set(key,[]);blocks.get(key).push(matrix);
    }
    geo.computeBoundingSphere();
    for(const block of blocks.values()){
      const geometry=new T.BufferGeometry();geometry.setIndex(geo.index);
      for(const [name,attribute] of Object.entries(geo.attributes))geometry.setAttribute(name,attribute);
      const bounds=new T.Box3(),sphere=new T.Sphere(),extent=new T.Box3();
      const mesh=new T.InstancedMesh(geometry,mat,block.length);mesh.name=label;
      block.forEach((matrix,i)=>{mesh.setMatrixAt(i,matrix);sphere.copy(geo.boundingSphere).applyMatrix4(matrix);sphere.getBoundingBox(extent);bounds.union(extent);});
      geometry.boundingSphere=bounds.getBoundingSphere(new T.Sphere());mesh.frustumCulled=true;
      mesh.castShadow=mesh.receiveShadow=true;parent.add(mesh);
    }
  }
}
