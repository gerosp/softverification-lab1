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
  await page.getByRole('button', { name: 'Выбрать пример', exact: true }).click();
  await expect(page.locator('[data-example-id]')).toHaveCount(57);
  await page.locator('[data-example-id="05-for.cs"]').click();
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
  await page.getByRole('button', { name: 'CFG', exact: true }).click();
  await cfgRequest;
  await ready(page);
  await expect(page.locator('#graph-pane')).toHaveAttribute('aria-label', 'Граф потока управления');
  await expect(page.getByRole('button', { name: 'CFG', exact: true })).toHaveAttribute('aria-pressed', 'true');
  await expect(page.locator('#graph')).toContainText('return z;');
  await expect(page.locator('#graph')).not.toContainText('method_declaration');
  await expect(page.getByRole('button', { name: 'DOT ↓' })).toBeEnabled();
  await replaceSource(page, 'int F() { return 1 }');
  await expect(page.locator('#diagnostics')).toContainText('Expected ;');
  await expect(page.locator('#graph svg')).toHaveCount(0);
  await page.getByRole('button', { name: 'AST', exact: true }).click();
  await expect(page.locator('#status')).toContainText('Частичное дерево');
  await expect(page.locator('#graph svg')).toBeVisible();
  await replaceSource(page, 'int F() { return 1; }'); await ready(page);
});

test('a CFG response uses shared rendering, source selection and CFG export names', async ({ page }) => {
  await page.goto('/'); await ready(page);
  await page.getByRole('button', { name: 'CFG', exact: true }).click(); await ready(page);
  await expect(page.locator('#status')).toContainText('6 узлов');
  await page.locator('#graph g.node').filter({ hasText: 'return z;' }).click();
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
  await page.getByRole('button', { name: 'CFG', exact: true }).click();
  await ready(page);
  await page.waitForTimeout(1500);
  await expect(page.locator('#graph svg')).toBeVisible();
  await expect(page.locator('#graph')).toContainText('return 1;');
  await expect(page.locator('#graph')).not.toContainText('OldAst');
});

test('category picker searches all examples and distinguishes duplicate filenames', async ({ page }) => {
  await page.goto('/'); await ready(page);
  const trigger = page.getByRole('button', { name: 'Выбрать пример', exact: true });
  await trigger.click();
  await expect(page.locator('#example-count')).toHaveText('57 из 57');
  await expect(page.locator('.original-examples [data-example-id]')).toHaveCount(7);
  await page.locator('#example-search').fill('01-sequence');
  await expect(page.locator('[data-example-id]')).toHaveCount(2);
  await page.locator('[data-example-id="expanded/01-sequence.cs"]').click();
  await ready(page);
  await expect(page.locator('#graph')).toContainText('AddOffset');
  await trigger.click();
  await page.locator('[data-example-id="01-sequence.cs"]').click();
  await ready(page);
  await expect(page.locator('#graph')).toContainText('Compute');
  await trigger.click();
  await page.locator('#example-search').fill('несуществующий пример');
  await expect(page.locator('#example-count')).toHaveText('0 из 57');
  await expect(page.locator('#example-tree')).toContainText('Примеры не найдены');
  await page.locator('#example-search').fill('continue');
  await expect(page.locator('[data-example-id="expanded/30-for-continue.cs"]')).toBeVisible();
  await page.keyboard.press('Escape');
  await expect(page.locator('#example-picker')).not.toBeVisible();
  await expect(trigger).toBeFocused();
});

test('wheel zoom anchors at the cursor, dragging pans without selecting, and fit resets', async ({ page }) => {
  await page.goto('/'); await ready(page);
  await page.getByRole('button', { name: 'CFG', exact: true }).click(); await ready(page);
  const svg = page.locator('#graph svg');
  const before = await svg.boundingBox();
  const box = await page.locator('#viewport').boundingBox();
  const anchor = { x: box.x + box.width * 0.6, y: box.y + box.height * 0.4 };
  await page.mouse.move(anchor.x, anchor.y);
  await page.mouse.wheel(0, -200);
  await expect.poll(async () => (await svg.boundingBox()).width).toBeGreaterThan(before.width);
  const after = await svg.boundingBox();
  expect((anchor.x - after.x) / after.width).toBeCloseTo((anchor.x - before.x) / before.width, 2);
  expect((anchor.y - after.y) / after.height).toBeCloseTo((anchor.y - before.y) / before.height, 2);
  await page.getByRole('button', { name: 'Вписать', exact: true }).click();
  const fitted = await svg.boundingBox();
  const node = await page.locator('#graph g.node').filter({ hasText: 'return z;' }).boundingBox();
  await page.mouse.move(node.x + node.width / 2, node.y + node.height / 2);
  await page.mouse.down(); await page.mouse.move(node.x + node.width / 2 + 65, node.y + node.height / 2 - 45, { steps: 8 }); await page.mouse.up();
  const moved = await svg.boundingBox();
  expect(moved.x - fitted.x).toBeCloseTo(65, 0);
  expect(moved.y - fitted.y).toBeCloseTo(-45, 0);
  await expect(page.locator('.monaco-editor .selected-text')).toHaveCount(0);
  await page.getByRole('button', { name: 'Вписать', exact: true }).click();
  expect((await svg.boundingBox()).x).toBeCloseTo(fitted.x, 0);
  await page.locator('#graph g.node').filter({ hasText: 'return z;' }).click();
  await expect(page.locator('.monaco-editor .selected-text').first()).toBeVisible();
});

test('fullscreen expands the graph and exits, including the browser fallback', async ({ page }) => {
  await page.goto('/'); await ready(page);
  await page.getByRole('button', { name: 'На весь экран', exact: true }).click();
  await expect(page.locator('#fullscreen')).toHaveAttribute('aria-pressed', 'true');
  await expect.poll(async () => (await page.locator('#graph-pane').boundingBox()).width).toBe(1440);
  await page.getByRole('button', { name: 'Выйти из полного экрана' }).click();
  await expect(page.locator('#fullscreen')).toHaveAttribute('aria-pressed', 'false');
  await page.evaluate(() => { document.getElementById('graph-pane').requestFullscreen = () => Promise.reject(new Error('Unavailable')); });
  await page.getByRole('button', { name: 'На весь экран', exact: true }).click();
  await expect(page.locator('#graph-pane')).toHaveClass(/expanded/);
  await page.keyboard.press('Escape');
  await expect(page.locator('#graph-pane')).not.toHaveClass(/expanded/);
  await expect(page.locator('#fullscreen')).toHaveAttribute('aria-pressed', 'false');
});

test('refresh retries unchanged source after backend failure and SSA reports its scaffold', async ({ page }) => {
  await page.goto('/'); await ready(page);
  await page.getByRole('button', { name: 'CFG', exact: true }).click(); await ready(page);
  await page.route('**/api/analyze', route => route.fulfill({ status: 502, contentType: 'application/json', body: JSON.stringify({ error: 'Analyzer unavailable' }) }));
  await page.getByRole('button', { name: 'Обновить граф' }).click();
  await expect(page.locator('#status')).toContainText('Analyzer unavailable');
  await expect(page.locator('#dot')).toBeDisabled();
  await page.unroute('**/api/analyze');
  const refreshed = page.waitForRequest(req => req.url().endsWith('/api/analyze') && req.postDataJSON()?.view === 'cfg');
  await page.getByRole('button', { name: 'Обновить граф' }).click();
  expect((await refreshed).postDataJSON().source).toContain('int Compute');
  await ready(page);
  await page.getByRole('button', { name: 'SSA', exact: true }).click();
  await expect(page.locator('#status')).toContainText('Построение SSA пока не реализовано');
  await expect(page.locator('#graph svg')).toHaveCount(0);
  await expect(page.locator('#dot')).toBeDisabled();
  await page.getByRole('button', { name: 'AST', exact: true }).click(); await ready(page);
});
