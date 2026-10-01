import * as monaco from 'monaco-editor/editor/editor.api.js';
import 'monaco-editor/languages/definitions/csharp/register.js';
import EditorWorker from 'monaco-editor/editor/editor.worker.js?worker';
import { byteToUtf16 } from './positions';
import type { AnalysisResponse, AnalysisView, SourceRange } from './types';
import './style.css';

(globalThis as typeof globalThis & { MonacoEnvironment: object }).MonacoEnvironment = { getWorker: () => new EditorWorker() };
document.querySelector<HTMLDivElement>('#app')!.innerHTML = `
  <header>
    <div class="brand"><span class="logo">C#</span><div><h1>Syntax Lab</h1><p>Исследуйте структуру метода</p></div></div>
    <div class="header-controls">
      <div class="view-control"><label for="view">Модель</label><select id="view" aria-label="Модель">
        <option value="syntax">AST</option><option value="cfg">CFG</option>
      </select></div>
      <div class="example-control"><label for="examples">Пример</label><select id="examples" aria-label="Пример"></select></div>
    </div>
  </header>
  <main>
    <section class="pane source-pane" aria-label="Исходный код">
      <div class="pane-header"><h2>Метод C#</h2><span class="badge">LIVE</span></div>
      <div id="editor"></div>
      <div class="hint">Сигнатура и тело одного метода · изменения применяются автоматически</div>
    </section>
    <section id="graph-pane" class="pane graph-pane" aria-label="Синтаксическое дерево">
      <div class="pane-header"><h2 id="graph-title">Синтаксическое дерево</h2><div class="tools">
        <button id="zoom-out" title="Уменьшить" aria-label="Уменьшить">−</button>
        <button id="fit" title="Вписать">Вписать</button>
        <button id="zoom-in" title="Увеличить" aria-label="Увеличить">+</button>
        <span class="separator"></span><button id="dot" disabled>DOT ↓</button><button id="svg" disabled>SVG ↓</button>
      </div></div>
      <div id="status" role="status" data-state="pending">Загрузка…</div>
      <div id="viewport"><div id="graph"><p class="placeholder">Дерево появится после разбора метода</p></div></div>
      <div class="hint">Нажмите на узел, чтобы выделить соответствующий код</div>
    </section>
  </main>
  <footer><h2>Диагностика <span id="error-count">0</span></h2><ul id="diagnostics"><li class="muted">Ошибок нет</li></ul></footer>`;
const element = <T extends HTMLElement>(id: string) => document.getElementById(id)! as T;
const editor = monaco.editor.create(element('editor'), {
  language: 'csharp', value: '', theme: 'vs', automaticLayout: true,
  fontSize: 14, lineHeight: 23, minimap: { enabled: false }, scrollBeyondLastLine: false,
  padding: { top: 18 }, tabSize: 4, ariaLabel: 'Редактор метода C#',
});
const model = editor.getModel()!;
const graph = element('graph');
const viewport = element('viewport');
const status = element('status');
const viewSelect = element<HTMLSelectElement>('view');
let selectedView: AnalysisView = 'syntax';
let revision = 0;
let timer: ReturnType<typeof setTimeout>;
let abort: AbortController | undefined;
let worker: Worker | undefined;
let renderTimer: ReturnType<typeof setTimeout> | undefined;
let latest: AnalysisResponse | undefined;
let latestSource = '';
let exportedSvg = '';
let originalWidth = 1;
let originalHeight = 1;
let zoom = 1;

function setStatus(text: string, state: string) { status.textContent = text; status.dataset.state = state; }
function disableExport() {
  element<HTMLButtonElement>('dot').disabled = true;
  element<HTMLButtonElement>('svg').disabled = true;
}
function fail(message: string) {
  setStatus(`${message}${graph.querySelector('svg') ? ' · Показан устаревший граф' : ''}`, 'error');
  graph.classList.add('stale');
  disableExport();
}
function emptyGraph(message: string) {
  graph.replaceChildren(); graph.classList.remove('stale');
  const placeholder = document.createElement('p');
  placeholder.className = 'placeholder'; placeholder.textContent = message;
  graph.append(placeholder);
  exportedSvg = ''; disableExport();
}
function rangeFor(source: string, r: SourceRange): monaco.IRange {
  const start = model.getPositionAt(byteToUtf16(source, r.start));
  const end = model.getPositionAt(byteToUtf16(source, r.end));
  return { startLineNumber: start.lineNumber, startColumn: start.column, endLineNumber: end.lineNumber, endColumn: end.column };
}
function selectRange(r: SourceRange) {
  if (!latest || latest.requestId !== revision) return;
  const range = rangeFor(latestSource, r);
  editor.setSelection(range); editor.revealRangeInCenter(range); editor.focus();
}
function showDiagnostics(response: AnalysisResponse, source: string) {
  const list = element('diagnostics'); list.replaceChildren();
  element('error-count').textContent = String(response.diagnostics.length);
  monaco.editor.setModelMarkers(model, 'lab', response.diagnostics.map(d => ({
    ...rangeFor(source, d.range), message: d.message, severity: monaco.MarkerSeverity.Error,
  })));
  for (const d of response.diagnostics) {
    const item = document.createElement('li');
    const button = document.createElement('button');
    const position = model.getPositionAt(byteToUtf16(source, d.range.start));
    button.textContent = `${position.lineNumber}:${position.column} · ${d.message}`;
    button.onclick = () => selectRange(d.range);
    item.append(button); list.append(item);
  }
  if (!response.diagnostics.length) { const li = document.createElement('li'); li.className = 'muted'; li.textContent = 'Ошибок нет'; list.append(li); }
}
function applyZoom() {
  const svg = graph.querySelector('svg');
  if (svg) { svg.style.width = `${originalWidth * zoom}px`; svg.style.height = `${originalHeight * zoom}px`; }
}
function fit() {
  zoom = Math.min(1, Math.max(0.02, Math.min((viewport.clientWidth - 40) / originalWidth, (viewport.clientHeight - 40) / originalHeight)));
  applyZoom();
}
function render(response: AnalysisResponse) {
  if (!response.dot) {
    emptyGraph(selectedView === 'cfg' ? 'CFG доступен после исправления синтаксиса' : 'Нет дерева для отображения');
    setStatus('Нет графа для отображения · исправьте код', 'error');
    return;
  }
  // A worker per accepted result makes superseded/slow Graphviz work cancellable.
  worker = new Worker(new URL('./graph.worker.ts', import.meta.url), { type: 'module' });
  const currentWorker = worker;
  renderTimer = setTimeout(() => {
    currentWorker.terminate();
    if (revision === response.requestId) fail('Graphviz превысил время ожидания');
  }, 15000);
  worker.onerror = () => { clearTimeout(renderTimer); currentWorker.terminate(); if (revision === response.requestId) fail('Не удалось запустить Graphviz'); };
  worker.onmessage = (event: MessageEvent<{ requestId: number; svg?: string; error?: string }>) => {
    if (event.data.requestId !== revision) return;
    clearTimeout(renderTimer); currentWorker.terminate(); worker = undefined;
    if (event.data.error || !event.data.svg) { fail(event.data.error ?? 'Ошибка Graphviz'); return; }
    exportedSvg = event.data.svg;
    const documentSvg = new DOMParser().parseFromString(exportedSvg, 'image/svg+xml');
    const svg = documentSvg.documentElement;
    if (svg.localName !== 'svg') { fail('Некорректный SVG'); return; }
    graph.replaceChildren(document.importNode(svg, true));
    const mounted = graph.querySelector('svg')!;
    originalWidth = mounted.viewBox.baseVal.width;
    originalHeight = mounted.viewBox.baseVal.height;
    graph.classList.remove('stale');
    fit();
    element<HTMLButtonElement>('dot').disabled = false;
    element<HTMLButtonElement>('svg').disabled = false;
    // Synthetic CFG entry/exit nodes may have no source range, but still count.
    setStatus(response.status === 'ok' ? `${graph.querySelectorAll('g.node').length} узлов · актуально` : 'Частичное дерево · есть ошибки синтаксиса', response.status === 'ok' ? 'ok' : 'error');
  };
  worker.postMessage({ requestId: response.requestId, dot: response.dot });
}
async function analyze(requestId: number) {
  const source = model.getValue(monaco.editor.EndOfLinePreference.TextDefined);
  const view = selectedView;
  latestSource = source;
  const controller = new AbortController(); abort = controller;
  setStatus('Разбор метода…', 'pending');
  try {
    const http = await fetch('/api/analyze', { method: 'POST', headers: { 'Content-Type': 'application/json' }, signal: controller.signal,
      body: JSON.stringify({ protocolVersion: 1, requestId, source, view }) });
    const body = await http.json();
    if (requestId !== revision) return;
    if (!http.ok) throw new Error(body.error ?? `HTTP ${http.status}`);
    const response = body as AnalysisResponse;
    if (response.requestId !== requestId) throw new Error('Несовпадение номера запроса');
    latest = response;
    showDiagnostics(response, source);
    if (response.status === 'error') {
      const message = response.diagnostics.some(d => d.code === 'cfg_not_implemented')
        ? 'Построение CFG пока не реализовано'
        : response.diagnostics[0]?.message ?? 'Ошибка анализатора';
      emptyGraph(message); setStatus(message, 'error'); return;
    }
    setStatus('Построение изображения…', 'pending');
    render(response);
  } catch (error) {
    if (controller.signal.aborted || requestId !== revision) return;
    fail(error instanceof Error ? error.message : String(error));
  }
}
function schedule(delay = 300) {
  revision++;
  abort?.abort(); worker?.terminate(); worker = undefined;
  clearTimeout(timer); clearTimeout(renderTimer);
  graph.classList.add('stale'); disableExport();
  monaco.editor.setModelMarkers(model, 'lab', []);
  setStatus(graph.querySelector('svg') ? 'Код изменён · показан устаревший граф' : 'Ожидание ввода…', 'pending');
  const id = revision;
  timer = setTimeout(() => void analyze(id), delay);
}
editor.onDidChangeModelContent(() => schedule());
viewSelect.onchange = () => {
  selectedView = viewSelect.value as AnalysisView;
  const title = selectedView === 'syntax' ? 'Синтаксическое дерево' : 'Граф потока управления';
  element('graph-title').textContent = title;
  element('graph-pane').setAttribute('aria-label', title);
  latest = undefined;
  emptyGraph('Загрузка выбранной модели…');
  element('diagnostics').replaceChildren(); element('error-count').textContent = '0';
  // Reuse the revision/abort pipeline so an old AST cannot overwrite a CFG view.
  schedule(0);
};
graph.addEventListener('click', event => {
  if (graph.classList.contains('stale')) return;
  const node = (event.target as Element).closest('g.node');
  if (node && latest?.nodeRanges[node.id]) selectRange(latest.nodeRanges[node.id]);
});
element('zoom-in').onclick = () => { zoom = Math.min(4, zoom * 1.3); applyZoom(); };
element('zoom-out').onclick = () => { zoom = Math.max(0.02, zoom / 1.3); applyZoom(); };
element('fit').onclick = fit;
function download(name: string, text: string, type: string) {
  const url = URL.createObjectURL(new Blob([text], { type }));
  const link = document.createElement('a'); link.href = url; link.download = name; link.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}
element('dot').onclick = () => { if (latest?.dot) download(`${selectedView}.dot`, latest.dot, 'text/vnd.graphviz;charset=utf-8'); };
element('svg').onclick = () => download(`${selectedView}.svg`, exportedSvg, 'image/svg+xml;charset=utf-8');
async function init() {
  const response = await fetch('/api/examples');
  if (!response.ok) throw new Error('Не удалось загрузить примеры');
  const examples: { name: string; source: string }[] = await response.json();
  const select = element<HTMLSelectElement>('examples');
  for (const example of examples) { const option = document.createElement('option'); option.value = example.name; option.textContent = example.name; select.append(option); }
  select.onchange = () => { const example = examples.find(e => e.name === select.value); if (example) editor.setValue(example.source); };
  if (examples[0]) editor.setValue(examples[0].source);
  else schedule();
}
void init().catch(error => fail(String(error)));
