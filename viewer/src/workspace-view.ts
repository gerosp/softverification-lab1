export function workspaceView(workspace: HTMLElement, divider: HTMLElement, fullscreen: HTMLButtonElement, onFullscreenChange: (active: boolean) => void) {
  let sourceWidth = 35;
  let pointer: number | undefined;
  function setWidth(value: number) {
    sourceWidth = Math.max(15, Math.min(70, value));
    workspace.style.setProperty('--source-width', `${sourceWidth}%`);
    divider.setAttribute('aria-valuenow', String(Math.round(sourceWidth)));
    divider.setAttribute('aria-valuetext', `${Math.round(sourceWidth)}% ширины экрана`);
  }
  function updateFullscreen() {
    const active = document.fullscreenElement === workspace || workspace.classList.contains('expanded');
    fullscreen.setAttribute('aria-pressed', String(active));
    fullscreen.title = active ? 'Выйти из полного экрана' : 'На весь экран';
    fullscreen.setAttribute('aria-label', fullscreen.title);
    onFullscreenChange(active);
  }
  fullscreen.onclick = async () => {
    if (document.fullscreenElement === workspace) await document.exitFullscreen();
    else if (workspace.classList.contains('expanded')) workspace.classList.remove('expanded');
    else {
      try { await workspace.requestFullscreen(); }
      catch { workspace.classList.add('expanded'); }
    }
    updateFullscreen();
  };
  document.addEventListener('fullscreenchange', updateFullscreen);
  document.addEventListener('keydown', event => {
    if (event.key === 'Escape' && workspace.classList.contains('expanded')) {
      workspace.classList.remove('expanded'); updateFullscreen(); fullscreen.focus();
    }
  });
  divider.addEventListener('pointerdown', event => {
    if (event.button !== 0 || pointer !== undefined) return;
    event.preventDefault();
    pointer = event.pointerId;
    divider.setPointerCapture(pointer);
    divider.focus();
    workspace.classList.add('resizing');
  });
  divider.addEventListener('pointermove', event => {
    if (event.pointerId !== pointer) return;
    const bounds = workspace.getBoundingClientRect();
    setWidth((event.clientX - bounds.left - divider.offsetWidth / 2) / bounds.width * 100);
  });
  function stop(event: PointerEvent) {
    if (event.pointerId !== pointer) return;
    pointer = undefined;
    workspace.classList.remove('resizing');
    if (divider.hasPointerCapture(event.pointerId)) divider.releasePointerCapture(event.pointerId);
  }
  divider.addEventListener('pointerup', stop);
  divider.addEventListener('pointercancel', stop);
  divider.addEventListener('lostpointercapture', stop);
  divider.addEventListener('keydown', event => {
    if (event.key === 'ArrowLeft') setWidth(sourceWidth - 2);
    else if (event.key === 'ArrowRight') setWidth(sourceWidth + 2);
    else if (event.key === 'Home') setWidth(15);
    else if (event.key === 'End') setWidth(70);
    else return;
    event.preventDefault();
  });
  setWidth(sourceWidth);
}
