import { createServer } from 'node:http';
import { readFile, readdir, stat } from 'node:fs/promises';
import { resolve, dirname, extname, sep } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { parseArgs } from 'node:util';
import { runAnalyzer } from './bridge.mjs';

export const viewerRoot = dirname(fileURLToPath(import.meta.url));
const projectRoot = resolve(viewerRoot, '..');
export const defaultAnalyzer = resolve(projectRoot, 'build', process.platform === 'win32' ? 'windows-debug/lab-model.exe' : 'linux-debug/lab-model');
const examplesRoot = resolve(projectRoot, 'examples');

async function readExamples() {
  const catalog = JSON.parse(await readFile(resolve(examplesRoot, 'expanded/cases.json'), 'utf8'));
  const expanded = await Promise.all(catalog.examples.map(async entry => ({
    id: `expanded/${entry.file}`, name: entry.file, title: entry.title,
    category: entry.group, description: entry.focus, method: entry.method, original: false,
    source: await readFile(resolve(examplesRoot, 'expanded', entry.file), 'utf8'),
  })));
  const names = (await readdir(examplesRoot)).filter(name => /^\d\d-[\w-]+\.cs$/.test(name)).sort();
  const original = await Promise.all(names.map(async name => ({
    id: name, name, title: name.replace(/^\d+-|\.cs$/g, '').replaceAll('-', ' '),
    category: 'Исходные примеры', description: 'Начальный набор лабораторной', original: true,
    source: await readFile(resolve(examplesRoot, name), 'utf8'),
  })));
  return [...expanded, ...original];
}

function send(res, status, body) {
  if (res.destroyed) return;
  res.writeHead(status, { 'Content-Type': 'application/json; charset=utf-8', 'Cache-Control': 'no-store' });
  res.end(JSON.stringify(body));
}
async function readJson(req) {
  const chunks = [];
  let size = 0;
  for await (const chunk of req) {
    size += chunk.length;
    if (size > 2 * 1024 * 1024) throw new Error('Request exceeds 2 MiB.');
    chunks.push(chunk);
  }
  return JSON.parse(Buffer.concat(chunks).toString('utf8'));
}

export function createApp({ analyzer = defaultAnalyzer, timeoutMs = 5000, assets } = {}) {
  const active = new Set();
  const server = createServer(async (req, res) => {
    const pathname = new URL(req.url, 'http://localhost').pathname;
    try {
      if (pathname === '/api/examples' && req.method === 'GET') {
        return send(res, 200, await readExamples());
      }
      if (pathname === '/api/analyze' && req.method === 'POST') {
        if (req.headers.origin && req.headers.origin !== `http://${req.headers.host}`)
          return send(res, 403, { error: 'Cross-origin requests are not allowed.' });
        if (!req.headers['content-type']?.startsWith('application/json'))
          return send(res, 415, { error: 'Expected application/json.' });
        let body;
        try { body = await readJson(req); }
        catch (error) { return send(res, 400, { error: error.message }); }
        if (body?.protocolVersion !== 1 || !Number.isSafeInteger(body?.requestId) || typeof body?.source !== 'string' || typeof body?.view !== 'string')
          return send(res, 400, { error: 'Invalid analysis request.' });
        if (Buffer.byteLength(body.source, 'utf8') > 256 * 1024)
          return send(res, 413, { error: 'Source exceeds 256 KiB.' });
        const controller = new AbortController();
        active.add(controller);
        const cancel = () => { if (!res.writableEnded) controller.abort(); };
        res.once('close', cancel);
        try {
          const response = await runAnalyzer(analyzer, body, { signal: controller.signal, timeoutMs });
          send(res, 200, response);
        } catch (error) { send(res, 502, { error: error.message }); }
        finally { active.delete(controller); res.off('close', cancel); }
        return;
      }
      if (pathname.startsWith('/api/')) return send(res, 404, { error: 'Unknown API route.' });
      if (assets) return assets(req, res);
      send(res, 404, { error: 'Not found.' });
    } catch (error) { send(res, 500, { error: error.message }); }
  });
  server.on('close', () => { for (const controller of active) controller.abort(); });
  server.cancelAnalyses = () => { for (const controller of active) controller.abort(); };
  return server;
}

async function main() {
  const { values } = parseArgs({ options: {
    analyzer: { type: 'string' }, port: { type: 'string', default: '3000' }, production: { type: 'boolean', default: false }
  } });
  const analyzer = values.analyzer ? resolve(values.analyzer) : defaultAnalyzer;
  await stat(analyzer).catch(() => { throw new Error(`Analyzer not found: ${analyzer}. Build the CMake preset first, or pass --analyzer <path>.`); });
  const port = Number(values.port);
  if (!Number.isInteger(port) || port < 1 || port > 65535) throw new Error('Invalid port.');
  let serveAssets;
  const server = createApp({ analyzer, assets: (req, res) => serveAssets(req, res) });
  let vite;
  if (values.production) {
    const dist = resolve(viewerRoot, 'dist');
    await stat(resolve(dist, 'index.html'));
    const mime = { '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css', '.wasm': 'application/wasm', '.svg': 'image/svg+xml', '.ttf': 'font/ttf' };
    serveAssets = async (req, res) => {
      try {
        const pathname = decodeURIComponent(new URL(req.url, 'http://localhost').pathname);
        const path = resolve(dist, '.' + (pathname === '/' ? '/index.html' : pathname));
        if (!path.startsWith(dist + sep)) return send(res, 403, { error: 'Forbidden.' });
        const content = await readFile(path);
        res.writeHead(200, { 'Content-Type': mime[extname(path)] ?? 'application/octet-stream' });
        res.end(content);
      } catch { send(res, 404, { error: 'Not found.' }); }
    };
  } else {
    const { createServer: createViteServer } = await import('vite');
    vite = await createViteServer({ root: viewerRoot, server: { middlewareMode: true, hmr: { server } }, appType: 'spa' });
    serveAssets = vite.middlewares;
  }
  server.on('error', error => { console.error(error.message); process.exitCode = 1; vite?.close(); });
  server.listen(port, '127.0.0.1', () => console.log(`Syntax Lab: http://127.0.0.1:${port}\nAnalyzer: ${analyzer}`));
  const shutdown = async () => {
    server.cancelAnalyses();
    await vite?.close();
    server.close();
    server.closeAllConnections();
  };
  process.once('SIGINT', shutdown);
  process.once('SIGTERM', shutdown);
}
if (process.argv[1] && import.meta.url === pathToFileURL(resolve(process.argv[1])).href)
  main().catch(error => { console.error(error.message); process.exitCode = 1; });
