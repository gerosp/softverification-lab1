import { test, before, after } from 'node:test';
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { readFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import { once } from 'node:events';
import { instance } from '@viz-js/viz';
import { createApp, defaultAnalyzer, viewerRoot } from '../server.mjs';
import { runAnalyzer } from '../bridge.mjs';
import { byteToUtf16 } from '../src/positions.ts';

const analyzer = process.env.LAB_ANALYZER ?? defaultAnalyzer;
const request = (source = 'int F(int x) { return x + 1; }') => ({ protocolVersion: 1, requestId: 17, source, view: 'syntax' });
let server;
let base;
before(async () => {
  server = createApp({ analyzer });
  server.listen(0, '127.0.0.1');
  await once(server, 'listening');
  base = `http://127.0.0.1:${server.address().port}`;
});
after(async () => { server.cancelAnalyses(); server.closeAllConnections(); await new Promise(resolve => server.close(resolve)); });
async function post(body) {
  return fetch(base + '/api/analyze', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
}

test('HTTP bridge forwards source, request ID and syntax diagnostics', async () => {
  const response = await post(request());
  assert.equal(response.status, 200);
  const body = await response.json();
  assert.equal(body.status, 'ok');
  assert.equal(body.requestId, 17);
  assert.ok(body.dot.startsWith('digraph'));
  assert.ok(body.nodeRanges.n0);
  const bad = await (await post(request('int F() { return 1 }'))).json();
  assert.equal(bad.status, 'syntax_error');
  assert.ok(bad.dot);
  assert.ok(bad.diagnostics.some(d => d.code === 'missing_token'));
});

test('invalid requests and unsupported views are explicit', async () => {
  assert.equal((await post({})).status, 400);
  const result = await (await post({ ...request(), view: 'unknown' })).json();
  assert.equal(result.status, 'error');
  assert.equal(result.diagnostics[0].code, 'unsupported_view');
  assert.equal((await post(request(' '.repeat(256 * 1024 + 1)))).status, 413);
});

test('CFG dispatch returns statements and rejects invalid syntax first', async () => {
  const response = await post({ ...request(), view: 'cfg' });
  assert.equal(response.status, 200);
  const cfg = await response.json();
  assert.equal(cfg.status, 'ok');
  assert.deepEqual(cfg.diagnostics, []);
  assert.match(cfg.dot, /^digraph "CFG"/);
  assert.match(cfg.dot, /label="return x \+ 1;"/);
  assert.ok(Object.values(cfg.nodeRanges).some(range => request().source.slice(range.start, range.end) === 'return x + 1;'));
  const invalid = await (await post({ ...request('int F() { return 1 }'), view: 'cfg' })).json();
  assert.equal(invalid.status, 'syntax_error');
  assert.equal(invalid.dot, null);
  assert.ok(invalid.diagnostics.some(d => d.code === 'missing_token'));
});

test('all 57 examples have unique picker IDs and real AST DOT results render through Graphviz', async () => {
  const examples = await (await fetch(base + '/api/examples')).json();
  assert.equal(examples.length, 57);
  assert.equal(new Set(examples.map(example => example.id)).size, 57);
  assert.equal(examples.filter(example => example.original).length, 7);
  assert.equal(examples.filter(example => !example.original).length, 50);
  assert.ok(examples.every(example => example.category && example.title && example.source));
  assert.notEqual(examples.find(example => example.id === '01-sequence.cs').source,
    examples.find(example => example.id === 'expanded/01-sequence.cs').source);
  const viz = await instance();
  for (const example of examples) {
    const body = await runAnalyzer(analyzer, request(example.source));
    assert.equal(body.status, 'ok', example.name);
    const svg = viz.renderString(body.dot, { format: 'svg' });
    assert.match(svg, /<svg/);
    assert.match(svg, /id="n0"/);
  }
  const source = 'string F() { return "quote\\\" backslash\\\\ newline\\n"; }';
  const result = await runAnalyzer(analyzer, request(source));
  assert.equal(result.status, 'ok');
  assert.match(viz.renderString(result.dot, { format: 'svg' }), /<svg/);
});

test('UTF-8 offsets map to UTF-16 across emoji, Cyrillic and CRLF', async () => {
  const source = '// 😀\r\nint Тест(int x)\r\n{ return x; }';
  const result = await runAnalyzer(analyzer, request(source));
  const start = Buffer.byteLength(source.slice(0, source.indexOf('Тест')), 'utf8');
  assert.equal(byteToUtf16(source, start), source.indexOf('Тест'));
  assert.equal(byteToUtf16(source, Buffer.byteLength(source)), source.length);
  assert.ok(Object.values(result.nodeRanges).some(r => r.start === start && r.end === start + Buffer.byteLength('Тест')));
});

test('process launch failure, timeout and cancellation settle', async () => {
  await assert.rejects(runAnalyzer(resolve(viewerRoot, 'not-a-real-executable'), request()), /Cannot launch analyzer/);
  // A near-zero deadline exercises the bridge's kill/close path on the real CLI.
  await assert.rejects(runAnalyzer(analyzer, request(), { timeoutMs: 0 }), /timed out/);
  const controller = new AbortController(); controller.abort();
  await assert.rejects(runAnalyzer(analyzer, request(), { signal: controller.signal }), /cancelled/);
  const active = new AbortController();
  const promise = runAnalyzer(analyzer, request(), { signal: active.signal });
  active.abort();
  await assert.rejects(promise, /cancelled/);
});

function cli(args, stdin = '') {
  return new Promise((resolve, reject) => {
    const child = spawn(analyzer, args, { windowsHide: true });
    let stdout = ''; let stderr = '';
    child.stdout.setEncoding('utf8'); child.stderr.setEncoding('utf8');
    child.stdout.on('data', s => stdout += s); child.stderr.on('data', s => stderr += s);
    child.on('error', reject); child.on('close', code => resolve({ code, stdout, stderr }));
    child.stdin.end(stdin);
  });
}
test('CLI file/stdin exports and exit codes', async () => {
  const example = resolve(viewerRoot, '../examples/01-sequence.cs');
  const file = await cli(['parse', '--input', example, '--format', 'json']);
  const stdin = await cli(['parse', '--input', '-', '--format', 'json'], await readFile(example, 'utf8'));
  assert.equal(file.code, 0); assert.equal(stdin.code, 0);
  assert.deepEqual(JSON.parse(file.stdout), JSON.parse(stdin.stdout));
  assert.equal(JSON.parse(file.stdout).tree.kind, 'method_declaration');
  const sexp = await cli(['parse', '--input', '-', '--format', 'sexp'], request().source);
  assert.match(sexp.stdout, /^\(method_declaration/);
  const malformed = await cli(['--request-json'], '{');
  assert.equal(malformed.code, 2); assert.equal(JSON.parse(malformed.stdout).status, 'error');
  const bad = await cli(['--request-json'], JSON.stringify(request('int F() {')));
  assert.equal(bad.code, 1); assert.equal(JSON.parse(bad.stdout).status, 'syntax_error');
  const cfg = await cli(['parse', '--input', example, '--view', 'cfg', '--format', 'json']);
  assert.equal(cfg.code, 0);
  assert.deepEqual(JSON.parse(cfg.stdout).diagnostics, []);
  assert.equal(JSON.parse(cfg.stdout).tree.kind, 'method_declaration');
  const cfgDot = await cli(['parse', '--input', example, '--view', 'cfg', '--format', 'dot']);
  assert.equal(cfgDot.code, 0); assert.equal(cfgDot.stderr, '');
  const expectedDot = await readFile(resolve(viewerRoot, '../tests/fixtures/sequence-cfg.dot'), 'utf8');
  assert.equal(cfgDot.stdout.replaceAll('\r\n', '\n'), expectedDot.replaceAll('\r\n', '\n'));
  assert.equal(JSON.parse(cfg.stdout).dot, cfgDot.stdout);
  const cfgSexp = await cli(['parse', '--input', example, '--view', 'cfg', '--format', 'sexp']);
  assert.equal(cfgSexp.code, 2); assert.match(cfgSexp.stderr, /sexp is only available/);
  const ssa = await cli(['parse', '--input', example, '--view', 'ssa', '--format', 'json']);
  assert.equal(ssa.code, 2);
  assert.equal(JSON.parse(ssa.stdout).diagnostics[0].code, 'ssa_not_implemented');
  assert.equal(JSON.parse(ssa.stdout).dot, null);
  const ssaDot = await cli(['parse', '--input', example, '--view', 'ssa', '--format', 'dot']);
  assert.equal(ssaDot.code, 2); assert.equal(ssaDot.stdout, '');
  assert.match(ssaDot.stderr, /ssa_not_implemented/);
});

test('SSA dispatch reaches its scaffold without leaking a CFG and rejects invalid syntax', async () => {
  const result = await (await post({ ...request(), view: 'ssa' })).json();
  assert.equal(result.status, 'error');
  assert.equal(result.diagnostics[0].code, 'ssa_not_implemented');
  assert.equal(result.dot, null);
  assert.deepEqual(result.nodeRanges, {});
  const invalid = await (await post({ ...request('int F() { return 1 }'), view: 'ssa' })).json();
  assert.equal(invalid.status, 'syntax_error');
  assert.equal(invalid.dot, null);
  assert.ok(invalid.diagnostics.some(d => d.code === 'missing_token'));
});
