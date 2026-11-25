// The same height field drives the carved silhouette relief and the colour of the recesses.
function heightCanvas(paint) {
  const c=document.createElement('canvas');c.width=c.height=512;
  const g=c.getContext('2d');g.fillStyle='#202020';g.fillRect(0,0,512,512);
  g.translate(256,256);g.scale(2.42,2.42);g.lineCap='round';g.lineJoin='round';paint(g);
  const blend=document.createElement('canvas');blend.width=blend.height=512;
  const smooth=blend.getContext('2d');smooth.filter='blur(.9px)';smooth.drawImage(c,0,0);
  return smooth.getImageData(0,0,512,512).data;
}
function cut(g,path,fill,edge,width=1.8){
  const p=new Path2D(path);
  if(fill){g.fillStyle=fill;g.fill(p);}
  if(edge){g.strokeStyle=edge;g.lineWidth=width;g.stroke(p);}
}
function dome(g,x,y,rx,ry,height){
  g.save();g.translate(x,y);g.scale(rx,ry);
  const sh=g.createRadialGradient(-.10,-.12,.05,0,0,1);
  sh.addColorStop(0,'rgb('+[height,height,height].join(',')+')');sh.addColorStop(1,'#585858');
  g.fillStyle=sh;g.beginPath();g.arc(0,0,1,0,2*Math.PI);g.fill();g.restore();
}
function fish(g){
  cut(g,'M-63 24 Q-71 8 -60 -16 Q-48 -39 -18 -45 Q6 -51 39 -36 Q48 -41 57 -43 Q54 -31 45 -24 Q56 -16 58 -4 Q43 -8 34 -24 Q14 -35 -5 -29 Q-28 -23 -37 -2 Q-46 22 -57 28 Q-63 30 -63 24Z','#999','#cdcdcd',2.3);
  cut(g,'M-32 -30 Q-18 -55 8 -56 Q1 -46 -3 -43 M-36 -3 Q-18 -10 -17 -23 Q-31 -19 -40 -12Z','#7d7d7d','#bebebe');
  cut(g,'M-59 -12 Q-43 -10 -42 3 M-63 18 Q-70 13 -74 15 M-62 22 Q-69 25 -72 24',null,'#dadada');
  dome(g,-58,7,3.3,4.5,221);
  for(let i=0;i<5;i++)for(let j=0;j<2;j++){
    g.save();g.translate(-44+i*10,-20-i*2.7+j*6);g.rotate(-.55+i*.18);
    cut(g,'M0 -3 Q9 0 0 4',null,'#656565',1.6);g.restore();
  }
}
function pond(g){
  for(const [r,w] of [[99,3],[94,1.4],[85,2.4]]){g.beginPath();g.arc(0,0,r,0,Math.PI*2);g.lineWidth=w;g.strokeStyle='#bcbcbc';g.stroke();}
  for(let k=0;k<30;k++){g.save();g.rotate(k*Math.PI/15);cut(g,'M-4 -89 Q-1 -95 4 -91 L0 -88Z','#909090');g.restore();}
  for(let i=0;i<2;i++){g.save();g.rotate(i*Math.PI-.28);fish(g);g.restore();}
  for(let i=0;i<8;i++){g.save();g.rotate(i*Math.PI/4);cut(g,'M0 -3 Q-11 -13 0 -22 Q11 -13 0 -3Z','#929292','#c6c6c6',1.5);g.restore();}
  dome(g,0,0,6,6,192);
  for(const s of [1,-1]){g.save();g.scale(s,s);cut(g,'M-68 -20 Q-52 -56 -20 -59 Q6 -64 22 -56 M-59 -39 Q-40 -66 -12 -68',null,'#777',2);g.restore();}
}
function lion(g){
  dome(g,0,0,81,82,143);
  for(const sign of [1,-1]){
    g.save();g.scale(sign,1);
    cut(g,'M16 -62 Q29 -88 56 -87 Q70 -86 79 -66 Q63 -70 56 -59 Q79 -64 88 -42 Q69 -48 65 -37 Q86 -39 90 -15 Q72 -21 64 -11 Q81 -12 80 12 Q65 4 56 16 Q67 20 64 42 Q50 31 43 40 Q36 53 23 58', '#858585','#bdbdbd',3);
    for(let i=0;i<4;i++){
      g.save();g.translate(49+7*Math.sin(i),-65+i*22);g.rotate(.44+i*.12);
      cut(g,'M-8 4 Q-9 -9 4 -10 Q15 -11 16 -2 Q15 6 7 5 Q3 4 5 0',null,'#cecece',3);g.restore();
    }
    dome(g,30,-19,27,19,196);
    cut(g,'M8 -25 Q18 -46 40 -48 Q56 -50 64 -39 Q45 -40 31 -28 Q22 -20 8 -19Z','#bdbdbd','#d9d9d9',2);
    cut(g,'M15 -15 Q31 -27 47 -18 Q35 -4 16 -11Z','#444','#d1d1d1',2);dome(g,31,-16,5,6,220);
    dome(g,24,17,24,23,186);
    for(let i=0;i<3;i++)cut(g,'M'+(19+i*6)+' 19 q4 4 8 2',null,'#666',2);
    cut(g,'M28 30 Q24 48 16 48 L15 31Z','#c7c7c7','#929292');
    g.restore();
  }
  dome(g,0,-9,13,27,206);dome(g,0,8,21,12,226);
  dome(g,-10,10,5,3,80);dome(g,10,10,5,3,80);
  cut(g,'M-25 35 Q0 27 25 35 Q17 52 0 51 Q-16 51 -25 35Z','#454545','#a5a5a5',2.5);
  cut(g,'M-12 -57 Q-8 -72 0 -79 Q10 -66 12 -57 L0 -62Z','#bdbdbd','#ddd',2);
}
export function carvedDisc(T,kind,radius,depth,color){
  const pixels=heightCanvas(kind==='bronze'?lion:pond),rings=kind==='bronze'?66:84,sides=160;
  const pos=[],colors=[],ix=[];const base=new T.Color(color).convertSRGBToLinear();
  function vertex(x,y,r){
    const u=Math.max(0,Math.min(511,Math.round((.5+x*.475)*512))),v=Math.max(0,Math.min(511,Math.round((.5-y*.475)*512)));
    const h=pixels[4*(v*512+u)]/255;
    pos.push(x*radius,y*radius,depth*(h-.125)*Math.min(1,(1-r)*42));
    const light=kind==='bronze'?.63+.96*h:.53+.86*h;colors.push(base.r*light,base.g*light,base.b*light);
  }
  vertex(0,0,0);
  for(let j=1;j<=rings;j++)for(let k=0;k<sides;k++){
    const r=j/rings,a=2*Math.PI*k/sides,i=1+(j-1)*sides+k,next=1+(j-1)*sides+(k+1)%sides;
    vertex(r*Math.cos(a),r*Math.sin(a),r);
    if(j===1)ix.push(0,i,next);else ix.push(i-sides,i,next,next,next-sides,i-sides);
  }
  const g=new T.BufferGeometry();g.setAttribute('position',new T.Float32BufferAttribute(pos,3));g.setAttribute('color',new T.Float32BufferAttribute(colors,3));g.setIndex(ix);g.computeVertexNormals();return g;
}
