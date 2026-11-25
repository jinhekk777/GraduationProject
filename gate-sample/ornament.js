// Painted and moulded ornaments are drawn at the proportions of the existing gate.
function canvas(w,h,paint){
  const c=document.createElement('canvas');c.width=w;c.height=h;paint(c.getContext('2d'));return c;
}
function shape(g,d,fill,stroke,width=2){
  const p=new Path2D(d);
  if(fill){g.fillStyle=fill;g.fill(p);}
  if(stroke){g.strokeStyle=stroke;g.lineWidth=width;g.stroke(p);}
}
const ink='#203d40', gold='#d4b476', ivory='#e3d7ad';
function lotus(g,x,y,r,coral){
  g.save();g.translate(x,y);g.scale(r/100,r/100);
  for(const [count,scale,offset] of [[10,1,0],[8,.66,.22]]){
    for(let i=0;i<count;i++){
      g.save();g.rotate(i*2*Math.PI/count+offset);g.scale(scale,scale);
      shape(g,'M0 -9 C-12 -20 -34 -35 -29 -56 C-25 -72 -9 -76 0 -96 C10 -75 27 -70 30 -53 C32 -33 12 -18 0 -9Z',coral?'#b96650':'#579786',ink,3);
      shape(g,'M-23 -53 Q-15 -68 0 -82 Q15 -65 23 -53 Q9 -61 0 -68 Q-10 -58 -23 -53Z',coral?'#e2a17a':'#a9c7a4');
      shape(g,'M0 -24 Q-3 -46 0 -66',null,gold,2.6);g.restore();
    }
  }
  g.fillStyle='#c69b4f';g.beginPath();g.arc(0,0,17,0,2*Math.PI);g.fill();
  for(let i=0;i<7;i++){g.fillStyle=ink;g.beginPath();g.arc(10*Math.cos(i*.898),10*Math.sin(i*.898),2,0,2*Math.PI);g.fill();}
  g.restore();
}
function sprig(g,x,y,length,mirror,coral){
  g.save();g.translate(x,y);g.scale(mirror*length/450,1);
  const line='M0 9 C59 -43 111 -48 168 -8 S278 42 327 -10 S418 -40 452 -3';
  shape(g,line,null,ink,8);shape(g,line,null,gold,3.2);
  for(const [px,py,k] of [[55,-23,1],[125,-28,-1],[190,6,1],[255,20,-1],[325,-8,1],[393,-30,-1]]){
    g.save();g.translate(px,py);g.scale(1,k);
    shape(g,'M0 0 C12 -30 21 -45 50 -46 Q41 -30 62 -32 C48 -13 25 -20 10 -4 Q30 -6 47 6 C28 13 14 6 0 0Z','#66977d',ink,3);
    shape(g,'M3 -1 Q25 -23 46 -38 M13 -6 Q26 -16 38 -12',null,'#c1cba0',2.7);g.restore();
  }
  lotus(g,161,-10,34,coral);lotus(g,331,-11,27,false);g.restore();
}
export function beamAtlas(T,central,anisotropy){
  const image=canvas(2048,512,g=>{
    g.lineCap='round';g.lineJoin='round';
    for(let row=0;row<2;row++){
      g.save();g.translate(0,row*256);
      const blue='#305a6d',green='#386c59',field=row?green:blue;
      g.fillStyle=row?blue:green;g.fillRect(0,0,2048,256);
      // The ends are tied with a band of beads. The middle is framed with three shades of pigment.
      for(const [yy,h,col] of [[1,7,ink],[10,3,gold],[19,5,'#99b79a'],[232,5,'#99b79a'],[243,3,gold],[249,7,ink]]){
        g.fillStyle=col;g.fillRect(0,yy,2048,h);
      }
      for(const [offset,col] of [[0,ink],[5,gold],[10,'#92ac96'],[20,'#294b51'],[26,field]]){
        const q=offset;
        shape(g,`M${382+q} 128 Q${436+q} 102 ${462+q} ${29+q*.62} H${1586-q} Q${1612-q} 102 ${1666-q} 128 Q${1612-q} 154 ${1586-q} ${227-q*.62} H${462+q} Q${436+q} 154 ${382+q} 128Z`,col);
      }
      for(const sign of [1,-1]){
        g.save();if(sign<0){g.translate(2048,0);g.scale(-1,1);}
        for(const [xx,width,col] of [[9,6,gold],[24,34,ink],[29,4,'#7a9c8a'],[63,6,gold],[79,8,'#abc2a2']]){g.fillStyle=col;g.fillRect(xx,0,width,256);}
        for(let y=12;y<256;y+=23){g.beginPath();g.arc(45,y,5.1,0,2*Math.PI);g.fillStyle='#cbb589';g.fill();}
        for(const [dx,c] of [[0,ink],[8,'#90b29b'],[18,gold],[29,'#63928a']])shape(g,`M${391+dx} 24 Q${379+dx} 78 ${323+dx} 128 Q${379+dx} 179 ${391+dx} 232`,null,c,5);
        lotus(g,214,128,88,central!==!!row);
        for(const yy of [40,216]){g.save();g.translate(214,yy);g.scale(1,yy===40?1:-1);shape(g,'M-104 36 Q-69 -3 -22 3 M22 3 Q69 -3 105 36',null,gold,3);g.restore();}
        g.restore();
      }
      sprig(g,990,128,central?475:445,-1,central);
      sprig(g,1058,128,central?475:445,1,central);
      if(central)lotus(g,1024,128,55,true);
      else {g.save();g.translate(1024,128);shape(g,'M-59 6 Q-88 -3 -62 -20 Q-45 -32 -29 -18 Q-31 -45 -6 -43 Q18 -44 19 -23 Q34 -37 53 -25 Q78 -8 51 5 Q33 11 26 0 Q6 3 -3 29 Q-17 11 -34 5 Q-48 16 -59 6Z','#718d76',gold,4);g.restore();}
      g.restore();
    }
    const pix=g.getImageData(0,0,2048,512);let seed=93;
    for(let i=0;i<pix.data.length;i+=4){seed=(Math.imul(seed,1664525)+1013904223)>>>0;const d=(seed/4294967296-.5)*7;for(let k=0;k<3;k++)pix.data[i+k]+=d;}
    g.putImageData(pix,0,0);
  });
  const t=new T.CanvasTexture(image);t.encoding=T.sRGBEncoding;t.anisotropy=anisotropy;t.wrapS=t.wrapT=T.RepeatWrapping;return t;
}
export function clayStamp(T){
  const c=canvas(256,256,g=>{
    g.fillStyle='#585858';g.fillRect(0,0,256,256);g.translate(128,128);
    for(const r of [114,104,31]){g.beginPath();g.arc(0,0,r,0,2*Math.PI);g.strokeStyle='#cecece';g.lineWidth=7;g.stroke();}
    for(let i=0;i<8;i++){g.rotate(Math.PI/4);shape(g,'M0 -35 Q-32 -55 -19 -75 Q-9 -85 0 -97 Q10 -83 20 -73 Q31 -51 0 -35Z','#aaa','#dcdcdc',4);shape(g,'M0 -40 Q-4 -61 0 -77',null,'#6d6d6d',4);}
    g.beginPath();g.arc(0,0,14,0,2*Math.PI);g.fillStyle='#c4c4c4';g.fill();
  });
  return new T.CanvasTexture(c);
}
export function stoneCarving(T){
  const c=canvas(1024,128,g=>{
    g.fillStyle='#747474';g.fillRect(0,0,1024,128);g.lineCap='round';
    for(let i=0;i<4;i++){
      g.save();g.translate(256*i,0);
      for(const [dy,col,lw] of [[2,'#424242',9],[0,'#aaa',5]]){
        g.save();g.translate(0,dy);
        shape(g,'M0 76 C42 76 42 43 72 39 C107 33 112 64 97 68 C84 72 78 61 84 54 M81 40 C122 29 130 82 173 80 S216 48 256 53 M164 79 Q181 54 201 57 Q195 76 174 79',null,col,lw);
        g.restore();
      }
      g.restore();
    }
    g.strokeStyle='#b9b9b9';g.lineWidth=3;g.strokeRect(7,10,1010,108);
  });
  return new T.CanvasTexture(c);
}
