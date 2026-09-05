import { readFile, writeFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { build } from 'esbuild';

const fragmentUrl = new URL('../visualization/intersection.fragment.html', import.meta.url);
const outputUrl = new URL('../public/intersection.html', import.meta.url);
const fragment = await readFile(fragmentUrl, 'utf8');
if (!fragment.includes('id="lumacross-clear-intersection"') || !fragment.includes('<script>')) {
  throw new Error('Intersection fragment is incomplete.');
}
const coreBuild = await build({
  entryPoints: [fileURLToPath(new URL('./browser-core-entry.ts', import.meta.url))],
  bundle: true,
  format: 'iife',
  platform: 'browser',
  write: false,
  minify: true,
  target: ['es2022'],
});
const browserCore = coreBuild.outputFiles[0].text;
const document = `<!doctype html>
<html lang="it">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="referrer" content="no-referrer">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; img-src data:; font-src data:; object-src 'none'; base-uri 'none'; form-action 'none'">
<title>Sem.01 · LumaCross Control</title>
<style>html,body{margin:0;min-height:100%;background:#e8edef}</style>
</head>
<body>
<script>${browserCore}</script>
${fragment}
</body>
</html>
`;
await writeFile(outputUrl, document);
