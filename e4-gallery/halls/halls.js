// Exhibition furniture is placed in the compound's world coordinates (metres).
function createStudyHalls(THREE) {
  const half = Math.PI / 2, directory = 'e4-gallery/halls/';
  const stations = {
    ta: {outdoor:true, at:[7.7,0,-51.6], yaw:.27, span:4.12,
      overview:[[12.8,3.15,-36.7],[.1,16.0,-65.8],67],
      plates:[['towereave','纵横两木怎样入斗']],
      models:[['dou','柱头十字斗口',-.87,1.12,.10]],
      angles:{'首层檐下':[[10.6,4.6,-51],[1,7.5,-60.3],48]}},
    qq: {outdoor:true, at:[9.0,0,-92.5], yaw:.70, span:6.55,
      overview:[[20.5,5.4,-92.9],[.5,3.35,-103.2],66],
      plates:[['bridgeweave','直木如何编成拱']],
      models:[['arch','三节苗与五节苗',-1.77,1.0,.015],['joint','平苗接牛头',.39,1.05,-.10]],
      angles:{'桥腹':[[15.5,1.75,-104.5],[0,2,-106],61]}},
    e1: {overview:[[-35.65,2.58,28.7],[-43.8,2.33,24.25],57], plates:[
      ['wedge','透榫怎样锁住横材',[-46.08,3.56,24.6],half,7.75,
        ['横材的榫头穿过立柱，直到榫肩贴住柱面。','从出榫一侧打入木楔，使榫端撑开，抵紧卯壁。']]], samples:['透榫加楔','十字搭接','格肩榫']},
    e2: {overview:[[-35.6,2.57,14.45],[-43.35,2.1,10.25],60], plates:[
      ['dove','燕尾锁住箱角',[-46.08,3.35,10.8],half,6.38,
        ['看箱角两块板的交接处，榫头外宽内窄，形似燕尾。','沿槽口滑入才能合拢；向外拉时，斜肩抵住相邻的木料。']]], samples:['燕尾榫','银锭榫','龙凤榫']},
    e3: {overview:[[.05,2.58,-23.30],[0,2.22,-33.75],66], plates:[
      ['chair','一圈弧木如何接长',[-5.7,3.26,-33.9],0,5.95],
      ['desk','一张案的面、牙与足',[5.7,3.26,-33.9],0,5.95]]},
    e5: {overview:[[27.65,3.08,-62.8],[32.25,2.47,-73.35],57], plates:[
      ['loop','八木相承',[32,3.27,-76.35],0,8.4],
      ['float','把斗栱悬在空中',[36.65,2.69,-52.05],Math.PI,4.4]]},
    xu: {overview:[[3.95,6.09,15.25],[-.5,5.22,12.2],59],plates:[
      ['route','顺着中轴读院落',[-3.85,5.10,11.75],.60,2.85]]},
    zt: {overview:[[8.5,7.9,-2.85],[-.8,8.2,2.9],59],plates:[
      ['bracket','从栌斗看到双下昂',[4.9,5.67,3.9],1.95,3.65]]},
    ti: {overview:[[3.8,3.38,-60.8],[-1,3.96,-68],56],plates:[
      ['pagoda','檐与檐之间',[2.65,3.48,-72],0,3.15]]},
    gf: {overview:[[43,2.68,15.5],[39.05,2.05,6.8],65],plates:[
      ['box','方盒怎样合拢',[40.55,2.53,5.27],0,5.38]]},
    wt: {overview:[[37.25,2.69,21.4],[41.15,2.09,30.5],62],plates:[
      ['index','几片木板，一件器物',[41.1,3.11,31.44],Math.PI,6.25]]}
  };
  const panels = {}, labels = {};
  const walnut=0x583f32, green=0x344c43, brass=0xac8351, stone=0x928678;
  const loader = new THREE.TextureLoader(), unit = new THREE.BoxGeometry(1,1,1);
  const surfaces = new Map();
  function material(color) {
    if(!surfaces.has(color)) {
      const m = new THREE.MeshStandardMaterial({color:new THREE.Color(color).convertSRGBToLinear(),roughness:color===brass?.50:.86,metalness:color===brass?.32:0});
      m.emissive.copy(m.color); m.emissiveIntensity=.25; surfaces.set(color,m);
    }
    return surfaces.get(color);
  }
  function box(parent,x,y,z,w,h,d,color) {
    const m=new THREE.Mesh(unit,material(color)); m.scale.set(w,h,d);m.position.set(x,y,z);m.receiveShadow=true;parent.add(m);return m;
  }
  function stand(world,pos,yaw) {const g=new THREE.Group();g.position.set(...pos);g.rotation.y=yaw;world.add(g);if(world.isScene)g.userData.studyFixture=true;return g;}
  const readingPages = new Map();
  const descriptions={
    route:['先在烫样上找山门、主殿和木塔，沿中轴认清两进院落。','进入西配殿看榫样，再经后殿前往北院。'],
    bracket:['先找柱顶的栌斗，再沿横栱和华栱向上看。','两道斜昂向檐外伸出，可以对照中央的柱头模型辨认。'],
    chair:['沿椅圈寻找弧形木料的分段接缝，再看靠背板的位置。','座框、腿足和横枨共同支承座面。'],
    desk:['从案面往下找牙条，再看牙条怎样进入腿足上端的槽口。','夹头榫将案面、牙条和腿足接在一起。'],
    pagoda:['从塔身外侧数明层，再在剖面模型中寻找夹在中间的暗层。','对照内外两圈柱子和层间的斜撑。'],
    loop:['循着木料首尾相搭的方向，找出每根木料托住和压住的位置。','展台上的整环、相邻两木和单根教具可以互相对照。'],
    float:['循悬线往下看，各层斗与栱在展开时显出相互承托的位置。','对照展板上的分层图，再观察装置合拢。'],
    box:['沿盒面、侧板和底板的接缝，辨认各片木板的装配方向。','在工作台上对照燕尾榫和拼板榫样。'],
    index:['从完整器物回看板件，再对照索引桌上的榫样。','东墙按接合方式陈列五种节点的分解图。'],
    towereave:['先在木样上找斗口，再辨认两个方向的木件。','沿纵横交接的位置抬头对照木塔檐下。'],
    bridgeweave:['顺着三节苗和五节苗寻找两组拱骨的交会处。','横向的牛头联系两侧拱架，台上木样可拆开对照。']
  };
  function surface(parent,w,h,key,anisotropy,title,lines=descriptions[key]) {
    const canvas=exhibitTextPage(title,lines);
    const preview=new THREE.CanvasTexture(canvas);preview.encoding=THREE.sRGBEncoding;
    const basic=new THREE.MeshBasicMaterial({map:preview,toneMapped:false});
    readingPages.set(key,canvas);
    const map=loader.load(directory+key+'.jpg',loaded=>{
      basic.map=loaded;basic.needsUpdate=true;preview.dispose();
      readingPages.set(key,loaded.image);showExhibitPage(key,title,loaded.image,true);
    },undefined,()=>map.dispose());
    map.encoding=THREE.sRGBEncoding;map.anisotropy=Math.min(8,anisotropy);
    const m=new THREE.Mesh(new THREE.PlaneGeometry(w,h),basic);
    m.name='study-'+key;m.userData.plate=key;m.position.z=.035;parent.add(m);return m;
  }
  function registerPage(id,key,title,mesh) {
    mesh.userData.plate=key;(panels[id]??=[]).push(mesh);
    labels[key]=title;readingPages.set(key,mesh.material.map.image);
  }
  function pieceLabel(g,text,x,y,z,w) {
    const c=document.createElement('canvas');c.width=650;c.height=125;const ctx=c.getContext('2d');
    ctx.fillStyle='#354942';ctx.fillRect(0,0,650,125);ctx.fillStyle='#f3e4c6';ctx.font='46px "Songti SC",serif';ctx.textAlign='center';ctx.textBaseline='middle';ctx.fillText(text,325,65);
    const map=new THREE.CanvasTexture(c);map.encoding=THREE.sRGBEncoding;
    const o=new THREE.Mesh(new THREE.PlaneGeometry(w,w*125/650),new THREE.MeshBasicMaterial({map,toneMapped:false}));o.position.set(x,y,z);g.add(o);
  }
  function rail(parent,width,height,depth) {
    box(parent,0,height-.12,0,width-.16,.16,depth-.10,walnut);
    box(parent,0,height-.032,0,width,.064,depth,green);
    box(parent,0,height-.078,depth/2-.025,width-.035,.014,.023,brass);
    for(const sign of [-1,1]) {
      box(parent,sign*width*.35,height*.42,-.1,.105,height*.84,.32,walnut);
      box(parent,sign*width*.35,.035,-.06,.38,.07,depth*.85,brass);
    }
  }
  const oak=0x875e42, blackwood=0x372b26;
  function displayBay(g,key,w,h,groundLocal) {
    const broad=['loop','wedge','dove','chair','desk','box','index','float'].includes(key);
    if(!broad)return;
    const width=w+(key==='loop'?5.3:key==='chair'||key==='desk'?1.75:1.5);
    const top=h/2+.35,full=top-groundLocal;
    box(g,0,(top+groundLocal)/2,-.11,width,full,.08,blackwood);
    box(g,0,(top+groundLocal)/2,-.11,width-.14,full-.12,.022,green);
    const sides=(width-w)/2-.15,n=Math.max(3,Math.round(sides/.115));
    for(const sign of [-1,1])for(let i=0;i<n;i++){
      const px=sign*(w/2+.24+(i+.26)*sides/n);
      box(g,px,(top+groundLocal)/2,-.065,.044,full-.24,.067,oak);
    }
    box(g,0,top-.038,-.065,width+.07,.07,.23,brass);
    box(g,0,groundLocal+.085,-.035,width+.12,.17,.29,blackwood);
  }
  function sampleOnTable(parent,makeSample,name,amount,x,z,top,size,twist) {
    const model=new THREE.Group();
    for(const [o,d] of makeSample(name)){
      const p=new THREE.Group();p.add(o);p.position.set(...d).multiplyScalar(amount);model.add(p);
    }
    model.rotation.y=twist;model.scale.setScalar(size);
    const bounds=new THREE.Box3().setFromObject(model),c=bounds.getCenter(new THREE.Vector3());
    model.position.set(x-c.x,top-bounds.min.y,z-c.z);parent.add(model);
  }
  function readingTable(scene,makeSample) {
    // The original index table in the east wing already has a 1.48 m high top.
    const g=stand(scene,[41,1.49,24.95],Math.PI);
    box(g,0,.012,0,2.86,.024,1.31,blackwood);box(g,0,.035,0,2.80,.023,1.25,brass);
    box(g,0,.054,0,2.75,.025,1.20,green);
    for(const [i,name,k] of [[0,'十字搭接',.66],[1,'银锭榫',.53],[2,'楔钉榫',.65]]) {
      const x=(i-1)*.91;box(g,x,.083,0,.84,.034,.83,stone);
      sampleOnTable(g,makeSample,name,k,x,-.04,.10,.74,.16);pieceLabel(g,name,x,.10,.53,.63);
    }
  }
  function workbenches(scene,makeSample) {
    const lessons=[['十字搭接',37.5,6.6],['透榫加楔',44.0,6.6],['燕尾榫',37.5,13.05],['银锭榫',44.0,13.05]];
    for(const [name,x,z] of lessons) {
      const g=stand(scene,[x,1.506,z],0);
      box(g,0,.012,0,1.64,.024,1.0,brass);box(g,0,.031,0,1.59,.025,.95,green);
      sampleOnTable(g,makeSample,name,.68,0,0,.047,1.13,-.27);
    }
  }
  function artBench(scene,anisotropy) {
    const g=stand(scene,[32,.61,-75.48],0);rail(g,7.85,.90,1.25);
    const stages=[['完整互承环',8],['相邻两木',2],['单根木料',1]];
    new THREE.GLTFLoader().load('huchen.json',asset=>{
      const poles=asset.scene.children.filter(o=>/^B\d+/.test(o.name)).sort((a,b)=>a.name.localeCompare(b.name));
      if(poles.length!==8)return;
      stages.forEach(([name,count],i)=>{
        const x=(i-1)*2.50;
        const disk=new THREE.Mesh(new THREE.CylinderGeometry(.68,.72,.09,48),material(stone));disk.position.set(x,.953,0);g.add(disk);
        const model=new THREE.Group();
        poles.slice(0,count).forEach((o,j)=>{const copy=o.clone(true);copy.traverse(n=>{if(n.isMesh)n.material=material(j===0?oak:walnut);});model.add(copy);});
        model.rotation.y=.35+i*.20;const bb=new THREE.Box3().setFromObject(model),sz=bb.getSize(new THREE.Vector3());const scale=Math.min(1.53/sz.x,1.03/sz.z,.88/sz.y);model.scale.setScalar(scale);
        bb.setFromObject(model);const c=bb.getCenter(new THREE.Vector3());model.position.set(x-c.x,1.0-bb.min.y,-c.z);g.add(model);pieceLabel(g,name,x,.72,.62,1.08);
      });
    });
  }
  function floorMat(scene,x,y,z,w,d,yaw=0) {
    const g=stand(scene,[x,y,z],yaw);box(g,0,0,0,w,.018,d,blackwood);
    box(g,0,.017,0,w-.06,.019,d-.06,brass);box(g,0,.033,0,w-.11,.016,d-.11,green);
  }

  function mountPlate(scene,id,plate,anisotropy) {
    const [key,title,pos,yaw,w,lines]=plate, h=w*950/2048;
    const g=stand(scene,pos,yaw);g.name=title+' · 展陈';g.userData.noEdit=true;
    box(g,0,0,-.075,w+.19,h+.18,.16,walnut);
    box(g,0,0,.014,w+.056,h+.054,.029,brass);
    for(const s of [-1,1]) {
      box(g,s*(w/2+.14),0,-.025,.073,h+.4,.12,green);
      box(g,s*(w/2+.14),h/2+.15,-.025,.078,.045,.13,brass);
    }
    box(g,0,h/2+.13,-.025,w+.40,.072,.23,walnut);
    const mesh=surface(g,w,h,key,anisotropy,title,lines);
    const faces=panels[id]??=[];
    faces.push(mesh);
    labels[key]=title;
    const reverse=mesh.clone();
    reverse.rotation.y=Math.PI;reverse.position.z=-.165;
    g.add(reverse);faces.push(reverse);
    const ground=id==='xu'||id==='zt'?3.90:id==='ti'?1.82:.60;
    displayBay(g,key,w,h,ground-pos[1]);
    const leg=pos[1]-ground-h/2;
    if(leg>.05) for(const s of [-1,1]) {
      box(g,s*w*.39,-h/2-leg/2,-.075,.09,leg,.12,walnut);
      box(g,s*w*.39,-h/2-leg+.034,-.04,.32,.068,.45,brass);
    }
  }
  function samples(scene,id,cfg,makeSample,addExhibit,finish) {
    const [,,pos,yaw,w]=cfg.plates[0];const z=pos[2];
    const g=stand(scene,[pos[0]+.72,.60,z],yaw);g.name='榫样研习台';rail(g,w-.08,.85,1.12);
    g.updateWorldMatrix(true,false);
    const spacing=w*.315;
    cfg.samples.forEach((name,i)=>{
      const x=(i-1)*spacing;
      box(g,x,.885,0,spacing*.91,.07,.91,stone);
      const root=new THREE.Group(),parts=[]; const k=id==='e1'?1.3:1.32;
      const sample=makeSample(name);
      for(const [object,direction] of sample) {
        const part=new THREE.Group(); part.add(object);part.userData.dir=new THREE.Vector3(...direction);parts.push(part);root.add(part);
      }
      root.scale.setScalar(k);root.rotation.y=yaw+(name==='燕尾榫'?.7:-.32);
      const bounds=new THREE.Box3().setFromObject(root),mid=bounds.getCenter(new THREE.Vector3());
      const location=g.localToWorld(new THREE.Vector3(x,.925,0));root.position.set(location.x-mid.x,location.y-bounds.min.y,location.z-mid.z);
      finish(root);addExhibit({id:id+'-sample-'+i,hall:id==='e1'?'展厅一':'展厅二',kind:'spec',title:name,root,parts,p:'点木样让构件沿装配方向分开，再点一次合拢。',list:[],note:'教学榫样，尺寸为示意。'});
      pieceLabel(g,name,x,.68,.53,Math.min(1.18,spacing*.85));
    });
  }
  function surround(scene,x,y,z,w,d) {
    const g=stand(scene,[x,y,z],0); const height=.08;
    box(g,0,height/2,0,w,height,d,walnut);box(g,0,height+.008,0,w-.06,.016,d-.06,brass);
    box(g,0,height+.025,0,w-.105,.030,d-.105,green);
  }
  // Low outdoor reading benches sit clear of the original buildings and bridge deck.
  function stationPoint(cfg,x,y,z) {
    return [cfg.at[0]+x*Math.cos(cfg.yaw)+z*Math.sin(cfg.yaw),cfg.at[1]+y,
            cfg.at[2]-x*Math.sin(cfg.yaw)+z*Math.cos(cfg.yaw)];
  }
  function benchViews(cfg,id) {
    const shot=(eye,look,fov)=>[stationPoint(cfg,...eye),stationPoint(cfg,...look),fov];
    const views={[id==='ta'?'全塔':'全桥']:cfg.overview,...cfg.angles};
    views['研习台']=shot([.22,2.45,cfg.span*.94],[0,1.22,0],47);
    if(id==='qq')views['牛头榫']=shot([1.08,2.02,1.8],[.39,1.27,0],42);
    return views;
  }
  async function outdoorBench(scene,id,cfg,addExhibit,finish,anisotropy) {
    const g=stand(scene,cfg.at,cfg.yaw),w=cfg.span;g.name=(id==='ta'?'塔前':'桥畔')+'木作研习台';
    const depth=1.32,top=.90;
    for(const sign of [-1,1]){
      box(g,sign*w*.37,.09,0,.44,.16,.99,stone);
      box(g,sign*w*.37,.45,0,.17,.61,.72,blackwood);
      box(g,sign*w*.37,.741,0,.32,.07,.87,brass);
    }
    box(g,0,.68,0,w*.79,.12,.26,walnut);
    box(g,0,.806,0,w,.15,depth,walnut);
    box(g,0,.884,0,w+.02,.022,depth+.02,brass);
    box(g,0,top,0,w-.02,.022,depth-.02,green);
    const [key,title]=cfg.plates[0],pw=id==='ta'?1.63:1.79,px=w/2-pw/2-.14;
    const panel=new THREE.Group();panel.position.set(px,1.26,.235);panel.rotation.x=-.72;g.add(panel);
    const ph=pw*1060/2260;
    for(const side of [-1,1])box(g,px+side*pw*.35,1.077,-.15,.054,.34,.082,brass);
    box(panel,0,0,-.032,pw+.07,ph+.07,.075,walnut);
    box(panel,0,0,.012,pw+.032,ph+.032,.022,brass);
    const page=surface(panel,pw,ph,key,anisotropy,title);(panels[id]??=[]).push(page);labels[key]=title;
    // The return path meets the opposite side. Tilt a second face upward from that approach.
    const other=panel.clone();other.rotation.y=Math.PI;other.rotation.order='YXZ';
    other.position.z-=ph*Math.sin(.72)+.10;g.add(other);
    (panels[id]).push(other.getObjectByName(page.name));
    const gltfLoader=new THREE.GLTFLoader();
    await Promise.all(cfg.models.map(async ([asset,title,x,scale,twist])=>{
      const gltf=await gltfLoader.loadAsync(directory+'out-'+asset+'.glb'), root=new THREE.Group(),parts=[];
      for(const member of [...gltf.scene.children]){
        const part=new THREE.Group();part.userData.dir=new THREE.Vector3(member.userData.shift||0,member.userData.rise||0,0);
        part.add(member);root.add(part);parts.push(part);
        member.traverse(o=>{if(o.isMesh){o.castShadow=o.receiveShadow=true;const m=o.material;if(m.emissive){m.emissive.copy(m.color);m.emissiveMap=m.map||null;m.emissiveIntensity=.11;}}});
      }
      const wide=asset==='arch'?2.78:asset==='dou'?1.9:1.14;
      box(g,x,.928,0,wide,.035,1.17,blackwood);
      if(asset==='arch')for(const side of [-1,1])box(g,x+side*1.23,.988,0,.19,.096,.77,stone);
      const y=asset==='arch'?1.035:.948;
      root.position.set(...stationPoint(cfg,x,y,0));root.rotation.y=cfg.yaw+twist;root.scale.setScalar(scale);root.name=title;
      pieceLabel(g,title,x,.797,.666,Math.min(wide*.86,1.4));
      finish(root);addExhibit({id:'out-'+asset,hall:id==='ta'?'北院木塔':'木拱廊桥',kind:'spec',title,root,parts,
        p:({dou:'点木样对读斗耳、上开槽与下开槽。',arch:'点拱架可分开三节苗、五节苗与横木。',joint:'点平苗看燕尾从上方退出，再点合拢。'})[asset],
        list:[],note:'教学构造示意。'});
    }));
  }
  async function install(scene,makeSample,addExhibit,finish,anisotropy) {
    const pending=[];
    for(const [id,cfg] of Object.entries(stations)) {
      if(cfg.outdoor){pending.push(outdoorBench(scene,id,cfg,addExhibit,finish,anisotropy));continue;}
      for(const plate of cfg.plates)mountPlate(scene,id,plate,anisotropy);
      if(cfg.samples)samples(scene,id,cfg,makeSample,addExhibit,finish);
    }
    readingTable(scene,makeSample);workbenches(scene,makeSample);artBench(scene,anisotropy);
    floorMat(scene,-39.9,.639,24.58,4.62,3.45);
    floorMat(scene,-40.05,.639,10.83,3.55,3.10);
    floorMat(scene,-4.3,.637,-31.25,3.72,2.65);
    floorMat(scene,4.3,.637,-31.25,4.47,2.65);
    // Low reveals extend the existing plinths; no extra island in the visitor aisle.
    for(const args of [[-40.1,1.003,24.52,3.24,2.5],[-40.2,1.03,10.75,1.82,1.82],[-4.3,1.003,-31.25,1.79,1.56],[4.3,1.003,-31.25,3.40,1.55]])
      surround(scene,...args);
    // Brass plan traces beneath the eight members show their radial arrangement.
    const field=stand(scene,[32,.975,-71],0);
    for(const [radius,y,height,finish] of [[3.16,-.20,.25,walnut],[3.18,-.075,.031,brass],[3.14,-.045,.055,green]]) {
      const disc=new THREE.Mesh(new THREE.CylinderGeometry(radius,radius,height,80),material(finish));
      disc.position.y=y;field.add(disc);
    }
    for(const r of [1.04,2.36]) {
      const ring=new THREE.Mesh(new THREE.RingGeometry(r,r+.021,96),material(brass));ring.rotation.x=-half;field.add(ring);
    }
    for(let i=0;i<8;i++){const t=stand(field,[0,.008,0],i*Math.PI/4);box(t,0,0,2.28,.035,.016,.18,brass);}
    await Promise.all(pending);
  }
  function collision(){
    const g=new THREE.Group();g.name='museum_base';
    function block(x,y,z,w,h,d,yaw) {const m=new THREE.Mesh(new THREE.BoxGeometry(w,h,d));m.rotation.y=yaw;m.position.set(x,y,z);g.add(m);}
    for(const [id,cfg] of Object.entries(stations)){
      if(cfg.outdoor){block(cfg.at[0],.48,cfg.at[2],cfg.span,.96,1.34,cfg.yaw);continue;}
      const ground=id==='xu'||id==='zt'?3.9:id==='ti'?1.82:.60;
      for(const [,,pos,yaw,w] of cfg.plates)block(pos[0],ground+1.06,pos[2],w+.22,2.12,.20,yaw);
      if(cfg.samples){const [,,pos,yaw,w]=cfg.plates[0];block(pos[0]+.72,ground+.425,pos[2],w-.08,.85,1.12,yaw);}
    }
    block(32,1.06,-75.48,7.85,.9,1.25,0);
    return g;
  }
  function views(id) {
    const s=stations[id];if(!s)return null;
    if(s.outdoor)return benchViews(s,id);
    const result={'全厅':s.overview};
    for(const [key,title,p,yaw,w] of s.plates){
      const names={wedge:'透榫图',dove:'燕尾图',chair:'圈椅',desk:'平头案',loop:'互承图',float:'悬浮斗栱',route:'烫样图',bracket:'铺作图',pagoda:'剖面图',box:'方盒图',index:'器物图'};
      const dist=w*.83; result[names[key]]=[[p[0]+Math.sin(yaw)*dist,p[1]+.05,p[2]+Math.cos(yaw)*dist],p,46];
    }
    if(s.samples){const p=s.plates[0][2];result['榫样台']=[[p[0]+3.3,2.38,p[2]+1.12],[p[0]+.68,1.57,p[2]],55];}
    if(id==='xu')result['烫样图']=[[-5.45,5.40,9.25],s.plates[0][2],54];
    if(id==='e1')result['透榫图']=[[-41.53,4.08,24.6],s.plates[0][2],62];
    if(id==='e3'){
      result['圈椅']=[[-8.15,3.10,-29.4],s.plates[0][2],58];
      result['平头案']=[[8.15,3.10,-29.4],s.plates[1][2],58];
    }
    if(id==='ti')result['立面与剖面']=[[-4.2,4.2,-67.7],[-4.2,4.2,-72.78],52];
    if(id==='e5'){
      result['悬浮装置']=[[28.8,3.6,-66.05],[32.65,3.6,-59.2],49];
      result['窗棂灯墙']=[[31.5,2.65,-63],[41.4,2.65,-63],67];
    }
    if(id==='gf')result['木材墙']=[[40.7,2.65,10.5],[46.14,2.25,10.5],58];
    if(id==='wt')result['榫卯图谱']=[[39.8,2.6,24.65],[46.5,2.05,24.65],66];
    return result;
  }
  function read(key) {
    if(!labels[key])return false;
    showExhibitPage(key,labels[key],readingPages.get(key));return true;
  }
  return {stations,panels,registerPage,collision,install,views,read};
}
