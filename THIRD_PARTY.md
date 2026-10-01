# Сторонние компоненты

Исходники зависимостей не копируются в исходный код проекта: CMake FetchContent
и npm загружают фиксированные версии. При распространении собранного приложения
сохраняйте уведомления о лицензиях используемых компонентов и их транзитивных
зависимостей.

| Компонент | Исходники / лицензия |
|---|---|
| Tree-sitter 0.25.10 | https://github.com/tree-sitter/tree-sitter/tree/v0.25.10 — `LICENSE` |
| tree-sitter-c-sharp 0.23.1 | https://github.com/tree-sitter/tree-sitter-c-sharp/tree/v0.23.1 — `LICENSE` |
| nlohmann/json 3.12.0 | https://github.com/nlohmann/json/tree/v3.12.0 — `LICENSE.MIT` |
| Monaco Editor | https://github.com/microsoft/monaco-editor — лицензия в npm-пакете |
| Viz.js / Graphviz WASM | https://github.com/mdaines/viz-js — лицензии JS-обёртки и встроенного Graphviz |
| Graphviz | https://graphviz.org/license/ |
| LLVM-MinGW | https://github.com/mstorsjo/llvm-mingw — уведомления в архиве инструментов |
| Ninja | https://github.com/ninja-build/ninja — `COPYING` |

Версии инструментов viewer и тестирования закреплены в `viewer/package-lock.json`.
