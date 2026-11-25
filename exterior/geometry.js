// The compound is batched by material and quantised in the GLB. Reconnect its
// separated triangle vertices in model space before measuring individual pieces.
function components(T,mesh,accept) {
  const g=mesh.geometry, p=g.attributes.position, index=g.index;
  const divisor=p.normalized ? (2**(8*p.array.BYTES_PER_ELEMENT-(p.array.constructor.name.startsWith('Int')?1:0))-1) : 1;
  const points=[],parents=[],ids=new Map(),byIndex=new Int32Array(p.count).fill(-1);
  function id(i){
    if(byIndex[i]>=0)return byIndex[i];
    const raw=[p.getX(i),p.getY(i),p.getZ(i)],key=raw.join('/');
    if(!ids.has(key)){
      ids.set(key,points.length);parents.push(points.length);
      points.push(new T.Vector3(...raw).multiplyScalar(1/divisor).applyMatrix4(mesh.matrixWorld));
    }
    return byIndex[i]=ids.get(key);
  }
  function root(i){while(parents[i]!==i){parents[i]=parents[parents[i]];i=parents[i];}return i;}
  const used=new Set();
  for(let j=0;j<index.count;j+=3){
    const i=id(index.getX(j)),k=id(index.getX(j+1)),l=id(index.getX(j+2));
    if(!accept(points[i],points[k],points[l]))continue;
    parents[root(k)]=root(i);parents[root(l)]=root(i);
    used.add(i);used.add(k);used.add(l);
  }
  const groups=new Map();
  for(const i of used){const r=root(i);if(!groups.has(r))groups.set(r,[]);groups.get(r).push(points[i]);}
  return groups.values();
}
function inGate(v){return Math.abs(v.x)<10.35 && v.z>62.18 && v.z<73.5;}
export function columns(T,mesh){
  const list=[];
  for(const vs of components(T,mesh,(a,b,c)=>!inGate(a)&&Math.max(a.y,b.y,c.y)-Math.min(a.y,b.y,c.y)>1.65)){
    if(vs.length<14||vs.length>130)continue;
    const bounds=new T.Box3().setFromPoints(vs),lo=bounds.min.y,hi=bounds.max.y;
    const bottom=vs.filter(v=>v.y<lo+.016),top=vs.filter(v=>v.y>hi-.016);
    if(bottom.length<7||top.length<7)continue;
    const centre=bounds.getCenter(new T.Vector3());
    const radius=ring=>ring.reduce((s,v)=>s+Math.hypot(v.x-centre.x,v.z-centre.z),0)/ring.length;
    const foot=radius(bottom),head=radius(top);
    if(foot<.11||foot>.92||bottom.some(v=>Math.abs(Math.hypot(v.x-centre.x,v.z-centre.z)-foot)>foot*.09))continue;
    list.push({x:centre.x,z:centre.z,lo,hi,foot,head});
  }
  return list;
}
export function tileEnds(T,mesh){
  const list=[],bound=new T.Box3(),size=new T.Vector3(),centre=new T.Vector3();
  for(const vs of components(T,mesh,(a,b,c)=>!inGate(a)&&a.distanceToSquared(b)<.23&&a.distanceToSquared(c)<.23&&b.distanceToSquared(c)<.23)){
    if(vs.length<12||vs.length>80)continue;
    bound.setFromPoints(vs);bound.getSize(size);bound.getCenter(centre);
    const alongX=size.x<size.z,diameter=alongX?size.z:size.x,depth=alongX?size.x:size.z;
    if(diameter<.12||diameter>.43||Math.abs(size.y-diameter)>.018||depth/diameter>.86||depth/diameter<.22)continue;
    // Each barrel is a small horizontal cylinder. Put the stamp only on a real circular face.
    const key=alongX?'x':'z',edge=vs.filter(p=>Math.abs(p[key]-bound.max[key])<.007);
    if(edge.length<7)continue;
    const dx=alongX?1:0,dz=alongX?0:1;
    list.push({x:centre.x,y:centre.y,z:centre.z,dx,dz,depth,r:diameter/2});
  }
  return list;
}
