木构院落 — 静态网站包

用法：把这个文件夹里的全部文件原样上传到任何静态网站托管服务（GitHub Pages、Netlify、Vercel、Cloudflare Pages 等），入口是 index.html。
注意：
1. 不能直接双击 index.html 打开，必须通过 http(s) 访问（本地预览可在此目录运行：python3 -m http.server 8000，然后打开 http://localhost:8000）。
2. 页面会从 cdn.jsdelivr.net 加载 three.js，从 fonts.googleapis.com 加载字体，访问者需要能连上这两个地址。
3. 文件名和目录结构不要改（compound.part1~5.txt、pingshenke.json、tex/）。
