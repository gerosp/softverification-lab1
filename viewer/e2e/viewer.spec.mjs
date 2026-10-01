import { test, expect } from '@playwright/test';
import { readFile } from 'node:fs/promises';

async function replaceSource(page, source) {
  await page.locator('.monaco-editor').click({ position: { x: 180, y: 50 } });
  await page.keyboard.press('ControlOrMeta+A');
  await page.keyboard.insertText(source);
}
async function ready(page) {
  await expect(page.locator('#status')).toHaveAttribute('data-state', 'ok', { timeout: 30000 });
  await expect(page.locator('#graph svg')).toBeVisible();
}
test('examples, live editing, graph selection, zoom and exports', async ({ page }) => {
  const errors = [];
  page.on('pageerror', error => errors.push(error.message));
  await page.goto('/'); await ready(page);
  await expect(page.locator('#examples option')).toHaveCount(7);
  await page.selectOption('#examples', '05-for.cs');
  await expect(page.locator('#graph')).toHaveClass(/stale/);
  await ready(page);
  await expect(page.locator('#graph g.node text').filter({ hasText: /^for_statement$/ })).toHaveCount(1);
  await page.screenshot({ path: 'test-results/viewer.png', fullPage: true });
  const before = await page.locator('#graph svg').evaluate(el => el.style.width);
  await page.getByRole('button', { name: 'Увеличить', exact: true }).click();
  expect(await page.locator('#graph svg').evaluate(el => el.style.width)).not.toBe(before);
  const identifier = page.locator('#graph g.node').filter({ hasText: 'SumEven' });
  await identifier.click();
  await expect(page.locator('.monaco-editor .selected-text').first()).toBeVisible();
  const dotDownload = page.waitForEvent('download');
  await page.getByRole('button', { name: 'DOT ↓' }).click();
  expect(await readFile(await (await dotDownload).path(), 'utf8')).toContain('digraph "Syntax"');
  const svgDownload = page.waitForEvent('download');
  await page.getByRole('button', { name: 'SVG ↓' }).click();
  expect(await readFile(await (await svgDownload).path(), 'utf8')).toContain('<svg');
  await replaceSource(page, 'int Edited(int x) { return x * 2; }');
  await ready(page);
  await expect(page.locator('#graph')).toContainText('Edited');
  await replaceSource(page, 'int Edited(int x) { return x * 2 }');
  await expect(page.locator('#status')).toContainText('Частичное дерево', { timeout: 15000 });
  await expect(page.locator('#error-count')).not.toHaveText('0');
  await replaceSource(page, 'int Fixed() { return 1; }');
  await ready(page);
  await expect(page.locator('#error-count')).toHaveText('0');
  expect(errors).toEqual([]);
});

test('a delayed obsolete response cannot replace the latest graph', async ({ page }) => {
  await page.goto('/'); await ready(page);
  let oldSeen;
  const seen = new Promise(resolve => { oldSeen = resolve; });
  await page.route('**/api/analyze', async route => {
    const body = route.request().postDataJSON();
    if (body.source.includes('OldResult')) {
      oldSeen();
      const response = await route.fetch();
      await new Promise(resolve => setTimeout(resolve, 1500));
      await route.fulfill({ response }).catch(() => {});
    } else await route.continue();
  });
  await replaceSource(page, 'int OldResult() { return 1; }');
  await seen;
  await replaceSource(page, 'int NewResult() { return 2; }');
  await ready(page);
  await expect(page.locator('#graph')).toContainText('NewResult');
  await page.waitForTimeout(1800);
  await expect(page.locator('#graph')).not.toContainText('OldResult');
});

test('backend failure marks the previous graph stale and disables export', async ({ page }) => {
  await page.goto('/'); await ready(page);
  await page.route('**/api/analyze', route => route.fulfill({ status: 502, contentType: 'application/json', body: JSON.stringify({ error: 'Analyzer unavailable' }) }));
  await replaceSource(page, 'int F() { return 3; }');
  await expect(page.locator('#status')).toContainText('устаревший граф');
  await expect(page.getByRole('button', { name: 'DOT ↓' })).toBeDisabled();
  await expect(page.locator('#graph')).toHaveClass(/stale/);
});

test('switching AST/CFG reaches the pass without displaying AST as CFG', async ({ page }) => {
  await page.goto('/'); await ready(page);
  const cfgRequest = page.waitForRequest(req => req.url().endsWith('/api/analyze') && req.postDataJSON()?.view === 'cfg');
  await page.selectOption('#view', 'cfg');
  await cfgRequest;
  await expect(page.locator('#graph-title')).toHaveText('Граф потока управления');
  await expect(page.locator('#status')).toContainText('Построение CFG пока не реализовано');
  await expect(page.locator('#graph svg')).toHaveCount(0);
  await expect(page.getByRole('button', { name: 'DOT ↓' })).toBeDisabled();
  await replaceSource(page, 'int F() { return 1 }');
  await expect(page.locator('#diagnostics')).toContainText('Expected ;');
  await expect(page.locator('#graph svg')).toHaveCount(0);
  await page.selectOption('#view', 'syntax');
  await expect(page.locator('#status')).toContainText('Частичное дерево');
  await expect(page.locator('#graph svg')).toBeVisible();
  await replaceSource(page, 'int F() { return 1; }'); await ready(page);
});

test('a CFG response uses shared rendering, source selection and CFG export names', async ({ page }) => {
  await page.goto('/'); await ready(page);
  // Test-only fixture: exercise the success path before the user implements CFG.
  await page.route('**/api/analyze', async route => {
    const body = route.request().postDataJSON();
    if (body.view !== 'cfg') return route.continue();
    const start = Buffer.byteLength(body.source.slice(0, body.source.indexOf('return')), 'utf8');
    await route.fulfill({ contentType: 'application/json', body: JSON.stringify({
      protocolVersion: 1, requestId: body.requestId, status: 'ok', diagnostics: [],
      dot: 'digraph "CFG" { entry [id="entry", label="Entry"]; b0 [id="b0", label="return z;"]; exit [id="exit", label="Exit"]; entry -> b0; b0 -> exit; }',
      nodeRanges: { b0: { start, end: start + 9 } },
    }) });
  });
  await page.selectOption('#view', 'cfg'); await ready(page);
  await expect(page.locator('#status')).toContainText('3 узлов');
  await page.locator('#graph #b0').click();
  await expect(page.locator('.monaco-editor .selected-text').first()).toBeVisible();
  const dotDownload = page.waitForEvent('download');
  await page.getByRole('button', { name: 'DOT ↓' }).click();
  const dot = await dotDownload;
  expect(dot.suggestedFilename()).toBe('cfg.dot');
  expect(await readFile(await dot.path(), 'utf8')).toContain('digraph "CFG"');
  const svgDownload = page.waitForEvent('download');
  await page.getByRole('button', { name: 'SVG ↓' }).click();
  expect((await svgDownload).suggestedFilename()).toBe('cfg.svg');
});

test('late AST responses cannot overwrite the selected CFG view', async ({ page }) => {
  await page.goto('/'); await ready(page);
  let signalOldRequest;
  const seen = new Promise(resolve => { signalOldRequest = resolve; });
  await page.route('**/api/analyze', async route => {
    if (route.request().postDataJSON().view !== 'syntax') return route.continue();
    const response = await route.fetch();
    signalOldRequest();
    await new Promise(resolve => setTimeout(resolve, 1200));
    await route.fulfill({ response }).catch(() => {});
  });
  await replaceSource(page, 'int OldAst() { return 1; }'); await seen;
  await page.selectOption('#view', 'cfg');
  await expect(page.locator('#status')).toContainText('Построение CFG пока не реализовано');
  await page.waitForTimeout(1500);
  await expect(page.locator('#graph svg')).toHaveCount(0);
  await expect(page.locator('#graph')).not.toContainText('OldAst');
});
