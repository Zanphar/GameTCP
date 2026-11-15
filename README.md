# GamerTCP 4

A self-contained native C++ (Win32) reimplementation of the **GameTCP3** Windows
network optimization toolkit, with both a **CLI** and a **GUI** in a single
dependency-free `GamerTCP.exe`.

The current release is **4.2.0** (2026-11-15) — the fourth generation of the
tool, following GameTCP3. See [`CHANGES.md`](CHANGES.md) for the full history and
for how upstream changes are tracked.

The tweak set, values and network-stack reset commands are derived from
[Zanphar/GameTCP](https://github.com/Zanphar/GameTCP) (GameTCP3, BSD 3-Clause,
© 2022 Charles McDonald). This project rewrites that toolkit in C++ with no .NET
runtime, no external libraries and no installer.

## What it does

| Area | Contents |
| --- | --- |
| Sections | 15 (TCP/IP, interfaces, DNS cache, service provider, LanmanServer, LanmanWorkstation, Multimedia, Games task profile, Memory Management, MSMQ, NetBT, NetBT interfaces, Internet settings, Policies, Miscellaneous) |
| Registry values | 106 (97 fixed values plus 8 per network interface and 1 per NetBT interface) |
| Network stack reset | The original 23 `netsh` commands (TCP/IPv4/IPv6 reset, Winsock catalog, ARP cache, global TCP parameters) |
| Teredo | Show state, disable, default and enterprise-client modes |
| Safety | Automatic timestamped backup before writes, `preview`/dry-run, `status` diff, `restore`, pre-restore backup |

Everything that the original `GameTCP3.vb` wrote is reproduced here, including
recursive per-interface configuration. Like the original, this software is
experimental: registry and TCP/IP tuning is not universally beneficial, and it
does not guarantee lower latency.

## Requirements

* Windows 10 or Windows 11 (x64 or ARM64 under x64 emulation)
* Administrator privileges for `apply`, `reset`, `restore` and Teredo changes
* No runtime: the executable is statically linked, no .NET or VC++ redistributable

Reading state (`status`, `list`, `preview`, `backup`, `scan`) works without
elevation.

## Build

```bat
build.bat          :: MSVC when installed, otherwise MinGW-w64 g++
build.bat mingw    :: force the MinGW-w64 build
```

The script compiles the resources and links one self-contained `GamerTCP.exe`:

* **MSVC** via `vswhere` / `vcvars64.bat`, with `/MT` so the C runtime is static
  (the default when Visual Studio C++ tools are installed)
* otherwise **MinGW-w64 g++** with `-static`

Both paths embed exactly one application manifest (common controls v6, DPI
awareness, `asInvoker`). GCC's startfile spec normally adds a second default
manifest, so the MinGW path compiles `res/manifest_only.rc` into
`build/default-manifest.o` and links with `-B<abs>/build/`.

Manual MinGW build:

```sh
windres -O coff -i res/gamerTCP.rc -o build/gamerTCP.res.o        # from res/ -> version info
windres -O coff -i res/manifest_only.rc -o build/default-manifest.o
g++ -std=c++17 -O2 -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601 \
    -mwindows -static -static-libgcc -static-libstdc++ -B"$PWD/build/" \
    src/main.cpp src/engine.cpp src/cli.cpp src/gui.cpp build/gamerTCP.res.o \
    -o GamerTCP.exe -lcomctl32 -lcomdlg32 -ladvapi32 -lshell32 -lgdi32 -luser32 \
    -ldwmapi -luxtheme -lpowrprof
```

## Distribution

```bat
build.bat                      :: build GamerTCP.exe (dist.bat does this on demand)
installer\build-installer.bat  :: build GamerTCP-Setup.exe (dist.bat does this on demand)
dist.bat                       :: package the three dist\ deliverables
```

`dist.bat` produces three separate deliverables in `dist\`, all named from the
newest heading in `CHANGES.md` (the version `selftest` also checks):

* `GamerTCP-<version>-source.zip` plus the staged `-source\` folder: the complete
  source, including `src/`, `res/`, `installer/`, `build.bat`, `dist.bat` and the
  documentation.
* `GamerTCP-<version>-portable.zip` plus the staged `GamerTCP-<version>\` folder:
  the statically linked `GamerTCP.exe` with `README.md`, `CHANGES.md` and
  `LICENSE.md` for a no-install run.
* `GamerTCP-<version>-setup.exe`: the per-user installer produced by
  `installer\build-installer.bat`.
* `GamerTCP-<version>-SHA256SUMS.txt`: SHA-256 hashes of the three artifacts,
  in `sha256sum -c` format.

Packaging refuses to run if `selftest` fails. A missing `GamerTCP.exe` or
`GamerTCP-Setup.exe` is built on demand with `build.bat` or
`installer\build-installer.bat`; when the executable had to be rebuilt, the
installer is rebuilt too so its embedded payload stays current. The source
archive rebuilds with `build.bat` on any machine with the Visual Studio C++
tools or MinGW-w64.

## GUI

Run `GamerTCP.exe` with no arguments (or `GamerTCP.exe --gui`).

* **ZGT Performance Setup** in the header is the one-click path: it ticks every
  recommended value (106 across 15 sections), takes the usual timestamped
  backup, applies them all and resets the network stack, after a single
  confirmation. It needs elevation, and the backup undoes it through Restore.
* **Power plan** in the header (bolt button) reads the active Windows power
  scheme and lists every plan Windows offers with its description. Selecting a
  plan and pressing **Apply** switches Windows to it immediately, without
  elevation, and the change is logged under Activity.
* A performance meter sits behind the ZGT icon in the header: the ring shows
  what share of the recommended registry values already match their targets, and
  animates up after an apply, restore or scan.
* Buttons, the performance ring, log headings and the selected Appearance row use
  the user's Windows accent colour and follow it live when Windows changes it;
  the original cyan remains only as a fallback when no accent is available.
* The window follows the Windows light/dark setting by default. The gear button
  in the header opens a separate **Appearance** panel with **Light**, **Dark**
  and **System** options, keeping the main window uncluttered. The choice is
  remembered per user in `HKCU\Software\GamerTCP4` (`Theme`); cards, tree,
  activity log, scroll bars and title bar follow it, and System mode tracks the
  Windows app theme live.
* Window size, position and maximised state are remembered in the same per-user
  key (`WindowX`, `WindowY`, `WindowWidth`, `WindowHeight`, `WindowMaximized`)
  and restored on the next launch, clamped to the nearest monitor's work area if
  the saved rectangle is no longer visible.
* Every area is expandable: it lists each registry value it writes with the
  current value next to the target (for example `DefaultTTL = 255 -> 64`), and
  refreshes those values after an apply, a restore, a scan or a refresh. The
  per-interface sections show every distinct current value with its interface
  count, for example `TcpAckFrequency = 2 x3  (3 interfaces) -> 1`. Hover a value
  for its one-line meaning.
* Tick individual values, or click a section's checkbox to tick/uncheck its whole
  list (`Select All` / `Select None` cover everything). A section's box shows
  ticked only when all of its values are ticked.
* `Apply Selected`, `Preview`, `Scan` and `Backup` act only on the ticked values.
* **Apply Selected** backs up every current value of the ticked items, writes
  them, then resets the network stack — with a confirmation first.
* **Preview (dry run)** shows exactly which values would change without writing.
* **Scan Current State** prints current values next to their targets.
* **Refresh Values** (F5) re-reads every value, including the per-interface ones,
  and rewrites the tree, so the current values can be inspected at any time
  without applying or scanning first.
* **Backup… / Restore…** use file dialogs and the same backup format as the CLI.
* **Reset Network Stack** and **Teredo…** wrap the `netsh` operations.
* **About…** shows the version, copyright and the full BSD 3-Clause licence text.
* If started without elevation the window is read-only and offers
  **Restart as administrator**.

Long operations run on a worker thread, so the window stays responsive, and the
activity log colour-codes changes, failures and summaries.

## CLI

```
GamerTCP <command> [options]

Commands
  apply      Apply the registry tweaks and reset the network stack
  preview    Show what apply would change without writing anything
  status     Show current values against their targets
  list       List every section and registry value in the toolkit
  backup     Save current values to a backup file
  restore    Restore values from a backup file
  reset      Reset the TCP/IP stack and Winsock catalog with netsh
  teredo     Show or change the Teredo tunneling state
  selftest   Run built-in self checks (read only)

Options
  -s, --section <id>[,<id>]  Limit to these sections (repeatable)
  -y, --yes                  Do not prompt for confirmation
  -n, --dry-run              Preview without writing (apply, reset)
      --no-reset             Skip the network stack reset
      --no-backup            Skip the automatic backup
      --all-values           status: also show values that already match
  -o, --backup-file <path>   Backup destination
  -h, --help                 Show this help
  -v, --version              Show version information
```

Section ids: `tcpip`, `interfaces`, `dnscache`, `serviceprovider`,
`lanmanserver`, `lanmanworkstation`, `multimedia`, `games`, `memory`, `msmq`,
`netbt`, `netbt-interfaces`, `internet`, `policies`, `misc`.

### Examples

```bat
GamerTCP preview
GamerTCP status -s games --all-values
GamerTCP apply -s tcpip,games,interfaces -y
GamerTCP backup D:\backups\before-gamertcp.txt
GamerTCP restore D:\backups\before-gamertcp.txt
GamerTCP reset -y
GamerTCP teredo show
```

Exit codes: `0` success, `1` usage error, `2` completed with failures,
`5` administrator privileges required.

CLI output is colourised when attached to a console; redirected output is plain
UTF-8.

## Backups

`backup` records the current value of every selected registry value, plus an
explicit marker for values that did not exist:

```
# GamerTCP backup v1
# created 2026-11-09 14:03:21
# format: HIVE|KEY_PATH|VALUE_NAME|TYPE|DATA
HKLM|SYSTEM\CurrentControlSet\Services\Tcpip\Parameters|DefaultTTL|DWORD|00000040
HKLM|...|Scheduling Category|SZ|48006900670068
HKLM|...|SomeValue|MISSING|
```

`restore` replays a file: DWORD and string values are written, `MISSING` entries
are deleted again. A fresh backup is written before restoring, and restoring is
the only way to undo `apply` exactly.

## Layout

```
src/engine.h        tweak table types + shared API
src/engine.cpp      all 106 tweaks, registry access, backup/restore, netsh, Teredo
src/cli.cpp         command line front end (console attach, colours, prompts)
src/gui.cpp         Win32 GUI (TreeView, RichEdit log, worker thread)
src/main.cpp        entry point: GUI when no arguments
res/gamerTCP.rc     version info (+ manifest for the MSVC build)
res/manifest_only.rc  manifest object source for the MinGW build
res/gamerTCP.manifest asInvoker, common controls v6, DPI awareness
res/gamerTCP.ico    multi-size ZGT application icon (window, taskbar, Explorer)
res/resource.h      shared resource ids
build.bat           MSVC build, MinGW-w64 fallback
dist.bat            source zip, portable zip and setup installer (runs selftest first)
dist/               packaged source, portable binary and setup installer
site/               project website with the download links
installer/          per-user installer source and build script
CHANGES.md          version history and upstream sync notes
```

## Notes and limitations

* A reboot is recommended after `apply` or `reset`; some values only take effect
  at boot.
* `netsh int tcp reset` and neighbours reset can drop connections briefly.
* Individual tweaks can fail on modern Windows builds (for example removed or
  deprecated TCP parameters); failures are reported per value and do not abort
  the run.
* Applying requires write access to three hives (`HKLM\SYSTEM`, `HKLM\SOFTWARE`,
  `HKCU`), so elevation is required.
* `GamerTCP.exe selftest` also checks that the compiled version matches the
  newest release heading in `CHANGES.md`; the check is skipped when that file is
  not next to the executable.

## License

BSD 3-Clause. See `LICENSE.md`. The tweak set is derived from GameTCP3 by
Charles McDonald (© 2022), also BSD 3-Clause.
