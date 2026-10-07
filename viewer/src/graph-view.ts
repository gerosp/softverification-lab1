// Presentation only: SVG coordinates are independent of the graph model.
export function graphView(viewport: HTMLElement, graph: HTMLElement) {
  let width = 1;
  let height = 1;
  let zoom = 1;
  let x = 0;
  let y = 0;
  let drag: { id: number; x: number; y: number; originX: number; originY: number; moved: boolean } | undefined;
  let suppressClick = false;
  const svg = () => graph.querySelector('svg');
  function apply() {
    const mounted = svg();
    if (!mounted) return;
    mounted.style.width = `${width * zoom}px`;
    mounted.style.height = `${height * zoom}px`;
    mounted.style.transform = `translate(${x}px, ${y}px)`;
  }
  function fit() {
    zoom = Math.min(1, Math.max(0.02, Math.min((viewport.clientWidth - 40) / width, (viewport.clientHeight - 40) / height)));
    x = (viewport.clientWidth - width * zoom) / 2;
    y = (viewport.clientHeight - height * zoom) / 2;
    apply();
  }
  function zoomAt(factor: number, anchorX = viewport.clientWidth / 2, anchorY = viewport.clientHeight / 2) {
    if (!svg()) return;
    const next = Math.min(4, Math.max(0.02, zoom * factor));
    x = anchorX - (anchorX - x) * next / zoom;
    y = anchorY - (anchorY - y) * next / zoom;
    zoom = next;
    apply();
  }
  viewport.addEventListener('wheel', event => {
    if (!svg()) return;
    event.preventDefault();
    const bounds = viewport.getBoundingClientRect();
    const delta = event.deltaY * (event.deltaMode === 1 ? 16 : event.deltaMode === 2 ? viewport.clientHeight : 1);
    zoomAt(Math.exp(-Math.max(-300, Math.min(300, delta)) * 0.002), event.clientX - bounds.left, event.clientY - bounds.top);
  }, { passive: false });
  viewport.addEventListener('pointerdown', event => {
    if (!svg() || event.button !== 0 || drag) return;
    suppressClick = false;
    drag = { id: event.pointerId, x: event.clientX, y: event.clientY, originX: x, originY: y, moved: false };
  });
  viewport.addEventListener('pointermove', event => {
    if (!drag || event.pointerId !== drag.id) return;
    const dx = event.clientX - drag.x;
    const dy = event.clientY - drag.y;
    if (!drag.moved && Math.hypot(dx, dy) < 4) return;
    drag.moved = true;
    viewport.setPointerCapture(event.pointerId);
    viewport.classList.add('panning');
    x = drag.originX + dx; y = drag.originY + dy;
    apply();
  });
  function stop(event: PointerEvent) {
    if (!drag || event.pointerId !== drag.id) return;
    suppressClick = drag.moved;
    drag = undefined;
    viewport.classList.remove('panning');
    if (viewport.hasPointerCapture(event.pointerId)) viewport.releasePointerCapture(event.pointerId);
  }
  viewport.addEventListener('pointerup', stop);
  viewport.addEventListener('pointercancel', stop);
  viewport.addEventListener('lostpointercapture', stop);
  viewport.addEventListener('pointerleave', event => { if (!drag?.moved) stop(event); });
  viewport.addEventListener('click', event => {
    if (suppressClick) { event.stopImmediatePropagation(); event.preventDefault(); suppressClick = false; }
  }, true);
  new ResizeObserver(() => fit()).observe(viewport);
  return {
    fit, zoomAt,
    mount() {
      const mounted = svg();
      if (!mounted) return;
      width = mounted.viewBox.baseVal.width || 1;
      height = mounted.viewBox.baseVal.height || 1;
      fit();
    },
  };
}
