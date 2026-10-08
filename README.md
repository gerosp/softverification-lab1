# C# Syntax Lab

Основа лабораторной работы по CFG/SSA: разбор **одного метода C#** из C++,
доступ к синтаксическому дереву, экспорт JSON/S-expression/DOT и локальный
редактор с live-визуализацией через Graphviz.

**CFG доступен; SSA пока содержит только заготовку с диагностикой.** Tree-sitter разбирает синтаксис, но не
проверяет типы, разрешение имён, определённость переменных или корректность
программы по всем правилам C#. Входной C# не компилируется и не исполняется.

## Быстрый запуск на Windows

Нужны Windows 10/11 x64, CMake ≥ 3.24 и Node.js ≥ 22.13 (рекомендуется 24).
Команды выполняются в PowerShell из корня проекта:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/bootstrap-windows.ps1
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug

cd viewer
npm.cmd ci
npm.cmd run dev
```

Открыть **http://127.0.0.1:3000**. Слева — метод C#, справа — дерево.
Изменения обрабатываются после паузы 300 мс. Нажатие на узел выделяет код;
кнопки DOT/SVG сохраняют текущий результат. Ошибки показываются под редактором.

В заголовке графа переключаются CFG / AST / SSA. Колесо мыши масштабирует граф
относительно курсора; перетаскивание перемещает его, «Вписать» сбрасывает вид.
Кнопка ⛶ разворачивает редактор и граф рядом на весь экран. Разделитель можно
перетаскивать по горизонтали; после Tab он управляется стрелками ←/→ и Home/End.
Выбор примера в этом режиме находится справа от «Метод C#» и открывается поверх обеих панелей.
Кнопка ↻ повторно анализирует текущий
код: после пересборки backend достаточно нажать её, перезапуск viewer не нужен.
Кнопка выбора примера открывает поиск и категории для 57 методов; семь исходных
примеров находятся в отдельной ветке. Поиск учитывает название, описание и код.

Bootstrap распаковывает LLVM-MinGW UCRT x86_64 и Ninja в `.tools`, проверяя
SHA-256 архивов. Права администратора, Visual Studio, изменение глобального
`PATH` и Cygwin не нужны. Preset указывает компиляторы явно. CMake уже должен
быть доступен в `PATH`; загрузка инструментов занимает около 182 MiB.

## Linux

Например, Ubuntu 24.04, с уже установленным Node.js 24:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug

cd viewer
npm ci
npm run dev
```

C++-ядру Node.js не нужен. Для Clang при первой конфигурации можно задать
`CC=clang CXX=clang++ cmake --preset linux-debug`. Не менять компилятор в уже
сконфигурированном каталоге сборки.

Для WSL предпочтительно запускать сервер Node.js и Linux CLI в одном
дистрибутиве. Windows Node.js по умолчанию запускает Windows `.exe`.

В текущем рабочем окружении DNS внутри WSL недоступен, поэтому Linux-сборка
проверена с portable CMake/Ninja из `.tools/linux` и уже скачанными исходниками.
Для повторной сборки здесь достаточно `sh scripts/build-linux.sh` из Ubuntu
в папке проекта. Скрипт предпочитает системные инструменты, если они установлены;
дополнительные аргументы передаются CMake configure. На другой машине используйте
обычную установку выше либо подготовьте те же portable-инструменты.

## CLI

Из корня проекта (в Linux заменить путь к `.exe` на `./build/linux-debug/lab-model`):

```powershell
.\build\windows-debug\lab-model.exe parse --input examples/05-for.cs --format json
.\build\windows-debug\lab-model.exe parse --input examples/05-for.cs --format dot
.\build\windows-debug\lab-model.exe parse --input examples/05-for.cs --format sexp
```

`--input -` читает UTF-8 из stdin. Windows CLI переключает stdin/stdout в
бинарный режим для сохранения CRLF и байтовых смещений. В Windows PowerShell
5 перенаправление `>` может перекодировать вывод в UTF-16: для сохранения
UTF-8 DOT используйте экспорт из viewer либо явную UTF-8 запись. В Linux:

```sh
./build/linux-debug/lab-model parse --input examples/05-for.cs --format dot > syntax.dot
# Необязательная внешняя визуализация при установленном Graphviz:
dot -Tsvg syntax.dot -o syntax.svg
```

Вход — сигнатура и блочное тело **ровно одного метода**, без класса:

```csharp
int Increment(int x)
{
    return x + 1;
}
```

Комментарии допустимы. Служебный класс скрыт в выводе; диапазоны относятся
к исходному пользовательскому тексту. Короткая запись `int F() => 1;`
диагностируется как неподдерживаемая форма входа.

JSON-export содержит дерево со всеми токенами и полями. DOT/viewer показывают
именованные узлы и значимые операторы. S-expression удобен для отладки.
Частичное дерево при ошибке не следует использовать как корректную программу.

Коды завершения: `0` — успешный разбор; `1` — ошибки исходного кода;
`2` — ошибка вызова, протокола или выполнения. stdout содержит только результат;
для DOT/S-expression диагностики выводятся в stderr, для JSON — в ответ.

## Viewer и проверки

Из `viewer`:

```sh
npm run dev -- --analyzer /absolute/path/to/lab-model --port 3000
npm run build
npm start
npm test
```

`npm start` обслуживает собранный `dist` и тот же API. `--analyzer` доступен и
для него. Сервер слушает только `127.0.0.1`; C# передаётся процессу через stdin,
без shell и временных файлов. Необходим только локальный браузер: Graphviz
включён в npm-зависимости как WebAssembly. После установки зависимостей
редактирование и визуализация работают без интернета.

Браузерные тесты:

```sh
npx playwright install chromium
npm run test:e2e
```

Либо использовать установленный Chrome без загрузки Chromium:

```powershell
$env:PLAYWRIGHT_CHANNEL = 'chrome'
npm.cmd run test:e2e
```

Для проверки собранного `dist` теми же тестами задайте
`LAB_VIEWER_PRODUCTION=1` после `npm run build`.

`npm test` запускает реальный CLI и Graphviz; предварительно соберите C++.
Для другого бинарника задайте `LAB_ANALYZER`. CTest проверяет парсер, поля,
операторы, примеры, диагностики, Unicode/CRLF и протокол. Playwright проверяет
редактирование, выбор узлов, экспорт, устаревшие ответы и ошибки сервера.
Снимок интерфейса сохраняется в `viewer/test-results/viewer.png`.

## Где продолжать разработку

- `include/lab` и `src`: API разбора, представление графа, сериализация, CLI.
- `examples`: семь исходных методов; `examples/expanded` — 50 дополнительных методов и `cases.json` с описаниями и контрольными вызовами.
- `include/lab/SSA.hpp`, `src/SSA.cpp`: заготовка `build_ssa(method, cfg)`, возвращающая `ssa_not_implemented`. Диспетчер сначала получает CFG; алгоритм SSA остаётся заданием студента.
- `viewer`: редактор, Graphviz worker и Node.js-мост.
- [Архитектура и контракт](docs/architecture.md): владение деревом, координаты,
  протокол и граница между будущими CFG/SSA и графом для отображения.

Первое конфигурирование CMake скачивает зафиксированные исходники. Повторная
сборка использует `build/<preset>/_deps`. Для полностью автономной первичной
сборки можно передать распакованные исходники через стандартные переменные
`FETCHCONTENT_SOURCE_DIR_TREE_SITTER`, `FETCHCONTENT_SOURCE_DIR_CSHARP` и
`FETCHCONTENT_SOURCE_DIR_JSON`.

Выбраны Tree-sitter 0.25.10, tree-sitter-c-sharp 0.23.1 и nlohmann/json 3.12.0.
Их версии и хеши находятся в `cmake/Dependencies.cmake`, версии viewer —
в `package.json` и `package-lock.json`. Сборка использует готовый C-парсер:
Rust, .NET SDK и генератор Tree-sitter не нужны. См. [источники и лицензии](THIRD_PARTY.md).
