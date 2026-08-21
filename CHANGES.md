# Changelog

All notable changes to GameTCP3 are documented here.

## [3.0.0] - 2026-08-21

### Added
- Unified `GameTCP3.vb` merging GameTCP (v1) and GameTCP2 (v2) into a single tool
- Recursive network interface settings traversal for all nested registry subkeys
- `GameTCP3.exe.manifest` for administrator privilege elevation
- Module-level success/fail/unchanged counters with conditional summary output
- Unchanged value detection in `ApplyMsmqSettings`, `ApplyInternetSettings`, `ApplyPoliciesSettings`, `ApplyMiscSettings`
- Section header and interface count for `ApplyNetbtInterfaceSettings`
- Per-command error reporting in `ResetNetworkStack` (shows failed command + error message)
- `ResetNetworkStack` summary (reports succeeded/failed counts)
- Compiler (`vbc`) and manifest tool (`mt`) availability checks in `Create.bat`
- License headers (`REM`) in `Create.bat`, `GameSet.bat`, `teredo.bat`
- Comprehensive header comment in `GameTCP3.vb` with README overview and full license text
- `CHANGES.md` changelog

### Fixed
- `ApplyGamesSettings` set `Scheduling Category`, `SFIO Priority`, `Background Only` as DWORDs instead of strings (`"High"`, `"High"`, `"False"`)
- `ResetNetworkStack` redirected stdout/stderr but never read them before `WaitForExit()`, risking deadlock
- `Create.bat` referenced deleted `GameTCP.vb`/`GameTCP2.vb` and compiled to old executables
- `GameSet.bat` launched `GameTCP.exe` instead of `GameTCP3.exe`
- `GameSet.bat` did not clean up temporary VBS file on all exit paths
- `GameSet.bat` did not pass exit code to caller
- `teredo.bat` had no administrator privilege check before modifying network settings
- `teredo.bat` used `goto end` fallthrough that skipped exit code handling
- `Create.bat` parser error ("not was unexpected at this time") from nested `if` blocks with delayed expansion
- `README.md` referenced deleted files (`GameTCP`, `GameTCP2`) and non-existent `GameTCP-AIO.bat`

### Removed
- `GameTCP.vb` (merged into `GameTCP3.vb`)
- `GameTCP2.vb` (merged into `GameTCP3.vb`)
- Dead code from `GameTCP3.vb`: `_stageNumber`, `ShowStageHeader`, `ShowRegistryResult`, `ShowValueNotFound`, `ApplyRegistryValue`, `ApplyRegistryString`

### Changed
- Renamed `LICENSE` to `LICENSE.md`
- Updated `README.md` to reflect unified GameTCP3 tool
