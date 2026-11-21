# Changelog

All notable changes to **GamerTCP 4** — the self-contained native C++ rewrite of
[Zanphar/GameTCP](https://github.com/Zanphar/GameTCP) (GameTCP3) — are documented
here. Newest first.

The version follows the toolkit it replaces: GameTCP3 is generation 3, so the
first public C++ release is **4.0.0**. The earlier 1.0.0/1.1.0 numbers were
internal development snapshots and are folded into 4.0.0 below.

The tweak set itself belongs to upstream. It was transcribed from `GameTCP3.vb`
at upstream version **3.0.0**, and upstream's own history is summarised near the
bottom so the two toolkits can be compared and merged later.

## [4.2.1] - 2026-11-21

Fixes the values that looked as if they were never written, and stops an apply
from running the destructive TCP/IP reset. The tweak set and the target values
are unchanged from 4.2.0.

### Fixed

- An apply no longer resets the stack out from under its own writes.
  `netsh int ip reset` overwrites `Tcpip\Parameters` and deletes the per-interface
  values (its own reset log shows `DontAddDefaultGateway` being removed) as well
  as `NetbiosOptions`, so when the reset ran after the writes it removed what had
  just been written: the interface sections, the NetBT interface section and most
  of the `Tcpip` values reverted immediately. The global TCP settings now run
  before the writes, and the destructive resets are not part of an apply at all
  (see **Changed**).
- The confirmations, the Activity log and the CLI output now state what runs, and
  the per-section results report the read-back outcome.

### Changed

- `apply` — `Apply Selected`, `ZGT Performance Setup` and `GamerTCP apply` — no
  longer runs the destructive stack resets. Microsoft documents
  `netsh int ip reset` as equivalent to removing and reinstalling TCP/IP, and it
  deletes the per-interface keys, so an apply was discarding static addresses,
  DHCP options and the per-interface tweaks as a side effect. The 23 commands are
  split into the 18 global TCP settings and cache flushes that an apply
  re-applies, and the 5 resets (`int tcp reset`, `int ipv4 reset`, `int ipv6
  reset`, `int ip reset`, `winsock reset catalog`) that only run from **Reset
  Network Stack**, `GamerTCP reset`, or `apply --with-reset`. The log says which
  of the two ran and that the reset was skipped, the reset action warns that it
  discards interface keys and static IP configuration, and `--no-reset` now skips
  the global TCP settings as well.

### Added

- Every apply now ends by reading all the values it wrote back from the registry;
  any that no longer hold the written value are reported as `FAILED` with what
  was found instead, and counted as failures rather than changes. This is what
  would have caught the reset ordering above on the first run, and it catches any
  other silent revert (a service, a policy) the same way. Both the GUI log and
  `GamerTCP apply` do it.
- `build.sh` cross-builds and packages the release on Linux with Zig and a
  MinGW-w64 `windres`, mirroring `build.bat` + `dist.bat`: both executables, the
  changelog/compiled version check that `selftest` performs on Windows, and the
  four files in `dist/`.

## [4.2.0] - 2026-11-15

Version bump for the per-value current readings and the refresh action. The tweak
set, the target values and the reset commands are unchanged from 4.1.0.

### Added

- Every row in the section tree shows what the value is set to right now. The
  per-interface areas (`interfaces`, `netbt-interfaces`) read every interface
  through the engine's recursive scan, collapse identical values and report the
  interface count, for example `TcpAckFrequency = 2 x3  (3 interfaces) -> 1`.
  Previously only the fixed sections showed a current value.
- **Refresh Values** in the GUI, also on **F5**: re-reads all 106 values and
  rewrites the tree without applying, previewing or scanning first. It only
  reads, so it works without elevation, and it is disabled while an operation is
  running.

### Changed

- Current values are read once per refresh through `ScanSections` and rewritten
  after an apply, preview, restore or scan, so the labels match what the registry
  holds at that moment.

## [4.1.0] - 2026-11-13

Version bump for the updated download package. The toolkit, tweak set and
CLI/GUI behaviour are unchanged from 4.0.0.

### Added

- Download page: one-click **Copy** buttons for each artifact's SHA-256 and a
  downloadable `SHA256SUMS.txt` covering all three packages.
- Download page: a verification hint showing `certutil -hashfile <file> SHA256`
  on Windows and `sha256sum <file>` on macOS/Linux.

### Changed

- The release date on the download page uses `Month DD, YYYY` format.

## [4.0.0] - 2026-11-09

First public release: GameTCP3 reimplemented in C++17 as one dependency-free
Windows executable with both a CLI and a GUI, plus per-value selection on top of
the original section-level tweak set.

### Added — GUI

- **Power plan** panel behind a header bolt button: reads the active Windows
  power scheme, lists every available plan with its description, and applies
  the selected plan without elevation, logging the change under Activity.
- Performance meter behind the ZGT header icon: a progress ring plus a live line
  reporting the share of recommended values at target, animated up after an
  apply, restore or scan.
- **ZGT Performance Setup** header action: selects all 106 recommended values and
  runs the existing backup, apply and stack-reset flow with one confirmation,
  logging under its own heading.
- Modernised window chrome with light, dark and system colour schemes: rounded
  cards replace the group boxes, flat owner-drawn buttons get hover, pressed and
  focus states, and an Appearance panel behind a header gear button switches
  Light, Dark and System, recolouring the tree, activity log, scroll bars,
  buttons and title bar. Button fills, the meter ring, log headings and the
  selected option follow the Windows personalisation accent, with the original
  cyan as fallback. System is the default and
  follows the Windows app theme live. The choice is stored per user in
  `HKCU\Software\GamerTCP4` (`Theme`).
- Window size, position and maximised state are remembered per user alongside
  the theme and restored on the next launch, clamped to the nearest monitor's
  work area when the saved rectangle is no longer visible.
- Expandable section tree: each of the 15 areas opens into the registry values it
  writes (106 values total, 121 tree nodes), with the current registry value next
  to the target beside each name and a one-line explanation as a tooltip.
- Per-value checkboxes. Ticking a section ticks its whole list; clicking a value
  re-syncs its section box, which only reads as ticked when every value under it
  is ticked. The status bar reports the selection ("106 of 106 values selected").
- **Apply Selected**, **Preview**, **Scan** and **Backup…** operate only on the
  ticked values, including the automatic pre-apply backup.
- **About…** dialog with the version, copyright, the full BSD 3-Clause licence
  text in an expandable details area, and the experimental-software warning.
- Coloured activity log (RichEdit), worker thread so the window stays responsive,
  confirmation prompts, Teredo task dialog, file dialogs for backup/restore, and
  a restart-as-administrator banner when the process is not elevated.

### Added — CLI

- Commands: `apply`, `preview`, `status`, `list`, `backup`, `restore`, `reset`,
  `teredo`, `selftest`.
- Options: `-s/--section` (repeatable, comma lists), `-y/--yes`, `-n/--dry-run`,
  `--no-reset`, `--no-backup`, `--all-values`, `-o/--backup-file`, `-h`, `-v`.
- Colourised output when attached to a console, plain UTF-8 when redirected.
- Exit codes: `0` success, `1` usage error, `2` completed with failures,
  `5` administrator privileges required.

### Added — safety

- `preview`/`--dry-run`, a `status` diff against the targets, and timestamped
  backups written before any change.
- `restore`, with a fresh backup taken beforehand — the only exact undo for
  `apply`.
- Versioned, human-readable backup format (`# GamerTCP backup v1`,
  `HIVE|KEY_PATH|VALUE_NAME|TYPE|DATA`) with an explicit `MISSING` marker for
  values that did not exist, so restore can delete them again.
- `selftest`: read-only checks of the tables, registry access, backup
  round-trip, per-value filtering and the reset command list. It also compares
  the compiled version with the newest release heading in `CHANGES.md`, and
  skips that check when the file is not shipped next to the executable.

### Added — engine and build

- `TweakFilter`, so `ApplySections`, `ScanSections` and `BackupSections` can act
  on a subset of a section's values; an empty filter means the whole section.
- Recursive per-interface expansion (8 values per network interface, 1 per NetBT
  interface), reproducing the original's nested subkey traversal.
- Multi-size `ZGT` application icon (`res/gamerTCP.ico`, id `IDI_APP_ICON` in
  `res/resource.h`): embedded in `GamerTCP.exe` for the window title bar,
  taskbar and Explorer, and in `GamerTCP-Setup.exe`; the Start Menu shortcut
  inherits it from the executable.
- `build.bat` with an MSVC path (`/MT`, static CRT) and a MinGW-w64 fallback
  (`-static`), both embedding exactly one application manifest (common controls
  v6, DPI aware, `asInvoker`).
- Optional per-user installer (`installer/setup.cpp`, built by
  `installer/build-installer.bat`): installs into
  `%LOCALAPPDATA%\Programs\GamerTCP4`, adds a Start Menu shortcut and an entry
  in Settings > Apps, and removes all of it with `--uninstall`. No administrator
  rights and no installer framework are involved.
- `site/`, a static project page describing the tool with download links for the
  installer, the portable executable and the source archive.
- `dist.bat`, which produces three separate deliverables: a source distribution
  (`GamerTCP-<version>-source.zip` with `src/`, `res/`, `installer/` and both
  build scripts), a portable binary distribution
  (`GamerTCP-<version>-portable.zip` with the executable and docs) and the
  per-user installer (`GamerTCP-<version>-setup.exe`), building either executable
  on demand when missing (and rebuilding the installer when a newly built
  executable has to be embedded), plus a `-SHA256SUMS.txt` file with the hashes
  of all three. It takes the version from `CHANGES.md` and refuses to package
  unless `selftest` passes.
- `README.md`, this changelog, and `LICENSE.md` (BSD 3-Clause, retaining the
  upstream copyright).

### Changed

- One statically linked `GamerTCP.exe`: no .NET Framework, no Visual Basic
  runtime, no installer, no `vbc`/`mt` toolchain.
- CLI and GUI share the same engine, so a run behaves identically either way.
- Registry writes are reported per value as `changed` / `unchanged` / `failed`
  and do not abort the run, matching upstream's counter behaviour.
- `netsh` execution reads the redirected output before waiting, keeping
  upstream's 3.0.0 deadlock fix and its per-command error reporting.
- The product reads **GamerTCP 4** consistently: window title, header, About and
  Teredo dialog captions, console title, version resource and licence text. The
  executable, the `GamerTCP` command name and the `# GamerTCP backup v1` format
  tag are unchanged, so existing backups and shortcuts keep working.

### Fixed

- Tree checkbox state is set explicitly after each item is inserted; passing
  `TVIS_STATEIMAGEMASK` at insert time is not reliably honoured by
  `TVS_CHECKBOXES` and could leave the default selection unmarked.
- GCC's startfile spec injected a second `default-manifest.o`, producing
  "multiple non-default manifests"; the MinGW path now compiles
  `res/manifest_only.rc` and links it explicitly with `-B build/`.

### Removed

- `GameTCP.vb`, `GameTCP2.vb`, `GameTCP3.vb` and the prebuilt `GameTCP*.exe`
  binaries.
- `Create.bat`, `GameSet.bat`, `GameTCP-AIO.bat`, `teredo.bat`: building is
  `build.bat`, elevation is built into the executable, and Teredo is a `teredo`
  command and a GUI dialog.
- `GameTCP.reg`, `GameTCP2.exe.manifest`, `GameTCP3.exe.manifest` and the
  temporary `.vbs` elevation shim — one embedded manifest replaces them.

## Upstream reference — GameTCP3 3.0.0 (2026-08-21)

Summarised from [upstream `CHANGES.md`](https://github.com/Zanphar/GameTCP/blob/main/CHANGES.md),
which remains the authority for the original toolkit:

- Unified `GameTCP3.vb`, merging GameTCP (v1) and GameTCP2 (v2).
- Recursive network interface traversal for nested registry subkeys.
- Administrator manifest for elevation; success/fail/unchanged counters.
- Unchanged-value detection in the MSMQ, internet, policies and misc sections.
- Per-command error reporting and a succeeded/failed summary for the network
  stack reset.
- Fixed `ApplyGamesSettings` writing `Scheduling Category`, `SFIO Priority` and
  `Background Only` as DWORDs instead of strings; fixed a deadlock when the reset
  commands redirected output that was never read.
- Batch-script fixes in `Create.bat`, `GameSet.bat` and `teredo.bat`.

## Where this project intentionally differs from upstream

| Upstream | Here |
| --- | --- |
| `GameTCP3.vb`, .NET Framework, `vbc` | C++17, Win32, MSVC or MinGW-w64 |
| `GameSet.bat` elevates through a temporary VBS | `RelaunchElevated` / `ShellExecuteW` |
| `teredo.bat` menu | `teredo` CLI command and a GUI task dialog |
| `Create.bat` + `mt` for the manifest | `build.bat`, manifest in the resource |
| Section-level apply only | plus per-value selection, `preview`, `status`, `restore`, `selftest` |

What is deliberately unchanged: every tweak name, registry path, type and target
value, the order and contents of the 23 `netsh` reset commands, the four Teredo
states, the 15 sections, and the BSD 3-Clause licence and attribution.

## Keeping in sync

Upstream status as of 2026-11-09: version 3.0.0, 8 commits on `main`, no changes
outstanding against this rewrite.

When upstream moves:

1. Diff the `Apply*` functions in `GameTCP3.vb` against the tables in
   `src/engine.cpp` (the sections are declared as `k<Name>Values` arrays).
2. Add or change values in the matching array. Counts, per-interface expansion
   and the GUI tree follow automatically.
3. If the reset list or Teredo options changed, update `ResetStackCommands()` or
   `TeredoOptions()`.
4. Re-check the count assertions in `selftest` (`src/cli.cpp`) — it expects
   106 values across 15 sections and 23 reset commands — then run
   `GamerTCP.exe selftest`.
5. Add an entry under `[Unreleased]`, name the upstream commit or release, bump
   the minor version (for example 4.1.0), and mention it in `README.md` if the
   area table changed. `selftest` fails when the binary version and the newest
   heading disagree, so bump the version and the heading together.
