// GamerTCP - Windows 10/11 network optimization toolkit
// Native C++ reimplementation of the GameTCP3 tweak set (BSD 3-Clause).
//
// engine.h - tweak tables, registry access, backup/restore, netsh helpers.
// This is the shared core used by both the CLI and the GUI front ends.

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <functional>
#include <string>
#include <vector>

namespace gtc {

// ---------------------------------------------------------------- metadata

const wchar_t* VersionString();
const wchar_t* CopyrightString();

enum class Root { LocalMachine, CurrentUser };

const wchar_t* RootTag(Root root);      // "HKLM" / "HKCU"
HKEY           HiveOf(Root root);       // HKEY_LOCAL_MACHINE / HKEY_CURRENT_USER
std::wstring   DisplayPath(Root root, const std::wstring& path);

// ---------------------------------------------------------------- tweaks

struct Tweak {
    Root          root;
    const wchar_t* path;     // key path relative to the hive
    const wchar_t* name;     // value name
    bool          isString;  // REG_SZ when true, REG_DWORD when false
    DWORD         dw;        // target value for DWORD tweaks
    const wchar_t* sz;       // target value for string tweaks
    const wchar_t* note;     // one-line explanation
};

enum class SectionMode {
    Fixed,            // values live directly under Tweak::path
    RecurseSubkeys,   // apply to every nested subkey that holds values
    NetbtInterfaces   // apply to every direct subkey of the NetBT Interfaces key
};

struct Section {
    const wchar_t* id;
    const wchar_t* name;
    const wchar_t* summary;
    SectionMode    mode;
    const Tweak*   tweaks;
    int            count;
    bool           defaultOn;
};

// Returns true when a single value is part of the operation. An empty filter
// means "everything in the section".
using TweakFilter = std::function<bool(const Section&, const Tweak&)>;

const std::vector<Section>& Sections();
const Section* FindSection(const std::wstring& idOrName);
std::wstring   Lower(const std::wstring& s);
std::vector<std::wstring> SplitList(const std::wstring& text, wchar_t sep);
std::wstring   Trim(const std::wstring& s);
std::wstring   FormatWin32Error(LONG code);

// ---------------------------------------------------------------- registry

struct OldValue {
    bool         exists = false;
    DWORD        type   = 0;
    DWORD        dw     = 0;
    std::wstring text;
    std::wstring Display() const;
};

OldValue ReadValue(Root root, const std::wstring& path, const std::wstring& name);

LONG WriteDword(Root root, const std::wstring& path, const std::wstring& name, DWORD value);
LONG WriteString(Root root, const std::wstring& path, const std::wstring& name, const std::wstring& value);
LONG DeleteValue(Root root, const std::wstring& path, const std::wstring& name);

enum class State { Changed, Unchanged, Failed, Deleted };

struct Entry {
    std::wstring section;
    std::wstring sub;      // interface GUID, for per-interface sections
    std::wstring path;     // full display path
    std::wstring name;
    std::wstring oldData;
    std::wstring newData;
    std::wstring error;
    State        state = State::Unchanged;
};

struct Summary {
    int changed   = 0;
    int unchanged = 0;
    int failed    = 0;
    void Add(const Entry& e);
};

using EntrySink = std::function<void(const Entry&)>;

struct ApplyOptions {
    bool         dryRun = false;
    bool         backup = false;
    std::wstring backupPath;
    TweakFilter  filter;
};

Summary ApplySections(const std::vector<const Section*>& sections,
                      const ApplyOptions& options,
                      const EntrySink& sink);

struct ScanRow {
    std::wstring  section;
    const Tweak*  tweak = nullptr;
    std::wstring  sub;
    std::wstring name;
    std::wstring current;
    std::wstring target;
    bool         matches = false;
    bool         exists  = false;
    bool         failed  = false;
};

// Reads every value and reports whether it already matches the target.
Summary ScanSections(const std::vector<const Section*>& sections, std::vector<ScanRow>& rows,
                     const TweakFilter& filter = TweakFilter());

// ---------------------------------------------------------------- backups


struct BackupRow {
    enum class Kind { Missing, Dword, String };
    Kind         kind = Kind::Missing;
    Root         root = Root::LocalMachine;
    std::wstring path;
    std::wstring name;
    DWORD        dw   = 0;
    std::wstring sz;
};

std::wstring DefaultBackupPath();
std::wstring DefaultBackupDirectory();

bool BackupSections(const std::vector<const Section*>& sections,
                    const std::wstring& file,
                    std::wstring* error,
                    const TweakFilter& filter = TweakFilter());

bool ReadBackupFile(const std::wstring& file, std::vector<BackupRow>& rows, std::wstring* error);

bool RestoreBackup(const std::wstring& file, const EntrySink& sink, Summary* summary,
                   std::wstring* error);

// ---------------------------------------------------------------- commands

struct CommandResult {
    std::wstring command;
    DWORD        exitCode = 0;
    std::wstring output;
    bool         ok = false;
};

struct CommandSummary {
    int succeeded = 0;
    int failed    = 0;
};

using CommandSink = std::function<void(const CommandResult&)>;

const std::vector<std::wstring>& ResetStackCommands();

CommandSummary RunCommands(const std::vector<std::wstring>& commands, bool dryRun,
                           const CommandSink& sink);

bool RunAndCapture(const std::wstring& commandLine, std::wstring* output, DWORD* exitCode);

// ---------------------------------------------------------------- teredo

struct TeredoOption {
    const wchar_t* label;   // menu label
    const wchar_t* netsh;   // argument for "netsh int teredo set state"
    const wchar_t* note;
};

const TeredoOption* TeredoOptions(int* count);
std::wstring TeredoShowState(std::wstring* output, bool* ok);
bool         TeredoSetState(const std::wstring& netshArgs, std::wstring* output, DWORD* exitCode);

// ---------------------------------------------------------------- elevation

bool IsElevated();
bool RelaunchElevated(const std::wstring& arguments, std::wstring* error);

}  // namespace gtc
