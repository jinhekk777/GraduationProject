import {beamAtlas,clayStamp,stoneCarving} from './ornament.js';

export function detailGate(T,scene,patches,surfaces,gate,anisotropy){
  const group=new T.Group(), switches=[],hidden=[];
  group.name='山门 · 深化饰面';scene.add(group);
  function finish(color,roughness,metalness=0){
    const m=new T.MeshStandardMaterial({color:new T.Color(color).convertSRGBToLinear(),roughness,metalness});
    m.emissive.copy(m.color);m.emissiveIntensity=.19;return m;
  }
  function mesh(geometry,material,parent=group){
    const m=new T.Mesh(geometry,material);m.castShadow=true;m.receiveShadow=true;parent.add(m);return m;
  }
  const unit=new T.BoxGeometry(1,1,1);
  function block(parent,mat,x,y,z,w,h,d){
    const m=mesh(unit,mat,parent);m.position.set(x,y,z);m.scale.set(w,h,d);return m;
  }
  const umber=finish('#744230',.54),edge=finish('#b78f53',.63,.28);
  const celadon=finish('#416660',.69),paper=finish('#716651',.94);
  const centre=surfaces.beams.clone(),wing=surfaces.beams.clone();
  centre.map=centre.emissiveMap=beamAtlas(T,true,anisotropy);
  wing.map=wing.emissiveMap=beamAtlas(T,false,anisotropy);
  const beam=patches.find(m=>m.name==='gate-sample-beams');
  const p=beam.geometry.attributes.position;
  let start=0,last=-1;
  for(let i=0;i<p.count;i+=3){
    const bay=Math.abs((p.getX(i)+p.getX(i+1)+p.getX(i+2))/3)<2.2?1:0;
    if(bay!==last){if(i>start)beam.geometry.addGroup(start,i-start,last);start=i;last=bay;}
  }
  beam.geometry.addGroup(start,p.count-start,last);
  switches.push([beam,beam.material,[wing,centre]]);
  for(const patch of patches.filter(m=>m.name==='gate-sample-verge')){
    const m=patch.material.clone();m.color.setRGB(.51,.60,.48);m.emissive.copy(m.color);
    switches.push([patch,patch.material,m]);
  }

  // Sample the existing curved eave at each tile course, so every cap meets the roof.
  const roof=patches.find(m=>m.name==='gate-sample-roof').geometry.attributes.position;
  // The compound has sparse plain end caps. Replace these faces only in the
  // detailed version; take installation heights from the continuous roof skin.
  const roofMesh=patches.find(m=>m.name==='gate-sample-roof'),skin=[],kept=[];
  for(let i=0;i<roof.count;i+=3){
    const xs=[0,1,2].map(k=>roof.getX(i+k)),zs=[0,1,2].map(k=>roof.getZ(i+k));
    const dz=Math.max(...zs)-Math.min(...zs),dx=Math.max(...xs)-Math.min(...xs);
    const legacy=dx<.20 && dz<.10 && zs.every(z=>Math.abs(z-67.75)>4.77);
    if(!legacy)kept.push(i,i+1,i+2);
    if(dx>.22&&dz>.20)skin.push(i);
  }
  const roofDetail=new T.BufferGeometry();
  for(const [name,attribute] of Object.entries(roofMesh.geometry.attributes))roofDetail.setAttribute(name,attribute);
  roofDetail.setIndex(kept);
  const roofOriginal=roofMesh.geometry;
  const ends=[];
  for(let tile=-36;tile<=36;tile++){
    const x=tile*2.04/8;let front=null,back=null;
    for(const i of skin){
      for(let k=0;k<3;k++){
        const a=i+k,b=i+(k+1)%3,xa=roof.getX(a),xb=roof.getX(b);
        if((xa>x)===(xb>x))continue;
        const t=(x-xa)/(xb-xa),y=roof.getY(a)+t*(roof.getY(b)-roof.getY(a)),z=roof.getZ(a)+t*(roof.getZ(b)-roof.getZ(a));
        if(!front||z>front.z+.001||(Math.abs(z-front.z)<.001&&y>front.y))front={x,y,z,sign:1};
        if(!back||z<back.z-.001||(Math.abs(z-back.z)<.001&&y>back.y))back={x,y,z,sign:-1};
      }
    }
    if(front&&front.z>71.5)ends.push(front);
    if(back&&back.z<64)ends.push(back);
  }
  const ceramic=finish('#677570',.64),stamp=clayStamp(T),badge=finish('#8a9080',.71);
  badge.bumpMap=stamp;badge.bumpScale=.017;
  badge.color.set('#798782').convertSRGBToLinear();badge.emissive.copy(badge.color);
  const ceramicWash=document.createElement('canvas');ceramicWash.width=ceramicWash.height=256;
  const cc=ceramicWash.getContext('2d');cc.fillStyle='#bec4bd';cc.fillRect(0,0,256,256);
  cc.globalAlpha=.27;cc.drawImage(stamp.image,0,0);badge.map=new T.CanvasTexture(ceramicWash);
  badge.map.encoding=T.sRGBEncoding;badge.emissiveMap=badge.map;
  const barrel=new T.InstancedMesh(new T.CylinderGeometry(.112,.105,.15,24),ceramic,ends.length);
  const face=new T.InstancedMesh(new T.CircleGeometry(.098,32),badge,ends.length);
  const rim=new T.InstancedMesh(new T.TorusGeometry(.104,.007,5,24),badge,ends.length);
  const pose=new T.Object3D();
  ends.forEach(({x,y,z,sign},i)=>{
    pose.position.set(x,y+.009,z+sign*.022);pose.rotation.set(sign*Math.PI/2,0,0);pose.updateMatrix();barrel.setMatrixAt(i,pose.matrix);
    pose.position.z=z+sign*.10;pose.rotation.set(0,sign===1?0:Math.PI,0);pose.updateMatrix();face.setMatrixAt(i,pose.matrix);
    pose.position.z+=sign*.002;pose.updateMatrix();rim.setMatrixAt(i,pose.matrix);
  });
  for(const obj of [barrel,face,rim]){obj.userData.underRoof=true;obj.castShadow=true;obj.receiveShadow=true;group.add(obj);}

  // A second, wall-scale layer follows the plinth and the sill rather than repeating across the bay.
  const wall=patches.find(o=>o.name==='gate-sample-wall');
  const pigment=wall.material.clone();
  pigment.onBeforeCompile=shader=>{
    shader.vertexShader='varying vec3 gatePosition;\n'+shader.vertexShader.replace('#include <begin_vertex>','#include <begin_vertex>\ngatePosition=position;');
    shader.fragmentShader='varying vec3 gatePosition;\n'+shader.fragmentShader.replace('#include <map_fragment>',`#include <map_fragment>
      vec3 p=gatePosition;
      float tide=.09*sin(p.x*5.9)+.04*sin(17.0*p.x);
      float foot=1.-smoothstep(1.68+tide,2.11+tide,p.y);
      float sill=(1.-smoothstep(1.2,1.55,abs(abs(p.x)-4.79)))*smoothstep(1.6,1.88,p.y)*(1.-smoothstep(1.93,2.15,p.y));
      diffuseColor.rgb*=mix(vec3(1.),vec3(.84,.78,.73),foot+.24*sill);
    `);
  };
  switches.push([wall,wall.material,pigment]);
  const silk=document.createElement('canvas');silk.width=silk.height=256;const sc=silk.getContext('2d');
  sc.fillStyle='#c7b896';sc.fillRect(0,0,256,256);
  for(let j=0;j<256;j+=3){sc.fillStyle='rgba(89,72,49,.10)';sc.fillRect(0,j,256,1);sc.fillStyle='rgba(255,245,215,.18)';sc.fillRect(j,0,1,256);}
  paper.map=new T.CanvasTexture(silk);paper.map.encoding=T.sRGBEncoding;paper.color.set('#a99c7b').convertSRGBToLinear();
  paper.emissive.copy(paper.color);paper.emissiveMap=paper.map;paper.emissiveIntensity=.22;
  // Fit woodwork inside the existing window frame; each line is a rectangular wood section.
  const lines=[];
  function stick(x1,y1,x2,y2,w,z,depth){
    const dx=x2-x1,dy=y2-y1;
    const rot=new T.Quaternion().setFromAxisAngle(new T.Vector3(0,0,1),Math.atan2(-dx,dy));
    lines.push(new T.Matrix4().compose(new T.Vector3((x1+x2)/2,(y1+y2)/2,z),rot,new T.Vector3(w,Math.hypot(dx,dy)+.012,depth)));
  }
  for(const sign of [1,-1])for(const side of [1,-1]){
    lines.length=0;const window=new T.Group();
    window.position.set(4.79*side,3.05,sign===1?70.365:65.135);if(sign===-1)window.rotation.y=Math.PI;group.add(window);
    block(window,paper,0,0,.018,2.27,1.47,.017);
    for(const y of [-.71,-.39,.39,.71])stick(-1.12,y,1.12,y,.033,.063,.043);
    for(const x of [-1.10,1.10])stick(x,-.72,x,.72,.031,.064,.044);
    for(let k=-3;k<=3;k++){
      const x=k*.31;
      for(const signY of [-1,1]){
        stick(x-.115,.39*signY,x-.115,.55*signY,.027,.064,.043);
        stick(x-.115,.55*signY,x+.115,.55*signY,.027,.064,.043);
        stick(x+.115,.55*signY,x+.115,.71*signY,.027,.064,.043);
      }
      // Interrupted uprights meet the four corners of each lozenge.
      stick(x,-.39,x,-.23,.033,.065,.048);stick(x,.23,x,.39,.033,.065,.048);
      for(const sx of [-1,1])for(const sy of [-1,1])stick(x,sy*.23,x+sx*.149,0,.034,.065,.047);
    }
    const lattice=new T.InstancedMesh(unit,celadon,lines.length);
    lines.forEach((m,i)=>lattice.setMatrixAt(i,m));lattice.castShadow=lattice.receiveShadow=true;window.add(lattice);
    for(const y of [-.755,.755])block(window,edge,0,y,.075,2.33,.016,.021);
  }
  gate?.traverse(n=>{if(n.isInstancedMesh&&n.count===15)hidden.push(n);});

  // Pressed bronze corner straps follow the door leaves' local axes and rotate with their faces.
  const plate=new T.Shape();
  plate.moveTo(.03,.03);plate.lineTo(.42,.03);plate.quadraticCurveTo(.43,.08,.33,.09);plate.lineTo(.17,.10);
  plate.quadraticCurveTo(.19,.19,.12,.22);plate.quadraticCurveTo(.09,.20,.09,.15);plate.lineTo(.075,.30);
  plate.quadraticCurveTo(.05,.34,.03,.32);plate.closePath();
  const strap=new T.ExtrudeGeometry(plate,{depth:.014,bevelEnabled:true,bevelSize:.008,bevelThickness:.004,bevelSegments:2,steps:1});
  for(const hinge of gate?.children||[]){
    if(Math.abs(hinge.position.z-69.9)>.01||!hinge.children[0])continue;
    const leaf=hinge.children[0],mount=new T.Group();mount.applyMatrix4(leaf.matrixWorld);group.add(mount);
    for(const right of [false,true])for(const top of [false,true]){
      const hardware=mesh(strap,edge,mount);hardware.position.set(right?2.11:0,top?2.90:0,.065);
      hardware.scale.set(right?-1:1,top?-1:1,1);
    }
  }
  // A stone sill moulding, a carved band in the dado, and layered shoes for the two door jambs.
  const sandstone=surfaces.cutstone.clone(),relief=surfaces.base.clone(),carving=stoneCarving(T);
  relief.bumpMap=carving;relief.bumpScale=.022;relief.normalMap=null;
  const stoneInset=finish('#b6ad96',.86);stoneInset.bumpMap=carving;stoneInset.bumpScale=.025;
  // Pigment in the incisions makes shallow relief visible in the soft evening fill.
  const wash=document.createElement('canvas');wash.width=1024;wash.height=128;
  const ctx=wash.getContext('2d');ctx.drawImage(carving.image,0,0);const pixels=ctx.getImageData(0,0,1024,128);
  for(let i=0;i<pixels.data.length;i+=4){const v=pixels.data[i];for(let k=0;k<3;k++)pixels.data[i+k]=215+(v-116)*.54;}
  ctx.putImageData(pixels,0,0);stoneInset.map=new T.CanvasTexture(wash);stoneInset.map.encoding=T.sRGBEncoding;
  for(const s of [-1,1]){
    const mid=s*4.85;
    for(const [y,h,z,d] of [[1.61,.095,70.41,.13],[1.52,.045,70.40,.11],[.76,.07,70.41,.13]]){
      block(group,sandstone,mid,y,z,4.85,h,d);
    }
    for(let k=-1;k<=1;k++){
      const body=block(group,relief,mid+k*1.52,1.13,70.402,1.45,.49,.062);
      const face=mesh(new T.PlaneGeometry(1.35,.37),stoneInset);face.position.set(body.position.x,1.13,70.439);
    }
    for(const [y,w,h,d] of [[.68,.39,.13,.49],[.77,.34,.07,.43],[.85,.28,.10,.38]])block(group,sandstone,s*2.21,y,70.02,w,h,d);
    for(const y of [.96,3.51])block(group,umber,s*2.21,y,70.018,.227,.08,.305);
  }
  return deep=>{
    group.visible=deep;roofMesh.geometry=deep?roofDetail:roofOriginal;
    for(const [node,oldMat,newMat] of switches)node.material=deep?newMat:oldMat;
    for(const n of hidden)n.visible=!deep;
  };
}
