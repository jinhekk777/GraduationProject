const views={
  court:['两进院落',[55,61,105],[0,6,-10],55],
  main:['重檐主殿',[10,7.3,57],[0,13.7,4],58],
  eaves:['主殿檐口',[7,12.7,29],[2,14.5,18],49],
  west:['西配殿',[-20,13.8,35],[-41,5.2,18],56],
  east:['东配殿',[20,13.8,35],[41,5.2,18],56],
  rear:['后殿',[13.7,6.8,-17],[0,5.2,-29],53],
  arcade:['前院回廊',[-13,2,63.6],[-35,2.4,63.6],55],
  annexWest:['北院西厢',[-14,5,-39],[-32,5,-64],52],
  annexEast:['北院东厢',[14,5,-39],[32,5,-64],52],
  pagoda:['北院木塔',[24,20,-32],[0,22,-66],66],
  bells:['木塔层檐',[14,16,-50],[3,14.3,-61],45],
  bridge:['木拱廊桥',[21,4.1,-109],[0,4,-106],65],
  bridgework:['桥廊木作',[8,4.3,-100],[2.2,5,-106],48],
  gate:['山门',[-5.3,3.5,91],[0,5.6,67.8],39]
};
export function exteriorViews(select,camera,controls,aim){
  for(const [key,[label]] of Object.entries(views))select.add(new Option(label,key));
  function go(key){
    if(!Object.hasOwn(views,key))return false;
    const [,position,target,lens]=views[key];
    controls.minDistance=3;controls.maxPolarAngle=2.22;
    camera.fov=Math.min(103,2*Math.atan(Math.tan(lens*Math.PI/360)*Math.max(1,1.4/camera.aspect))*180/Math.PI);
    camera.updateProjectionMatrix();select.value=key;aim(position,target,650);return true;
  }
  select.addEventListener('change',()=>go(select.value));return go;
}
