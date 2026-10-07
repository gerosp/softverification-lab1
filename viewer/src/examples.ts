import type { Example } from './types';

export function examplePicker(examples: Example[], select: (example: Example) => void) {
  const dialog = document.querySelector<HTMLDialogElement>('#example-picker')!;
  const trigger = document.querySelector<HTMLButtonElement>('#examples')!;
  const search = document.querySelector<HTMLInputElement>('#example-search')!;
  const tree = document.querySelector<HTMLElement>('#example-tree')!;
  const count = document.querySelector<HTMLElement>('#example-count')!;
  let selected = '';
  const expanded = new Set<string>();

  function choose(example: Example) {
    selected = example.id;
    trigger.textContent = `${example.name} · ${example.original ? 'Исходный' : example.title}`;
    trigger.title = example.description;
    select(example);
    dialog.close();
  }
  function render() {
    const terms = search.value.trim().toLocaleLowerCase().split(/\s+/).filter(Boolean);
    const matches = examples.filter(example => {
      const text = `${example.name} ${example.title} ${example.category} ${example.description} ${example.method ?? ''} ${example.source}`.toLocaleLowerCase();
      return terms.every(term => text.includes(term));
    });
    tree.replaceChildren();
    count.textContent = `${matches.length} из ${examples.length}`;
    if (!matches.length) {
      const empty = document.createElement('p');
      empty.className = 'muted'; empty.textContent = 'Примеры не найдены. Попробуйте другой запрос.';
      tree.append(empty);
    }
    for (const category of new Set(matches.map(example => example.category))) {
      const entries = matches.filter(example => example.category === category);
      const branch = document.createElement('details');
      branch.className = entries[0].original ? 'example-category original-examples' : 'example-category';
      branch.open = terms.length > 0 || expanded.has(category);
      const summary = document.createElement('summary');
      summary.textContent = `${category} (${entries.length})`;
      branch.append(summary);
      branch.addEventListener('toggle', () => {
        if (!terms.length && branch.isConnected) {
          if (branch.open) expanded.add(category); else expanded.delete(category);
        }
      });
      const list = document.createElement('ul');
      for (const example of entries) {
        const item = document.createElement('li');
        const button = document.createElement('button');
        button.dataset.exampleId = example.id;
        button.setAttribute('aria-pressed', String(example.id === selected));
        const name = document.createElement('strong'); name.textContent = `${example.name} · ${example.title}`;
        const description = document.createElement('span'); description.textContent = example.description;
        button.append(name, description);
        button.onclick = () => choose(example);
        item.append(button); list.append(item);
      }
      branch.append(list); tree.append(branch);
    }
  }
  trigger.onclick = () => {
    render(); dialog.showModal(); trigger.setAttribute('aria-expanded', 'true'); search.focus();
  };
  dialog.addEventListener('close', () => { trigger.setAttribute('aria-expanded', 'false'); trigger.focus(); });
  // Search inputs consume Escape to clear their value; close the picker consistently.
  dialog.addEventListener('keydown', event => {
    if (event.key === 'Escape') { event.preventDefault(); event.stopPropagation(); dialog.close(); }
  });
  document.querySelector<HTMLButtonElement>('#close-examples')!.onclick = () => dialog.close();
  search.oninput = render;
  const initial = examples.find(example => example.original) ?? examples[0];
  if (initial) { expanded.add(initial.category); choose(initial); }
}
