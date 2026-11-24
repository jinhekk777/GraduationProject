/* The fitted interior uses gallery-local metres; the entrance to the existing west wing stays on its east side. */
function createTimberGallery(THREE) {
  const origin = new THREE.Vector3(-32, .635, -64);
  const root = new THREE.Group(), fixtures = new THREE.Group(), panels = [];
  root.name = '展厅四 · 大木作陈列'; root.userData.noEdit = true; root.position.copy(origin); root.add(fixtures);
  const titles = {
    intro: '一榀屋架', taiwall: '抬梁式 · 轴测与投影', chuanwall: '穿斗式 · 轴测与投影',
    beam: '梁上立柱', post: '枋穿柱列', arm: '斗栱出挑', tai: '抬梁式屋架', chuan: '穿斗式屋架',
    study: '穿枋贯柱 · 合装剖视', dou: '清式平身科', guide: '从一层构件到一榀屋架'
  };
  const keyLight = new THREE.SpotLight(0xffe7c3, 1.95, 25, 1.04, .58, 1.1);
  keyLight.position.set(-8.0, 5.6, .7); keyLight.target.position.set(1.0, .3, -6.7);
  keyLight.castShadow = true; keyLight.shadow.mapSize.set(1024,1024);
  keyLight.shadow.camera.near = .45; keyLight.shadow.camera.far = 25;
  keyLight.shadow.bias = -.00035; keyLight.shadow.normalBias = .016;
  root.add(keyLight,keyLight.target); keyLight.visible = false;
  const groups = new Map();
  function groupFor(id) {
    if (!groups.has(id)) {
      const group = new THREE.Group();
      group.name = id; group.position.copy(origin); groups.set(id, group);
    }
    return groups.get(id);
  }
  async function install(scene, addExhibit, finishMaterials, anisotropy) {
    const gltf = await new THREE.GLTFLoader().loadAsync('e4-gallery/gallery.gltf');
    for (const node of [...gltf.scene.children]) {
      const {exhibit, plate} = node.userData;
      node.traverse(mesh => {
        if (!mesh.isMesh) return;
        const m = mesh.material;
        if (m.map) m.map.anisotropy = Math.min(anisotropy, 8);
        if (plate) {
          m.toneMapped = false;
          if (titles[plate]) { mesh.userData.plate = plate; panels.push(mesh); }
        } else {
          mesh.castShadow = mesh.receiveShadow = true;
          if (m.emissive) { m.emissive.copy(m.color); m.emissiveMap = m.map; m.emissiveIntensity = /杉木|榆木|斗栱/.test(m.name) ? .51 : .32; }
        }
      });
      (exhibit ? groupFor(exhibit) : fixtures).add(node);
    }
    scene.add(root); finishMaterials(root);
    groups.forEach(g => finishMaterials(g));
    const hall = '展厅四 · 大木作';
    for (const [id, title, p, list] of [
      ['liangjia','抬梁式屋架','从柱顶的五架梁往上看，短柱托住三架梁，最高处的脊瓜柱承托脊檩。点屋架可以分层观察。',['五架梁与三架梁','金瓜柱与脊瓜柱','檩与椽']],
      ['chuandou','穿斗式屋架','先找随屋面逐级升高的落地柱，再沿横枋追踪它们怎样连在一起。点屋架可以分层观察。',['落地柱直接承檩','穿枋横向联柱','纵向联系与椽']]
    ]) {
      const g = groups.get(id);
      for (const layer of g.children) layer.userData.base = layer.position.clone();
      addExhibit({id,hall,title,p,list,root:g,layers:g.children,layerGap:.205,note:'屋架为构造示意。'});
    }
    const bracket = groups.get('pingshenke');
    addExhibit({id:'pingshenke',hall,title:'五踩单翘单昂平身科',root:bracket,
      parts:bracket.children.map(o => ({o,base:o.position.clone(),c:o.userData.course,dou:o.userData.dou})),spread:.31,
      p:'从下方的坐斗向上看，翘和栱在两个方向交叠。点展品分层，右侧展板可以放大对读。',
      list:['坐斗与头翘','升、栱与昂','上部枋木'],
      note:'原展品为预览模型，部分步骤为推测，耍头与拽枋的卡口仍待考证。'});
    const joint = groups.get('through-tie'), beam = joint.children.find(o => o.userData.moving);
    beam.userData.dir = new THREE.Vector3(.435,0,0);
    addExhibit({id:'through-tie',hall,title:'穿枋贯柱',root:joint,kind:'spec',parts:[beam],
      p:'沿着枋端可以看到榫肩和柱上的贯通卯口。点木样推合，再点一次抽出；台前的图解可以放大查看柱内剖面。',
      list:['枋端与卯口同轴','榫肩抵住柱身'],note:'教学榫样。'});
  }
  function collision() {
    const g = new THREE.Group(); g.name = 'museum_base'; g.position.copy(origin);
    function solid(geo,x,y,z) { const mesh = new THREE.Mesh(geo); mesh.position.set(x,y,z); g.add(mesh); }
    for (const [x,z,r] of [[-4.35,-5.1,3.37],[4.35,-5.45,3.25],[0,-10,1.45]])
      solid(new THREE.CylinderGeometry(r,r,1.25,32),x,.625,z);
    for (const [x,z,width,depth] of [[-4.35,-1.73,2.19,.72],[4.35,-2.20,2.19,.72],[6.95,-1.12,2.27,1.16],[0,-12.85,16.4,.19],[1.27,-12.16,8.23,.72]])
      solid(new THREE.BoxGeometry(width,2.2,depth),x,1.1,z);
    return g;
  }
  // The source compound combines these two former display platforms with all other stonework.
  function oldPlinth(v) {
    return v.y >= .58 && v.y <= .97 && (Math.hypot(v.x+32,v.z+57)<3.23 || Math.hypot(v.x+32,v.z+71)<3.23);
  }
  function readPlate(key) {
    const dlg=document.getElementById('e4-reader'), img=dlg.querySelector('img');
    dlg.querySelector('h2').textContent=titles[key];
    img.alt=titles[key]; img.src='e4-gallery/'+key+'.jpg'; dlg.showModal();
  }
  const reader=document.getElementById('e4-reader');
  reader.querySelector('button').addEventListener('click',()=>reader.close());
  reader.addEventListener('click',e=>{ if(e.target===reader) reader.close(); });
  return {install,collision,oldPlinth,panels,readPlate,lightForVisitor(inside) { keyLight.visible = inside; }};
}
