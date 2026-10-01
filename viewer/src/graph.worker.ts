import { instance } from '@viz-js/viz';
const viz = instance();
self.onmessage = async (event: MessageEvent<{ requestId: number; dot: string }>) => {
  const { requestId, dot } = event.data;
  try {
    const renderer = await viz;
    const svg = renderer.renderString(dot, { format: 'svg', engine: 'dot' });
    self.postMessage({ requestId, svg });
  } catch (error) {
    self.postMessage({ requestId, error: error instanceof Error ? error.message : String(error) });
  }
};
