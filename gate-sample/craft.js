import {carvedDisc} from './carving.js?v=3';

function painting(T,w,h,draw){
  const c=document.createElement('canvas');c.width=w;c.height=h;draw(c.getContext('2d'),w,h);
  const t=new T.CanvasTexture(c);t.encoding=T.sRGBEncoding;t.anisotropy=8;return t;
}
function bracketFaces(T,parent,patches){
  // Join coplanar triangles by their shared edges before fitting each painted face.
  const positions=[],uv=[];
  for(const mesh of patches.filter(o=>/gate-sample-(jade|indigo)/.test(o.name))){
    const p=mesh.geometry.attributes.position,byVertex=new Map(),faces=[];
    const point=i=>new T.Vector3().fromBufferAttribute(p,i);
    for(let i=0;i<p.count;i+=3){
      const pts=[point(i),point(i+1),point(i+2)],n=new T.Vector3().crossVectors(pts[1].clone().sub(pts[0]),pts[2].clone().sub(pts[0])).normalize();
      if(Math.abs(n.z)<.994||pts[0].y<5.68||(pts[0].z-67.75)*n.z<0)continue;
      const id=faces.length;faces.push({pts,n,neighbours:[]});
      for(let j=0;j<3;j++){
        const key=v=>[v.x,v.y,v.z].map(a=>Math.round(a*1000)).join(',');
        const edge=[key(pts[j]),key(pts[(j+1)%3])].sort().join('/');
        if(byVertex.has(edge)){const other=byVertex.get(edge);faces[other].neighbours.push(id);faces[id].neighbours.push(other);}else byVertex.set(edge,id);
      }
    }
    const seen=new Set();
    for(let i=0;i<faces.length;i++){
      if(seen.has(i))continue;
      const cluster=[],stack=[i],bounds=new T.Box3();seen.add(i);
      while(stack.length){const face=faces[stack.pop()];cluster.push(face);face.pts.forEach(v=>bounds.expandByPoint(v));for(const k of face.neighbours)if(!seen.has(k)){stack.push(k);seen.add(k);}}
      const w=bounds.max.x-bounds.min.x,h=bounds.max.y-bounds.min.y;
      if(w<.16||w>2.7||h<.085||h>.64||w/h<1.25)continue;
      for(const face of cluster)for(const v of face.pts){positions.push(v.x,v.y,v.z+face.n.z*.001);uv.push((v.x-bounds.min.x)/w,(v.y-bounds.min.y)/h);}
    }
  }
  if(!positions.length)return;
  const t=painting(T,256,96,g=>{
    g.lineCap='round';g.lineJoin='round';
    for(const [y,c,lw] of [[13,'#9cbaa5',4],[20,'#d6ae70',2],[77,'#669b8d',5]]){
      g.strokeStyle=c;g.lineWidth=lw;g.beginPath();g.moveTo(18,y);g.lineTo(238,y);g.stroke();
    }
    for(const s of [1,-1]){g.save();if(s<0){g.translate(256,0);g.scale(-1,1);}
      g.fillStyle='#77a59c';g.strokeStyle='#c9b88a';g.lineWidth=2;
      const path=new Path2D('M19 64 Q35 53 43 41 Q56 21 75 32 Q89 42 79 53 Q70 60 60 50 Q70 53 73 46 Q76 37 66 37 Q57 36 50 47 Q37 66 19 64Z');g.fill(path);g.stroke(path);g.restore();}
    g.fillStyle='#d3af72';g.fill(new Path2D('M115 48 L128 38 L141 48 L128 58Z'));
  });
  const geom=new T.BufferGeometry();geom.setAttribute('position',new T.Float32BufferAttribute(positions,3));geom.setAttribute('uv',new T.Float32BufferAttribute(uv,2));geom.computeVertexNormals();
  const m=new T.MeshStandardMaterial({map:t,alphaTest:.32,roughness:.74,emissive:0xffffff,emissiveMap:t,emissiveIntensity:.19,polygonOffset:true,polygonOffsetFactor:-1,polygonOffsetUnits:-1});
  const paint=new T.Mesh(geom,m);paint.name='斗栱端面彩画';paint.userData.underRoof=true;parent.add(paint);
}
function screen(T){
  return painting(T,384,512,(g,w,h)=>{
    const grad=g.createRadialGradient(w*.47,h*.44,5,w*.5,h*.5,323);grad.addColorStop(0,'#ffe8aa');grad.addColorStop(.7,'#e1b874');grad.addColorStop(1,'#ab7748');g.fillStyle=grad;g.fillRect(0,0,w,h);
    for(let i=0;i<h;i+=3){g.fillStyle='rgba(82,46,24,.075)';g.fillRect(0,i,w,1);}
    for(let i=0;i<w;i+=3){g.fillStyle='rgba(253,241,187,.16)';g.fillRect(i,0,1,h);}
    g.strokeStyle='#a2804b';g.lineWidth=3;g.strokeRect(12,14,w-24,h-28);g.lineWidth=1.5;g.strokeRect(19,21,w-38,h-42);
    // Two inked bamboo stems on the silk; left and right leaves change direction at the nodes.
    function bamboo(x,y,angle,scale){
      g.save();g.translate(x,y);g.rotate(angle);g.scale(scale,scale);
      for(let j=0;j<5;j++){const yy=-j*69;g.fillStyle=j%2?'#756841':'#70633d';g.fillRect(-3,yy-66,6,65);g.fillRect(-5,yy-1,10,3);
        if(j>0){g.save();g.translate(0,yy);g.scale(j%2?-1:1,1);g.strokeStyle='#74613c';g.lineWidth=2;g.stroke(new Path2D('M0 0 Q37 -27 83 -22 M35 -18 Q51 -43 68 -54'));
          for(const [a,b,r] of [[29,-16,-.9],[42,-21,.35],[63,-25,.58],[55,-38,-.95],[73,-24,-.15]]){
            g.save();g.translate(a,b);g.rotate(r);g.fill(new Path2D('M0 0 Q21 -8 48 -1 Q21 10 0 0Z'));g.restore();
          }g.restore();}
      }g.restore();
    }
    bamboo(76,474,.20,1.1);bamboo(226,490,-.13,.77);
  });
}
export function craftGate(T,scene,patches,gate){
  const root=new T.Group();root.name='山门 · 细作';scene.add(root);
  function finish(hex,r=.64,metal=0){
    const m=new T.MeshStandardMaterial({color:new T.Color(hex).convertSRGBToLinear(),roughness:r,metalness:metal});m.emissive.copy(m.color);m.emissiveIntensity=.19;return m;
  }
  function add(g,m,x,y,z,parent=root){const obj=new T.Mesh(g,m);obj.position.set(x,y,z);obj.castShadow=true;obj.receiveShadow=true;parent.add(obj);return obj;}
  const brown=finish('#60392c'),bronze=finish('#a58151',.39,.62),dark=finish('#5c4937',.52,.5),cord=finish('#844035',.87);
  const stoneMat=new T.MeshStandardMaterial({vertexColors:true,roughness:.85,emissive:0x19130d,emissiveIntensity:.24});
  const metalMat=new T.MeshStandardMaterial({vertexColors:true,roughness:.42,metalness:.55,emissive:0x25180c,emissiveIntensity:.25});
  const drumGeo=carvedDisc(T,'stone',.386,.037,'#beb39a'),faceGeo=carvedDisc(T,'bronze',.212,.069,'#ad8857');
  // Give the drum rim clearance in front of the dado cap while keeping each pedestal on the landing.
  const drumZ=71.14;
  const drumBases=(gate?.children||[]).filter(o=>Math.abs(o.position.z-70.62)<.005&&Math.abs(Math.abs(o.position.x)-2.53)<.005);
  for(const sign of [-1,1])for(const side of [-1,1]){
    const obj=add(drumGeo,stoneMat,sign*2.53+side*.155,1.66,drumZ);obj.name="双鱼抱鼓石";obj.rotation.y=side*Math.PI/2;
  }
  const hidden=[];
  for(const hinge of gate?.children||[]){
    if(Math.abs(hinge.position.z-69.9)>.01||!hinge.children[0])continue;
    const leaf=hinge.children[0],mount=new T.Group();mount.applyMatrix4(leaf.matrixWorld);root.add(mount);
    for(const obj of leaf.children){if(Math.abs(obj.position.x-1.91)<.01&&['LatheGeometry','SphereGeometry','TorusGeometry'].includes(obj.geometry?.type))hidden.push(obj);}
    const x=1.91,y=1.45;
    add(faceGeo,metalMat,x,y,.059,mount).name="兽面铺首";
    const hoop=add(new T.TorusGeometry(.108,.021,10,42),bronze,x,y-.161,.122,mount);hoop.rotation.x=-.15;
    const keeper=add(new T.TorusGeometry(.039,.015,9,24),dark,x,y-.057,.113,mount);keeper.rotation.y=Math.PI/2;
  }

  bracketFaces(T,root,patches);
  const silk=screen(T);
  const shade=new T.MeshStandardMaterial({map:silk,emissiveMap:silk,emissive:0xffffff,emissiveIntensity:.45,roughness:.85,side:T.DoubleSide});
  const lamp=new T.Group(),hex=6;
  function turned(profile,material){
    return add(new T.LatheGeometry(profile.map(([r,y])=>new T.Vector2(r,y)),hex),material,0,0,0,lamp);
  }
  function rod(parent,a,b,r,material){
    const A=new T.Vector3(...a),B=new T.Vector3(...b),v=B.clone().sub(A),mid=A.clone().lerp(B,.5);
    const o=add(new T.CylinderGeometry(r,r,v.length(),8),material,mid.x,mid.y,mid.z,parent);
    o.quaternion.setFromUnitVectors(new T.Vector3(0,1,0),v.normalize());return o;
  }
  turned([[.01,.55],[.11,.51],[.24,.44],[.43,.39],[.46,.35],[.405,.315]],brown);
  turned([[.44,.367],[.451,.36],[.45,.35]],bronze);
  turned([[.31,-.35],[.365,-.33],[.40,-.268],[.397,-.247]],brown);
  turned([[.315,-.363],[.345,-.347],[.36,-.329]],bronze);
  const panel=new T.PlaneGeometry(.378,.546);
  for(let k=0;k<6;k++){
    const a=(k+.5)*Math.PI/3;
    const face=add(panel,shade,.341*Math.sin(a),.026,.341*Math.cos(a),lamp);face.rotation.y=a;
    const xx=.401*Math.sin(k*Math.PI/3),zz=.401*Math.cos(k*Math.PI/3);rod(lamp,[xx,-.26,zz],[xx,.33,zz],.025,brown);
    for(const yy of [-.25,.30]){
      const b=(k+1)*Math.PI/3;
      rod(lamp,[xx,yy,zz],[.401*Math.sin(b),yy,.401*Math.cos(b)],.021,brown);
    }
  }
  turned([[.20,-.35],[.15,-.42],[.055,-.44],[.037,-.51],[.005,-.52]],bronze);
  for(let k=0;k<12;k++){
    const a=k*Math.PI/6,x=.018*Math.cos(a),z=.018*Math.sin(a);
    rod(lamp,[x,-.50,z],[x*1.7,-.70-.016*Math.cos(a*3),z*1.7],.006,cord);
  }
  const ray=new T.Raycaster(),up=new T.Vector3(0,1,0);
  for(const s of [-1,1]){
    const x=4.79*s,z=71.17;ray.set(new T.Vector3(x,4.1,z),up);
    const hit=ray.intersectObjects(patches).find(h=>h.point.y>5.48);if(!hit)continue;
    const ceiling=hit.point.y,cy=Math.min(5.17,ceiling-.73),copy=lamp.clone(true);copy.name="六角竹纹宫灯";copy.userData.underRoof=true;copy.position.set(x,cy,z);root.add(copy);
    rod(root,[x,cy+.51,z],[x,ceiling+.01,z],.016,dark).userData.underRoof=true;
    add(new T.CylinderGeometry(.077,.055,.04,12),bronze,x,ceiling-.015,z).userData.underRoof=true;
  }
  return visible=>{root.visible=visible;for(const base of drumBases)base.position.z=visible?drumZ:70.62;for(const o of hidden)o.visible=!visible;};
}
