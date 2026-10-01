import { defineConfig } from '@playwright/test';
export default defineConfig({
  testDir: './e2e', timeout: 45000, workers: 1,
  use: { baseURL: 'http://127.0.0.1:3001', headless: true, viewport: { width: 1440, height: 1000 },
    ...(process.env.PLAYWRIGHT_CHANNEL ? { channel: process.env.PLAYWRIGHT_CHANNEL } : {}) },
  webServer: { command: `node server.mjs --port 3001${process.env.LAB_VIEWER_PRODUCTION === '1' ? ' --production' : ''}`, url: 'http://127.0.0.1:3001', reuseExistingServer: false, timeout: 60000 },
});
