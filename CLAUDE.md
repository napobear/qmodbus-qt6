# CLAUDE.md

Project-specific notes for working on QModBus (a Qt6 port of the upstream
QModBus Modbus master GUI — see README for credits/upstream links).

## Build

- Qt 6.11.2, qmake project (no CMake). Build out-of-tree:
  ```
  mkdir build && cd build
  qmake6 ..
  make -j$(nproc)
  ```
  Use `qmake6`, not `qmake`/`qmake5` — this machine (openSUSE Tumbleweed)
  keeps the Qt6 qmake binary under that name. Required dev packages:
  `qt6-base-devel`, `qt6-serialport-devel`.
- `build/` is gitignored; remove it after testing (`rm -rf build`) rather
  than leaving it around.
- Treat any new compiler warning in `src/` (not `3rdparty/`) as something to
  fix, not ignore — `3rdparty/libmodbus` has a handful of pre-existing
  fallthrough/unused-parameter/maybe-uninitialized warnings that are not
  ours to clean up incidentally.

## Verifying a change is not optional

A clean build is not sufficient evidence a change works — this codebase has
no automated test suite, and several real bugs in this project only showed
up at runtime (a NULL-pointer crash, silently-swapped IP octets, a fragile
index-based lookup) despite compiling cleanly. Before considering a change
done:

- Run the app with `QT_QPA_PLATFORM=offscreen` (no X server needed) and
  exercise the actual code path, not just `--help`.
- For anything touching serial (RTU/ASCII), test against a `socat` pty pair
  (`socat -d -d pty,raw,echo=0,link=/tmp/ptyA pty,raw,echo=0,link=/tmp/ptyB`)
  rather than assuming a real port. A pty is deliberately *not* enumerated
  by `QSerialPortInfo::availablePorts()`, which is exactly the edge case
  that broke the original serial-port-selection code.
- For anything touching Modbus TCP, script a tiny raw-socket Python server
  that parses the MBAP header + PDU and prints unit id / function code /
  start address / quantity, and diff that against what was requested. This
  is how the CLI request options were verified end-to-end, not just that
  `modbus_connect()` returned 0.
- `qmodbus` persists settings via `QSettings` at
  `~/.config/EDC Electronic Design Chemnitz GmbH/QModBus.conf`. Test runs
  write real values there (serial port, baud, TCP host, etc.) — delete that
  file (and the now-empty parent dir) after testing so it doesn't leak into
  the user's next manual run.
- `timeout N` every invocation that connects or shows the GUI — the Qt
  event loop runs forever otherwise and will hang the shell.

## Architecture notes / known gotchas

- **No bundled serial port library.** `3rdparty/qextserialport` was removed
  entirely (huge, unmaintained, Qt4/5-era). Serial port *enumeration* (for
  the port dropdown) uses Qt's own `QSerialPortInfo`. Actual serial/TCP I/O
  never goes through Qt at all — it's handled by the bundled
  `3rdparty/libmodbus` (POSIX/Win32 syscalls directly). Don't reintroduce a
  Qt-serial-for-I/O dependency; `QSerialPortInfo` is enumeration-only here.
- **`globalMainWin` ordering.** `src/main.cpp` has a global `MainWindow *
  globalMainWin`, used by the static libmodbus monitor callbacks
  (`MainWindow::stBusMonitorAddItem`/`stBusMonitorRawData`). It **must** be
  assigned before anything that can trigger a real modbus connect/request —
  including `MainWindow::applyCliOptions()`. That's exactly why
  `applyCliOptions()` is a public method called from `main()` after
  `globalMainWin = &w;`, rather than being invoked from inside the
  `MainWindow` constructor: doing the latter caused a NULL-dereference
  segfault the first time CLI options triggered an actual
  `modbus_read_registers()` during construction.
- **Driving combo boxes programmatically.** `SerialSettingsWidget` and
  `TcpIpSettingsWidget` wire each settings combo box's `currentIndexChanged`
  straight to a slot that immediately (re)connects
  (`SerialSettingsWidget::changeSerialPort`). If you need to set several
  fields before connecting (see `configureAndActivate()` in both classes),
  block each combo's signals first (`QSignalBlocker`) and fire one explicit
  connect call at the end — otherwise every intermediate `setCurrentIndex`
  fires a partially-configured connect attempt.
- **TCP mode reconnects per request.** `MainWindow::sendModbusRequest()`
  calls `tcpSettingsWidget->tcpConnect()` again at the top whenever
  `m_tcpActive` is true, opening a fresh TCP connection for every request
  (pre-existing, intentional design — see git log "Open new TCP connection
  for each request"). This means a CLI-driven connect-then-send opens *two*
  TCP connections (one from `configureAndActivate()`, a second from
  `sendModbusRequest()`); that's expected, not a bug.
- **The "ModBus Request" panel is shared across all three mode tabs.** Slave
  ID / Function code / Start address / Num of coils (`ui->slaveID` etc.,
  `src/mainwindow.h`) live on `MainWindow` itself, not on a per-mode
  settings widget — they apply to whichever mode is currently active.
- **CLI options** (`--mode`, per-mode connection params, TCP request
  params) are parsed in `src/main.cpp` with `QCommandLineParser`, carried in
  `CliModbusOptions` (`src/climodbusoptions.h`), and applied by
  `MainWindow::applyCliOptions()`. TCP request options
  (`--slave-id`/`--function-code`/`--start-address`/`--num-coils`) are
  independent of each other — giving any one sends a request, the rest
  default to the GUI's own defaults (slave ID 1, read-coils, address 0,
  count 1). `--function-code` only accepts the four *read* operations: a
  write code has no CLI option to supply the value(s) to write, so accepting
  one would silently send zeros — reject it instead of guessing.

## Code style

- Match the indentation of the file you're editing rather than reformatting
  it: most of the original codebase (`mainwindow.cpp`, `BatchProcessor.cpp`,
  the `*settingswidget.*` files) uses tabs; a few newer files
  (`ipaddressctrl.cpp`, `iplineedit.*`) use 4-space indentation. Don't mix
  the two within one file.
- Prefer `nullptr` and range-`for` in new code; the codebase predates both
  and still has `NULL`/`foreach` in older functions — no need to churn
  those incidentally, but don't add new instances.

## Git workflow

When asked to ship a change: create a feature branch, commit, `git push -u
origin <branch>`, open a PR with `gh pr create`, then `gh pr merge --merge
--delete-branch`. `gh` is already installed and authenticated as `napobear`
on this machine. Only do the push/PR/merge steps when explicitly asked —
implementation work on its own should stop at a local commit.
