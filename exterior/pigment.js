// Pair the two triangles of a flat rectangular bracket face by their common
// diagonal. The resulting paint is fitted to that piece and ends at its edges.
export function paintBracketEnds(T,roof,parent,arcade=false){
  const plates=[],candidates=new Map();
  for(const mesh of roof.children.filter(o=>['r_cyan','r_blue'].includes(o.material?.name))){
    const g=mesh.geometry,p=g.attributes.position,ix=g.index,world=mesh.matrixWorld;
    const div=p.normalized?2**(p.array.BYTES_PER_ELEMENT*8-(p.array.constructor.name.startsWith('Int')?1:0))-1:1;
    const a=new T.Vector3(),b=new T.Vector3(),c=new T.Vector3();
    function point(i,out){return out.set(p.getX(i),p.getY(i),p.getZ(i)).multiplyScalar(1/div).applyMatrix4(world);}
    for(let j=0;j<ix.count;j+=3){
      point(ix.getX(j),a);point(ix.getX(j+1),b);point(ix.getX(j+2),c);
      const minY=Math.min(a.y,b.y,c.y),maxY=Math.max(a.y,b.y,c.y),h=maxY-minY;
      if(h<.13||h>.8||minY<2.8)continue;
      if(arcade&&(minY>6.5||(Math.abs(a.x)<25&&a.z<36&&a.z> -83)||(Math.abs(a.x)<10.5&&Math.abs(a.z-67.7)<6)))continue;
      const dx=Math.max(a.x,b.x,c.x)-Math.min(a.x,b.x,c.x),dz=Math.max(a.z,b.z,c.z)-Math.min(a.z,b.z,c.z);
      if(Math.min(dx,dz)>.002||Math.max(dx,dz)<(arcade?4:1.52)*h||Math.max(dx,dz)>(arcade?32:5.1)*h)continue;
      const side=dz>dx,axis=side?'z':'x',width=Math.max(dx,dz),minU=Math.min(a[axis],b[axis],c[axis]);
      // All vertices must be corners of this candidate rectangle.
      if(![a,b,c].every(v=>(Math.abs(v.y-minY)<.006||Math.abs(v.y-maxY)<.006)&&(Math.abs(v[axis]-minU)<.006||Math.abs(v[axis]-minU-width)<.006)))continue;
      const key=[side?1:0,Math.round(a[side?'x':'z']*250),Math.round(minY*250),Math.round(minU*250),Math.round(width*250),Math.round(h*250)].join(',');
      if(!candidates.has(key)){candidates.set(key,true);continue;}
      candidates.delete(key);
      const u=minU+width/2,level=minY+h/2,d=a[side?'x':'z'];
      plates.push({x:side?d:u,z:side?u:d,y:level,w:width,h,side});
    }
  }
  if(!plates.length)return;
  const image=document.createElement('canvas');image.width=192;image.height=80;
  const ctx=image.getContext('2d');ctx.lineCap='round';
  for(const [y,color,width] of [[9,'#a6ba99',2.6],[15,'#c4a168',1.6],[69,'#98ac90',2]]){
    ctx.strokeStyle=color;ctx.lineWidth=width;ctx.beginPath();ctx.moveTo(9,y);ctx.lineTo(183,y);ctx.stroke();
  }
  for(const mirror of [false,true]){
    ctx.save();if(mirror){ctx.translate(192,0);ctx.scale(-1,1);}
    ctx.fillStyle='#6e9c8a';ctx.strokeStyle='#cab786';ctx.lineWidth=1.8;
    const cloud=new Path2D('M10 57 Q26 55 39 37 Q52 23 65 31 Q77 39 68 48 Q59 54 53 46 Q65 50 64 40 Q63 34 56 36 Q50 37 43 46 Q29 60 10 57Z');
    ctx.fill(cloud);ctx.stroke(cloud);ctx.restore();
  }
  ctx.fillStyle='#c7a96f';ctx.fill(new Path2D('M86 41 L96 33 L106 41 L96 49 Z'));
  if(arcade){
    ctx.clearRect(0,0,192,80);ctx.strokeStyle='#c5ab74';ctx.lineWidth=1.8;
    for(const y of [12,63]){ctx.beginPath();ctx.moveTo(3,y);ctx.lineTo(189,y);ctx.stroke();}
    for(const side of [0,1]){
      ctx.save();if(side){ctx.translate(192,0);ctx.scale(-1,1);}
      ctx.strokeStyle='#88ab97';ctx.lineWidth=3;
      ctx.stroke(new Path2D('M5 54 H18 V24 H42 V46 H29 V34 H38 M23 54 H56 L69 39 H81 M8 19 H53 L70 33 H82'));
      ctx.restore();
    }
    ctx.fillStyle='#b89259';ctx.fill(new Path2D('M82 39 L96 23 L110 39 L96 55 Z'));
    ctx.fillStyle='#587b69';ctx.fill(new Path2D('M88 39 L96 31 L104 39 L96 47 Z'));
  }
  const tex=new T.CanvasTexture(image);tex.encoding=T.sRGBEncoding;tex.anisotropy=4;
  const mat=new T.MeshStandardMaterial({map:tex,alphaTest:.36,emissiveMap:tex,emissive:0xffffff,emissiveIntensity:.17,side:T.DoubleSide,roughness:.75,polygonOffset:true,polygonOffsetFactor:-1,polygonOffsetUnits:-2});
  const mesh=new T.InstancedMesh(new T.PlaneGeometry(1,1),mat,plates.length),pose=new T.Object3D();
  plates.forEach((p,i)=>{pose.position.set(p.x,p.y,p.z);pose.scale.set(p.w*.94,p.h*.89,1);pose.rotation.y=p.side?Math.PI/2:0;pose.updateMatrix();mesh.setMatrixAt(i,pose.matrix);});
  mesh.name=arcade?'回廊枋面 · 回纹与菱花':'斗栱端面 · 如意云头';parent.add(mesh);
}
export function limeFinish(T,source,projected){
  const mat=projected(source,'vec2(run,w.y)/2.65',`
    if(abs(w.x)<10.5 && abs(w.z+66.)<10.5 && abs(n.y)<.15 && w.y>2.){
      float band=wallHeight;
      float edge=step(.866,band)*step(band,.933)+step(.08,band)*step(band,.091);
      float border=step(.866,band)*step(band,.875)+step(.924,band)*step(band,.933);
      vec2 tile=vec2(fract(run*2.5),clamp((band-.875)/.049,0.,1.));
      float lozenge=1.-smoothstep(.065,.17,abs(abs(tile.x-.5)*1.7+abs(tile.y-.5)-.4));
      diffuseColor.rgb*=mix(vec3(1.),mix(vec3(.72,.67,.53),vec3(.39,.41,.32),max(border,lozenge)),edge);
    }`);
  mat.color.setRGB(.93,.86,.73);mat.emissiveIntensity=.12;
  const setup=mat.onBeforeCompile;
  mat.onBeforeCompile=sh=>{setup(sh);sh.vertexShader='attribute float bandHeight; varying float wallHeight;\n'+sh.vertexShader.replace('#include <begin_vertex>','#include <begin_vertex>\nwallHeight=bandHeight;');sh.fragmentShader='varying float wallHeight;\n'+sh.fragmentShader;};
  return mat;
}
export function prepareLime(T,mesh){
  // Plaster walls use split vertices at the edges of doors and windows. Keep a
  // common height for each storey, regardless of adjacent window openings.
  const g=mesh.geometry,p=g.attributes.position,ix=g.index,raw=new Float32Array(p.count);
  const div=p.normalized?2**(p.array.BYTES_PER_ELEMENT*8-(p.array.constructor.name.startsWith('Int')?1:0))-1:1;
  const positions=Array.from({length:p.count},(_,i)=>new T.Vector3(p.getX(i),p.getY(i),p.getZ(i)).multiplyScalar(1/div).applyMatrix4(mesh.matrixWorld));
  const spans=[];
  for(let i=0;i<ix.count;i+=3){
    const tris=[0,1,2].map(j=>positions[ix.getX(i+j)]);
    if(tris.some(p=>Math.abs(p.x)>10.5||Math.abs(p.z+66)>10.5))continue;
    const lo=Math.min(...tris.map(p=>p.y)),hi=Math.max(...tris.map(p=>p.y));
    if(hi-lo>2.4)spans.push([lo,hi]);
  }
  positions.forEach((p,i)=>{if(Math.abs(p.x)>10.5||Math.abs(p.z+66)>10.5)return;
    let widest=0;for(const [lo,hi] of spans)if(p.y>=lo-.02&&p.y<=hi+.02&&hi-lo>widest){widest=hi-lo;raw[i]=(p.y-lo)/widest;}
  });
  g.setAttribute('bandHeight',new T.BufferAttribute(raw,1));
}
