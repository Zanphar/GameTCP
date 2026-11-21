// GamerTCP - Windows 10/11 network optimization toolkit
// Native C++ reimplementation of the GameTCP3 tweak set (BSD 3-Clause).
//
// cli.cpp - command line front end.

#include "cli.h"

#include "engine.h"

#include <algorithm>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

namespace gtc {
namespace {

enum class Color { Default, Red, Green, Cyan, Yellow, Gray, White, Bold };

HANDLE g_out = INVALID_HANDLE_VALUE;
bool   g_console = false;
bool   g_vt = false;
bool   g_allocated = false;

// ------------------------------------------------------------- output

void EnableVirtualTerminal(HANDLE handle) {
    DWORD mode = 0;
    if (GetConsoleMode(handle, &mode) &&
        SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        g_vt = true;
    }
}

bool InitOutput() {
    HANDLE standard = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD type = (standard && standard != INVALID_HANDLE_VALUE) ? GetFileType(standard)
                                                               : FILE_TYPE_UNKNOWN;

    if (type == FILE_TYPE_CHAR) {
        g_out = standard;
        g_console = true;
        EnableVirtualTerminal(g_out);
        return true;
    }
    if (type == FILE_TYPE_PIPE || type == FILE_TYPE_DISK) {
        g_out = standard;
        return true;
    }

    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        if (console && console != INVALID_HANDLE_VALUE) {
            g_out = console;
            g_console = true;
            EnableVirtualTerminal(g_out);
            SetConsoleTitleW(L"GamerTCP 4");
            return true;
        }
    }

    if (AllocConsole()) {
        g_allocated = true;
        HANDLE console = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0,
                                     nullptr);
        if (console != INVALID_HANDLE_VALUE) {
            g_out = console;
            g_console = true;
            EnableVirtualTerminal(g_out);
            SetConsoleTitleW(L"GamerTCP 4");
            return true;
        }
    }

    return false;
}

void WriteOut(const std::wstring& text) {
    if (g_out == INVALID_HANDLE_VALUE || text.empty()) return;
    DWORD written = 0;
    if (g_console) {
        WriteConsoleW(g_out, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
        return;
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                         nullptr, 0, nullptr, nullptr);
    if (size <= 0) return;
    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), &utf8[0], size,
                        nullptr, nullptr);
    WriteFile(g_out, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
}

void SetColor(Color color) {
    if (!g_vt) return;
    switch (color) {
        case Color::Red: WriteOut(L"\x1b[91m"); break;
        case Color::Green: WriteOut(L"\x1b[92m"); break;
        case Color::Cyan: WriteOut(L"\x1b[96m"); break;
        case Color::Yellow: WriteOut(L"\x1b[93m"); break;
        case Color::Gray: WriteOut(L"\x1b[90m"); break;
        case Color::White: WriteOut(L"\x1b[97m"); break;
        case Color::Bold: WriteOut(L"\x1b[1m"); break;
        default: WriteOut(L"\x1b[0m"); break;
    }
}

void Line(const std::wstring& text = std::wstring()) {
    WriteOut(text + L"\r\n");
}

void ColoredLine(Color color, const std::wstring& text) {
    SetColor(color);
    Line(text);
    SetColor(Color::Default);
}

int ConsoleWidth() {
    if (g_console) {
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (GetConsoleScreenBufferInfo(g_out, &info)) {
            const int width = info.srWindow.Right - info.srWindow.Left + 1;
            if (width >= 40) return width;
        }
    }
    return 100;
}

std::vector<std::wstring> Wrap(const std::wstring& text, size_t width) {
    std::vector<std::wstring> lines;
    std::wstring line;
    for (const std::wstring& word : SplitList(text, L' ')) {
        if (word.empty()) continue;
        if (!line.empty() && line.size() + 1 + word.size() > width) {
            lines.push_back(line);
            line.clear();
        }
        if (!line.empty()) line.push_back(L' ');
        line += word;
    }
    if (!line.empty()) lines.push_back(line);
    if (lines.empty()) lines.push_back(L"");
    return lines;
}

void Wrapped(Color color, const std::wstring& text, const std::wstring& indent, size_t width) {
    SetColor(color);
    for (const std::wstring& line : Wrap(text, width > indent.size() ? width - indent.size() : 40))
        Line(indent + line);
    SetColor(Color::Default);
}

std::wstring Pad(const std::wstring& text, size_t width) {
    if (text.size() >= width) return text + L" ";
    return text + std::wstring(width - text.size(), L' ');
}

std::vector<const Section*> AllSections() {
    std::vector<const Section*> all;
    for (const Section& section : Sections()) all.push_back(&section);
    return all;
}

std::wstring SectionIdList() {
    std::wstring list;
    for (const Section& section : Sections()) {
        if (!list.empty()) list += L", ";
        list += section.id;
    }
    return list;
}

std::wstring ValueText(const Tweak& tweak) {
    if (tweak.isString) return tweak.sz ? tweak.sz : L"";
    return std::to_wstring(tweak.dw);
}

// ------------------------------------------------------------- prompts

void WaitBeforeExit() {
    if (!g_allocated) return;
    WriteOut(L"\r\nPress Enter to close this window...");
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (!input || input == INVALID_HANDLE_VALUE)
        input = CreateFileW(L"CONIN$", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0,
                            nullptr);
    if (!input || input == INVALID_HANDLE_VALUE) return;
    wchar_t buffer[8];
    DWORD read = 0;
    ReadConsoleW(input, buffer, 7, &read, nullptr);
}

bool Confirm(const std::wstring& question) {
    WriteOut(question + L" [y/N] ");
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (!input || input == INVALID_HANDLE_VALUE)
        input = CreateFileW(L"CONIN$", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0,
                            nullptr);
    if (!input || input == INVALID_HANDLE_VALUE) {
        Line();
        Line(L"No interactive input available - pass -y to confirm.");
        return false;
    }

    wchar_t buffer[128] = {0};
    DWORD read = 0;
    BOOL ok = ReadConsoleW(input, buffer, 127, &read, nullptr);
    if (!ok) {
        DWORD bytes = 0;
        ok = ReadFile(input, buffer, sizeof(buffer) - sizeof(wchar_t), &bytes, nullptr);
        read = bytes / sizeof(wchar_t);
    }
    if (!ok || read == 0) {
        Line();
        Line(L"No answer received - pass -y to confirm non-interactively.");
        return false;
    }
    const std::wstring answer = Trim(std::wstring(buffer, read));
    Line();
    return !answer.empty() && (answer[0] == L'y' || answer[0] == L'Y');
}

// ------------------------------------------------------------- printing

void Banner(const std::wstring& subtitle) {
    SetColor(Color::Bold);
    Line(L"GamerTCP " + std::wstring(VersionString()) + L" - Windows 10/11 network optimization");
    SetColor(Color::Default);
    if (!subtitle.empty()) Line(subtitle);
    Line();
}

void PrintEntry(const Entry& entry) {
    const std::wstring prefix = entry.sub.empty() ? L"  " : L"  [" + entry.sub + L"] ";
    switch (entry.state) {
        case State::Changed:
            SetColor(Color::Cyan);
            WriteOut(prefix + entry.name + L": ");
            SetColor(Color::Gray);
            WriteOut(entry.oldData);
            SetColor(Color::Cyan);
            WriteOut(L" -> ");
            SetColor(Color::Green);
            Line(entry.newData);
            break;
        case State::Deleted:
            SetColor(Color::Cyan);
            WriteOut(prefix + entry.name + L": ");
            SetColor(Color::Gray);
            WriteOut(entry.oldData);
            SetColor(Color::Cyan);
            Line(L" -> (deleted)");
            break;
        case State::Failed:
            SetColor(Color::Red);
            Line(prefix + entry.name + L": FAILED - " + entry.error);
            break;
        default:
            SetColor(Color::Gray);
            Line(prefix + entry.name + L" = " + entry.newData + L" (unchanged)");
            break;
    }
    SetColor(Color::Default);
}

void PrintSummary(const Summary& summary) {
    Line();
    SetColor(summary.failed > 0 ? Color::Yellow : Color::Green);
    Line(L"Summary: " + std::to_wstring(summary.changed) + L" changed, " +
         std::to_wstring(summary.unchanged) + L" unchanged, " +
         std::to_wstring(summary.failed) + L" failed.");
    SetColor(Color::Default);
}

void PrintRebootNotice() {
    Line();
    SetColor(Color::Yellow);
    Line(L"A reboot is recommended so every change takes effect.");
    SetColor(Color::Default);
}

int UsageError(const std::wstring& message) {
    SetColor(Color::Red);
    Line(L"Error: " + message);
    SetColor(Color::Default);
    Line(L"Run \"GamerTCP help\" for usage.");
    return 1;
}

int ElevationError() {
    SetColor(Color::Red);
    Line(L"Administrator privileges are required to change these settings.");
    SetColor(Color::Default);
    Line(L"Run GamerTCP from an elevated command prompt (or use the GUI and click");
    Line(L"\"Restart as administrator\").");
    return 5;
}

// ------------------------------------------------------------- options

struct Options {
    std::vector<std::wstring> sectionIds;
    std::vector<std::wstring> positional;
    std::wstring              file;
    bool                      dryRun = false;
    bool                      noReset = false;
    bool                      withReset = false;
    bool                      noBackup = false;
    bool                      yes = false;
    bool                      allValues = false;
};

void AddSectionList(const std::wstring& text, Options& options) {
    for (const std::wstring& part : SplitList(text, L',')) {
        if (!part.empty()) options.sectionIds.push_back(part);
    }
}

bool ParseOptions(const std::vector<std::wstring>& args, size_t start, Options& options,
                  std::wstring* error) {
    for (size_t i = start; i < args.size(); ++i) {
        const std::wstring& arg = args[i];
        if (arg == L"-s" || arg == L"--section") {
            if (i + 1 >= args.size()) {
                if (error) *error = L"--section needs a section name";
                return false;
            }
            AddSectionList(args[++i], options);
        } else if (arg.rfind(L"--section=", 0) == 0) {
            AddSectionList(arg.substr(10), options);
        } else if (arg == L"-y" || arg == L"--yes") {
            options.yes = true;
        } else if (arg == L"-n" || arg == L"--dry-run") {
            options.dryRun = true;
        } else if (arg == L"--no-reset") {
            options.noReset = true;
        } else if (arg == L"--with-reset") {
            options.withReset = true;
        } else if (arg == L"--no-backup") {
            options.noBackup = true;
        } else if (arg == L"--all" || arg == L"--all-values") {
            options.allValues = true;
        } else if (arg == L"-o" || arg == L"--backup-file") {
            if (i + 1 >= args.size()) {
                if (error) *error = L"--backup-file needs a path";
                return false;
            }
            options.file = args[++i];
        } else if (arg.rfind(L"--backup-file=", 0) == 0) {
            options.file = arg.substr(14);
        } else if (!arg.empty() && arg[0] == L'-') {
            if (error) *error = L"unknown option: " + arg;
            return false;
        } else {
            options.positional.push_back(arg);
        }
    }
    return true;
}

bool ResolveSections(const Options& options, std::vector<const Section*>& out,
                     std::wstring* unknown) {
    out.clear();
    if (options.sectionIds.empty()) {
        for (const Section& section : Sections()) {
            if (section.defaultOn) out.push_back(&section);
        }
        return true;
    }
    for (const std::wstring& id : options.sectionIds) {
        const Section* section = FindSection(id);
        if (!section) {
            if (unknown) *unknown = id;
            return false;
        }
        if (std::find(out.begin(), out.end(), section) == out.end()) out.push_back(section);
    }
    return true;
}

int UnknownSectionError(const std::wstring& id) {
    SetColor(Color::Red);
    Line(L"Unknown section: " + id);
    SetColor(Color::Default);
    Wrapped(Color::Default, L"Available sections: " + SectionIdList(), L"  ", ConsoleWidth());
    return 1;
}

// ------------------------------------------------------------- commands

CommandSummary RunCommandList(const std::vector<std::wstring>& commands, bool dryRun,
                              const std::wstring& title, const std::wstring& label) {
    SetColor(Color::White);
    Line(title);
    SetColor(Color::Default);

    const CommandSummary summary =
        RunCommands(commands, dryRun, [](const CommandResult& result) {
            if (result.ok) {
                SetColor(Color::Gray);
                Line(L"  OK    " + result.command);
            } else {
                SetColor(Color::Red);
                Line(L"  FAIL  " + result.command);
                if (!result.output.empty()) {
                    for (const std::wstring& line : SplitList(result.output, L'\n'))
                        Line(L"          " + line);
                }
            }
            SetColor(Color::Default);
        });

    Line();
    SetColor(summary.failed > 0 ? Color::Yellow : Color::Green);
    Line(label + L": " + std::to_wstring(summary.succeeded) + L" succeeded, " +
         std::to_wstring(summary.failed) + L" failed.");
    SetColor(Color::Default);
    return summary;
}

// The 23-command reset, including the destructive parts. Only the explicit reset
// command and apply --with-reset reach this.
CommandSummary RunReset(bool dryRun) {
    return RunCommandList(ResetStackCommands(), dryRun,
                          dryRun ? L"Network stack reset (preview):"
                                 : L"Resetting network stack...",
                          L"Network stack reset");
}

// The global TCP settings only: no interface keys, no IP configuration.
CommandSummary RunTune(bool dryRun) {
    return RunCommandList(TuneCommands(), dryRun,
                          dryRun ? L"Global TCP settings (preview):"
                                 : L"Applying the global TCP settings...",
                          L"Global TCP settings");
}

int CommandApply(const Options& options, bool preview) {
    std::vector<const Section*> sections;
    std::wstring unknown;
    if (!ResolveSections(options, sections, &unknown)) return UnknownSectionError(unknown);
    if (!options.positional.empty())
        return UsageError(L"unexpected argument: " + options.positional[0]);

    const bool dryRun = preview || options.dryRun;
    if (options.withReset && options.noReset)
        return UsageError(L"--with-reset and --no-reset cannot be combined");
    if (!dryRun && !IsElevated()) return ElevationError();

    Banner(preview ? L"(preview - nothing will be written)"
                   : (dryRun ? L"(dry run - nothing will be written)" : L"(apply)"));
    Line(L"Administrator: " + std::wstring(IsElevated() ? L"yes" : L"no"));
    Line();

    if (!dryRun) {
        if (!options.yes) {
            const std::wstring question =
                options.withReset
                    ? L"Reset the network stack (this discards interface keys and static "
                      L"IP configuration) and apply " +
                          std::to_wstring(sections.size()) + L" section(s) of registry values?"
                    : L"Apply " + std::to_wstring(sections.size()) +
                          L" section(s) of registry values? The global TCP settings are "
                          L"re-applied; the destructive stack reset is not run.";
            if (!Confirm(question)) {
                Line(L"Cancelled.");
                return 0;
            }
        }

        if (!options.noBackup) {
            const std::wstring file =
                options.file.empty() ? DefaultBackupPath() : options.file;
            std::wstring error;
            if (BackupSections(sections, file, &error)) {
                ColoredLine(Color::Gray, L"Backup written: " + file);
            } else {
                ColoredLine(Color::Yellow, L"WARNING: could not write the backup (" + error +
                                               L"); continuing without it.");
            }
            Line();
        }
    }

    // The destructive stack reset is opt-in (--with-reset): it is equivalent to
    // removing and reinstalling TCP/IP, so it throws away interface keys, static
    // addresses and DHCP settings. When it is asked for it runs the full original
    // set, resets first and global settings second, before the writes.
    CommandSummary reset;
    CommandSummary tune;
    if (!dryRun && options.withReset) {
        reset = RunReset(false);
        Line();
    } else if (!dryRun) {
        ColoredLine(Color::Gray,
                    options.noReset
                        ? L"No netsh commands run (--no-reset)."
                        : L"Network stack reset skipped: it discards interface and static IP "
                          L"configuration. Use \"GamerTCP reset\" or --with-reset for it.");
        Line();

        // The global TCP settings are safe: they set parameters and flush caches
        // without removing any configuration.
        if (!options.noReset) {
            tune = RunTune(false);
            Line();
        }
    }

    ApplyOptions applyOptions;
    applyOptions.dryRun = dryRun;

    // Collect first, then verify: every value that was written is read back, so
    // one that does not stick is reported as a failure and never as a change.
    std::vector<Entry> entries;
    for (const Section* section : sections) {
        ApplySections({section}, applyOptions,
                      [&](const Entry& entry) { entries.push_back(entry); });
    }

    if (!dryRun) {
        const int reverted = VerifyEntries(entries);
        if (reverted > 0) {
            ColoredLine(Color::Red, L"Verification: " + std::to_wstring(reverted) +
                                        L" value(s) did not keep the value that was written.");
        }
    }

    Summary total;
    std::wstring lastSection;
    for (const Entry& entry : entries) {
        if (entry.section != lastSection) {
            SetColor(Color::White);
            Line(L"[" + entry.section + L"]");
            SetColor(Color::Default);
            lastSection = entry.section;
        }
        PrintEntry(entry);
        total.Add(entry);
    }

    PrintSummary(total);

    if (!dryRun) PrintRebootNotice();
    return (total.failed > 0 || reset.failed > 0 || tune.failed > 0) ? 2 : 0;
}

int CommandStatus(const Options& options) {
    std::vector<const Section*> sections;
    std::wstring unknown;
    if (!ResolveSections(options, sections, &unknown)) return UnknownSectionError(unknown);
    if (!options.positional.empty())
        return UsageError(L"unexpected argument: " + options.positional[0]);

    Banner(L"(current values)");

    std::vector<ScanRow> rows;
    const Summary summary = ScanSections(sections, rows);
    const int width = ConsoleWidth();

    std::wstring lastSection;
    for (const ScanRow& row : rows) {
        if (row.matches && !options.allValues) continue;
        if (row.section != lastSection) {
            Line();
            SetColor(Color::White);
            Line(L"[" + row.section + L"]");
            SetColor(Color::Default);
            lastSection = row.section;
        }

        const std::wstring name =
            row.sub.empty() ? row.name : L"[" + row.sub + L"] " + row.name;
        const std::wstring left = Pad(L"  " + name, 52);
        if (row.failed) {
            ColoredLine(Color::Red, left + L"= unreadable");
        } else if (row.matches) {
            ColoredLine(Color::Gray, left + L"= " + row.current + L"  (matches target)");
        } else {
            SetColor(Color::Yellow);
            Line(left + L"= " + row.current + L"  -> target " + row.target);
            SetColor(Color::Default);
        }
        (void)width;
    }

    Line();
    const int total = summary.changed + summary.unchanged + summary.failed;
    SetColor(summary.failed > 0 ? Color::Yellow : Color::Green);
    Line(L"Values: " + std::to_wstring(total) + L", matching: " +
         std::to_wstring(summary.unchanged) + L", differing: " +
         std::to_wstring(summary.changed) + L", failed: " + std::to_wstring(summary.failed));
    SetColor(Color::Default);

    if (summary.changed > 0) {
        Line(L"Run \"GamerTCP apply\" from an elevated prompt to write the targets.");
    }
    return summary.failed > 0 ? 2 : 0;
}

int CommandList(const Options& options) {
    if (!options.positional.empty())
        return UsageError(L"unexpected argument: " + options.positional[0]);

    const int width = ConsoleWidth();
    Banner(L"(tweak catalogue)");

    for (const Section& section : Sections()) {
        SetColor(Color::Bold);
        std::wstring header = std::wstring(section.id) + L"  -  " + section.name + L"  (" +
                              std::to_wstring(section.count);
        header += section.mode == SectionMode::Fixed ? L" values)" : L" values per interface)";
        Line(header);
        SetColor(Color::Default);

        Wrapped(Color::Gray, section.summary, L"  ", width);

        std::wstring lastPath;
        for (int i = 0; i < section.count; ++i) {
            const Tweak& tweak = section.tweaks[i];
            if (tweak.path != lastPath) {
                lastPath = tweak.path;
                ColoredLine(Color::Gray, L"  key:  " + DisplayPath(tweak.root, tweak.path));
            }
            const std::wstring left =
                Pad(L"    " + std::wstring(tweak.name) + L" = " + ValueText(tweak), 54);
            if (tweak.note && *tweak.note) {
                Line(left + tweak.note);
            } else {
                Line(left);
            }
        }
        Line();
    }
    return 0;
}

int CommandBackup(const Options& options) {
    std::vector<const Section*> sections;
    std::wstring unknown;
    if (!ResolveSections(options, sections, &unknown)) return UnknownSectionError(unknown);

    std::wstring file = options.file;
    if (file.empty() && !options.positional.empty()) file = options.positional[0];
    if (file.empty()) file = DefaultBackupPath();

    Banner(L"(backup)");

    std::wstring error;
    if (!BackupSections(sections, file, &error)) {
        ColoredLine(Color::Red, L"Backup failed: " + error);
        return 2;
    }

    std::vector<BackupRow> rows;
    std::wstring readError;
    ReadBackupFile(file, rows, &readError);

    ColoredLine(Color::Green, L"Backup written: " + file);
    Line(L"Recorded " + std::to_wstring(rows.size()) + L" registry value(s).");
    if (!readError.empty()) ColoredLine(Color::Gray, L"Note: " + readError);
    return 0;
}

int CommandRestore(const Options& options) {
    if (options.positional.empty() && options.file.empty())
        return UsageError(L"restore needs a backup file, for example: GamerTCP restore "
                          L"backup.txt");

    const std::wstring file = options.file.empty() ? options.positional[0] : options.file;
    if (!IsElevated()) return ElevationError();

    Banner(L"(restore)");
    Line(L"Backup file: " + file);

    std::wstring error;
    std::vector<BackupRow> rows;
    if (!ReadBackupFile(file, rows, &error)) {
        ColoredLine(Color::Red, L"Cannot read the backup: " + error);
        return 2;
    }
    Line(L"Entries: " + std::to_wstring(rows.size()));
    Line();

    if (!options.yes && !Confirm(L"Restore every value listed in this backup?")) {
        Line(L"Cancelled.");
        return 0;
    }

    const std::wstring safety = DefaultBackupPath();
    std::wstring safetyError;
    if (BackupSections(AllSections(), safety, &safetyError)) {
        ColoredLine(Color::Gray, L"Pre-restore backup: " + safety);
    } else {
        ColoredLine(Color::Yellow, L"Warning: pre-restore backup failed (" + safetyError + L").");
    }
    Line();

    Summary summary;
    if (!RestoreBackup(file, [](const Entry& entry) { PrintEntry(entry); }, &summary, &error)) {
        ColoredLine(Color::Red, L"Restore failed: " + error);
        return 2;
    }

    PrintSummary(summary);
    PrintRebootNotice();
    return summary.failed > 0 ? 2 : 0;
}

int CommandReset(const Options& options) {
    if (!options.dryRun && !IsElevated()) return ElevationError();
    if (!options.positional.empty())
        return UsageError(L"unexpected argument: " + options.positional[0]);

    Banner(options.dryRun ? L"(reset preview)" : L"(network stack reset)");
    if (!options.dryRun && !options.yes &&
        !Confirm(L"Reset the TCP/IP stack and Winsock catalog now?")) {
        Line(L"Cancelled.");
        return 0;
    }

    const CommandSummary summary = RunReset(options.dryRun);
    if (!options.dryRun) PrintRebootNotice();
    return summary.failed > 0 ? 2 : 0;
}

int CommandTeredo(const Options& options) {
    const std::wstring action =
        options.positional.empty() ? L"show" : Lower(options.positional[0]);

    Banner(L"(teredo tunneling)");

    if (action == L"show" || action == L"status") {
        bool ok = false;
        std::wstring output;
        TeredoShowState(&output, &ok);
        if (output.empty()) {
            ColoredLine(Color::Yellow, L"No Teredo information returned.");
        } else {
            for (const std::wstring& line : SplitList(output, L'\n')) Line(L"  " + line);
        }
        if (!ok) {
            Line();
            ColoredLine(Color::Yellow, L"Could not query the Teredo state (netsh returned an "
                                        L"error, or Teredo is not available).");
        }
        return ok ? 0 : 2;
    }

    std::wstring netsh;
    if (action == L"disable" || action == L"disabled")
        netsh = L"disabled";
    else if (action == L"default")
        netsh = L"type=default";
    else if (action == L"enterprise" || action == L"enterpriseclient")
        netsh = L"enterpriseclient";
    else
        return UsageError(L"unknown Teredo state \"" + action +
                          L"\" (use show, disable, default or enterprise)");

    if (!IsElevated()) return ElevationError();
    if (!options.yes &&
        !Confirm(L"Set the Teredo state to \"" + action + L"\"?")) {
        Line(L"Cancelled.");
        return 0;
    }

    std::wstring output;
    DWORD code = 1;
    const bool ok = TeredoSetState(netsh, &output, &code);
    if (!output.empty()) {
        for (const std::wstring& line : SplitList(output, L'\n')) Line(L"  " + line);
    }
    Line();
    if (ok) {
        ColoredLine(Color::Green, L"Teredo state updated.");
    } else {
        ColoredLine(Color::Red, L"Failed to update the Teredo state (exit code " +
                                    std::to_wstring(code) + L").");
    }
    return ok ? 0 : 2;
}

// ------------------------------------------------------------- changelog

std::wstring ExeDirectory() {
    wchar_t module[MAX_PATH * 2] = {0};
    const DWORD length = GetModuleFileNameW(
        nullptr, module, static_cast<DWORD>(sizeof(module) / sizeof(module[0])));
    const std::wstring path(module, length);
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring(L".") : path.substr(0, slash);
}

bool ReadTextFile(const std::wstring& file, std::string* text) {
    HANDLE handle = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;

    text->clear();
    LARGE_INTEGER size{};
    if (GetFileSizeEx(handle, &size) && size.QuadPart > 0 && size.QuadPart <= (1 << 20)) {
        text->resize(static_cast<size_t>(size.QuadPart));
        DWORD read = 0;
        if (ReadFile(handle, &(*text)[0], static_cast<DWORD>(text->size()), &read, nullptr))
            text->resize(read);
        else
            text->clear();
    }
    CloseHandle(handle);
    return !text->empty();
}

// Newest release heading in CHANGES.md, for example "4.1.0" from "## [4.1.0] - ...".
bool NewestChangelogVersion(const std::wstring& file, std::wstring* version) {
    std::string text;
    if (!ReadTextFile(file, &text)) return false;

    size_t pos = 0;
    while (pos < text.size()) {
        const size_t end = text.find('\n', pos);
        const std::string line =
            text.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        if (line.compare(0, 4, "## [") == 0) {
            const size_t close = line.find(']', 4);
            if (close != std::string::npos) {
                const std::string value = line.substr(4, close - 4);
                if (!value.empty() &&
                    value.find_first_not_of("0123456789.") == std::string::npos) {
                    version->assign(value.begin(), value.end());
                    return true;
                }
            }
        }
        if (end == std::string::npos) break;
        pos = end + 1;
    }
    return false;
}

bool FindChangelog(std::wstring* file, std::wstring* version) {
    const std::wstring directory = ExeDirectory();
    const std::wstring candidates[] = {
        directory + L"\\CHANGES.md",
        directory + L"\\..\\CHANGES.md",
        std::wstring(L"CHANGES.md"),
    };
    for (const std::wstring& candidate : candidates) {
        if (NewestChangelogVersion(candidate, version)) {
            *file = candidate;
            return true;
        }
    }
    return false;
}

int CommandSelfTest() {
    Banner(L"(self test - read only)");
    int failures = 0;

    auto check = [&](bool ok, const std::wstring& label) {
        SetColor(ok ? Color::Green : Color::Red);
        Line(std::wstring(ok ? L"  PASS  " : L"  FAIL  ") + label);
        SetColor(Color::Default);
        if (!ok) ++failures;
    };

    int fixedValues = 0;
    int tableValues = 0;
    for (const Section& section : Sections()) {
        tableValues += section.count;
        if (section.mode == SectionMode::Fixed) fixedValues += section.count;
    }
    check(Sections().size() == 15,
          L"sections: expected 15, got " + std::to_wstring(Sections().size()));
    check(fixedValues == 97, L"fixed values: expected 97, got " + std::to_wstring(fixedValues));
    check(tableValues == 106, L"table values: expected 106, got " + std::to_wstring(tableValues));

    const Section* tcpip = FindSection(L"tcpip");
    check(tcpip != nullptr && std::wstring(tcpip->id) == L"tcpip", L"FindSection by id");
    const Section* games = FindSection(L"GAMES TASK PROFILE");
    check(games != nullptr && std::wstring(games->id) == L"games",
          L"FindSection by name (case insensitive)");
    check(FindSection(L"does-not-exist") == nullptr, L"unknown section rejected");

    check(Lower(L"GaMeS") == L"games", L"Lower()");
    check(Trim(L"  padded  ") == L"padded", L"Trim()");
    const std::vector<std::wstring> parts = SplitList(L"a, b,c", L',');
    check(parts.size() == 3 && parts[0] == L"a" && parts[1] == L"b" && parts[2] == L"c",
          L"SplitList()");

    check(ResetStackCommands().size() == 23, L"reset commands: expected 23");
    check(StackResetCommands().size() == 5, L"destructive reset commands: expected 5");
    check(TuneCommands().size() == 18, L"global TCP commands: expected 18");
    bool tuneIsSafe = true;
    for (const std::wstring& command : TuneCommands()) {
        if (Lower(command).find(L" reset") != std::wstring::npos) tuneIsSafe = false;
    }
    check(tuneIsSafe, L"apply never runs one of the reset commands");
    int teredoCount = 0;
    TeredoOptions(&teredoCount);
    check(teredoCount == 3, L"Teredo options: expected 3");

    // The binary and the changelog must agree on the current release. The check
    // is skipped when CHANGES.md is not shipped alongside the executable.
    std::wstring changelogFile;
    std::wstring changelogVersion;
    if (FindChangelog(&changelogFile, &changelogVersion)) {
        const bool matches = changelogVersion == VersionString();
        check(matches, matches ? L"version " + std::wstring(VersionString()) +
                                     L" matches the newest CHANGES.md release"
                               : L"version mismatch: binary is " +
                                     std::wstring(VersionString()) +
                                     L", newest CHANGES.md release is [" + changelogVersion +
                                     L"] (" + changelogFile + L")");
    } else {
        Line(L"  info  version check skipped (CHANGES.md not found)");
    }

    std::wstring output;
    DWORD code = 1;
    const bool captured = RunAndCapture(L"cmd.exe /c echo gamerTCP-selftest", &output, &code);
    check(captured && code == 0 && output.find(L"gamerTCP-selftest") != std::wstring::npos,
          L"child process output capture");

    const CommandSummary dry = RunCommands(ResetStackCommands(), true, CommandSink());
    check(dry.succeeded == 23 && dry.failed == 0, L"reset command dry run");
    const CommandSummary dryTune = RunCommands(TuneCommands(), true, CommandSink());
    check(dryTune.succeeded == 18 && dryTune.failed == 0, L"global TCP command dry run");

    const OldValue product =
        ReadValue(Root::LocalMachine, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                  L"ProductName");
    check(product.exists && !product.text.empty(), L"registry read (Windows ProductName)");

    check(!FormatWin32Error(ERROR_ACCESS_DENIED).empty(), L"FormatWin32Error()");

    wchar_t temp[MAX_PATH] = {0};
    const DWORD tempLength =
        GetTempPathW(static_cast<DWORD>(sizeof(temp) / sizeof(temp[0])), temp);
    const std::wstring file =
        (tempLength > 0 ? std::wstring(temp) : std::wstring(L".\\")) + L"gamerTCP-selftest.txt";

    std::wstring error;
    const bool writtenBackup = BackupSections(AllSections(), file, &error);
    std::vector<BackupRow> rows;
    const bool parsedBackup = writtenBackup && ReadBackupFile(file, rows, &error);
    check(writtenBackup, L"backup file written");
    check(parsedBackup && rows.size() >= 97,
          L"backup round trip (" + std::to_wstring(rows.size()) + L" entries)");
    DeleteFileW(file.c_str());

    // Per-value filtering (the GUI tree selection): with a filter, only the
    // chosen value may be scanned, backed up or applied.
    const Section* filterSection = FindSection(L"games");
    if (filterSection) {
        const Tweak* chosen = &filterSection->tweaks[0];
        const TweakFilter onlyChosen = [chosen](const Section&, const Tweak& tweak) {
            return &tweak == chosen;
        };

        std::vector<ScanRow> filteredRows;
        ScanSections({filterSection}, filteredRows, onlyChosen);
        check(filteredRows.size() == 1 && filteredRows[0].name == chosen->name,
              L"per-value filter (scan)");

        const std::wstring filterFile = file + L".filtered";
        const bool filteredBackup =
            BackupSections({filterSection}, filterFile, &error, onlyChosen);
        std::vector<BackupRow> filteredBackupRows;
        const bool filteredParsed =
            filteredBackup && ReadBackupFile(filterFile, filteredBackupRows, &error);
        check(filteredParsed && filteredBackupRows.size() == 1,
              L"per-value filter (backup)");
        DeleteFileW(filterFile.c_str());

        ApplyOptions filterOptions;
        filterOptions.dryRun = true;
        filterOptions.filter = onlyChosen;
        int applied = 0;
        ApplySections({filterSection}, filterOptions, [&applied](const Entry&) { ++applied; });
        check(applied == 1, L"per-value filter (apply preview)");
    }

    const OldValue sample = ReadValue(Root::LocalMachine,
                                      L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                                      L"DefaultTTL");
    Line(L"  info  current Tcpip\\Parameters\\DefaultTTL is " + sample.Display());

    std::vector<ScanRow> scanRows;
    const Summary scan = ScanSections(AllSections(), scanRows);
    check(scanRows.size() >= 97,
          L"scan rows: " + std::to_wstring(scanRows.size()) + L" (" +
              std::to_wstring(scan.unchanged) + L" matching)");

    Line();
    if (failures == 0) {
        ColoredLine(Color::Green, L"All self checks passed.");
        return 0;
    }
    ColoredLine(Color::Red, std::to_wstring(failures) + L" self check(s) failed.");
    return 1;
}

void PrintVersion() {
    Line(L"GamerTCP " + std::wstring(VersionString()));
    Line(CopyrightString());
}

void PrintHelp() {
    const int width = ConsoleWidth();
    SetColor(Color::Bold);
    Line(L"GamerTCP " + std::wstring(VersionString()) +
         L" - Windows 10/11 network optimization toolkit");
    SetColor(Color::Default);
    Wrapped(Color::Gray,
            L"Native C++ reimplementation of the GameTCP3 tweak set: 106 registry values "
            L"across 15 sections, recursive per-interface configuration, a network stack reset "
            L"and Teredo control.",
            L"", width);
    Line();
    Line(L"Usage:");
    Line(L"  GamerTCP                        Launch the graphical interface");
    Line(L"  GamerTCP <command> [options]");
    Line();
    Line(L"Commands:");
    Line(L"  apply      Apply the registry tweaks after the global TCP settings");
    Line(L"  preview    Show what apply would change without writing anything");
    Line(L"  status     Show current values against their targets");
    Line(L"  list       List every section and registry value in the toolkit");
    Line(L"  backup     Save current values to a backup file");
    Line(L"  restore    Restore values from a backup file");
    Line(L"  reset      Reset the TCP/IP stack and Winsock catalog (destructive)");
    Line(L"  teredo     Show or change the Teredo tunneling state");
    Line(L"  selftest   Run built-in self checks (read only)");
    Line();
    Line(L"Options:");
    Line(L"  -s, --section <id>[,<id>]  Limit to these sections (repeatable)");
    Line(L"  -y, --yes                  Do not prompt for confirmation");
    Line(L"  -n, --dry-run              Preview without writing (apply, reset)");
    Line(L"      --no-reset             Skip the global TCP settings as well");
    Line(L"      --with-reset           Also reset the TCP/IP stack first; that discards");
    Line(L"                             interface keys and static IP configuration");
    Line(L"      --no-backup            Skip the automatic backup");
    Line(L"      --all-values           status: also show values that already match");
    Line(L"  -o, --backup-file <path>   Backup destination");
    Line(L"  -h, --help                 Show this help");
    Line(L"  -v, --version              Show version information");
    Line();
    Wrapped(Color::Default, L"Sections: " + SectionIdList(), L"  ", width);
    Line();
    Line(L"Teredo states:");
    Line(L"  show | disable | default | enterprise");
    Line();
    Line(L"Examples:");
    Line(L"  GamerTCP preview");
    Line(L"  GamerTCP apply -s tcpip,games");
    Line(L"  GamerTCP status -s games --all-values");
    Line(L"  GamerTCP backup D:\\backups\\before-gamertcp.txt");
    Line(L"  GamerTCP restore D:\\backups\\before-gamertcp.txt");
}

}  // namespace

int RunCli(int argc, wchar_t* const* argv) {
    if (!InitOutput()) return 1;

    std::vector<std::wstring> args;
    for (int i = 1; i < argc; ++i) {
        if (argv[i]) args.push_back(argv[i]);
    }

    int code = 0;
    if (args.empty()) {
        PrintHelp();
    } else {
        const std::wstring command = Lower(args[0]);
        Options options;
        std::wstring error;

        if (command == L"help" || command == L"-h" || command == L"--help" || command == L"-?" ||
            command == L"/?") {
            PrintHelp();
        } else if (command == L"version" || command == L"-v" || command == L"--version") {
            PrintVersion();
        } else if (command == L"apply" || command == L"preview") {
            if (!ParseOptions(args, 1, options, &error))
                code = UsageError(error);
            else
                code = CommandApply(options, command == L"preview");
        } else if (command == L"status") {
            if (!ParseOptions(args, 1, options, &error))
                code = UsageError(error);
            else
                code = CommandStatus(options);
        } else if (command == L"list") {
            if (!ParseOptions(args, 1, options, &error))
                code = UsageError(error);
            else
                code = CommandList(options);
        } else if (command == L"backup") {
            if (!ParseOptions(args, 1, options, &error))
                code = UsageError(error);
            else
                code = CommandBackup(options);
        } else if (command == L"restore") {
            if (!ParseOptions(args, 1, options, &error))
                code = UsageError(error);
            else
                code = CommandRestore(options);
        } else if (command == L"reset") {
            if (!ParseOptions(args, 1, options, &error))
                code = UsageError(error);
            else
                code = CommandReset(options);
        } else if (command == L"teredo") {
            if (!ParseOptions(args, 1, options, &error))
                code = UsageError(error);
            else
                code = CommandTeredo(options);
        } else if (command == L"selftest") {
            if (!ParseOptions(args, 1, options, &error))
                code = UsageError(error);
            else
                code = CommandSelfTest();
        } else if (!command.empty() && command[0] == L'-') {
            code = UsageError(L"unknown option: " + args[0]);
        } else {
            code = UsageError(L"unknown command: " + args[0]);
        }
    }

    WaitBeforeExit();
    return code;
}

}  // namespace gtc
