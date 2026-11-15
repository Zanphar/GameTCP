// GamerTCP 4 -- per-user installer and uninstaller.
//
// A single console executable that carries GamerTCP.exe and its documentation
// as resources. It installs into %LOCALAPPDATA%\Programs\GamerTCP4, adds a Start
// Menu shortcut and an entry in Settings > Apps. No administrator rights and no
// third-party installer framework are involved.
//
//   GamerTCP-Setup.exe                     install (asks first)
//   GamerTCP-Setup.exe --yes               install without prompting
//   GamerTCP-Setup.exe --dir <path>        install somewhere else
//   GamerTCP-Setup.exe --uninstall --yes   remove an existing installation

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>

#include <string>

namespace {

const wchar_t* const kAppName = L"GamerTCP 4";
const wchar_t* const kDisplayVersion = L"4.2.0";
const wchar_t* const kPublisher = L"GamerTCP contributors";
const wchar_t* const kUninstallKey =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\GamerTCP4";
const wchar_t* const kShortcutName = L"GamerTCP 4.lnk";

enum : int {
    kResourceApp = 101,
    kResourceReadme = 102,
    kResourceChanges = 103,
    kResourceLicense = 104,
};

// ---------------------------------------------------------------- output

void Say(const std::wstring& text) {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!out || out == INVALID_HANDLE_VALUE) return;

    DWORD written = 0;
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode)) {
        WriteConsoleW(out, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
        return;
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                         nullptr, 0, nullptr, nullptr);
    if (size <= 0) return;
    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), &utf8[0], size,
                        nullptr, nullptr);
    WriteFile(out, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
}

void Line(const std::wstring& text = std::wstring()) { Say(text + L"\r\n"); }

bool Ask(const std::wstring& question) {
    Say(question + L" [Y/n] ");
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    if (!in || in == INVALID_HANDLE_VALUE) return true;

    wchar_t buffer[16] = {0};
    DWORD read = 0;
    if (!ReadConsoleW(in, buffer, 15, &read, nullptr)) return true;
    const wchar_t first = buffer[0];
    return first == 0 || first == L'\r' || first == L'\n' || first == L'y' || first == L'Y';
}

// ---------------------------------------------------------------- paths

std::wstring KnownFolder(int csidl) {
    wchar_t path[MAX_PATH] = {0};
    if (FAILED(SHGetFolderPathW(nullptr, csidl | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT,
                                path)))
        return std::wstring();
    return path;
}

std::wstring ExecutablePath() {
    wchar_t module[MAX_PATH * 2] = {0};
    const DWORD length = GetModuleFileNameW(
        nullptr, module, static_cast<DWORD>(sizeof(module) / sizeof(module[0])));
    return std::wstring(module, length);
}

std::wstring ParentOf(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring(L".") : path.substr(0, slash);
}

std::wstring AbsolutePath(const std::wstring& path) {
    wchar_t full[MAX_PATH * 2] = {0};
    const DWORD length = GetFullPathNameW(path.c_str(), static_cast<DWORD>(sizeof(full) /
                                                                           sizeof(full[0])),
                                          full, nullptr);
    return length > 0 && length < sizeof(full) / sizeof(full[0]) ? std::wstring(full) : path;
}

std::wstring DefaultInstallDirectory() {
    const std::wstring local = KnownFolder(CSIDL_LOCAL_APPDATA);
    return (local.empty() ? std::wstring(L".") : local) + L"\\Programs\\GamerTCP4";
}

std::wstring StartMenuShortcut() {
    const std::wstring programs = KnownFolder(CSIDL_PROGRAMS);
    return (programs.empty() ? std::wstring(L".") : programs) + L"\\" + kShortcutName;
}

// ---------------------------------------------------------------- resources

bool ExtractResource(HINSTANCE instance, int id, const std::wstring& file, std::wstring* error) {
    HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!resource) {
        *error = L"embedded file " + std::to_wstring(id) + L" is missing";
        return false;
    }
    const DWORD size = SizeofResource(instance, resource);
    const void* data = LockResource(LoadResource(instance, resource));
    if (!data || size == 0) {
        *error = L"embedded file " + std::to_wstring(id) + L" cannot be read";
        return false;
    }

    HANDLE handle = CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        *error = L"cannot write " + file + L" (" + std::to_wstring(GetLastError()) + L")";
        return false;
    }
    DWORD written = 0;
    const bool ok = WriteFile(handle, data, size, &written, nullptr) != FALSE && written == size;
    CloseHandle(handle);
    if (!ok) {
        *error = L"cannot write " + file;
        DeleteFileW(file.c_str());
    }
    return ok;
}

// ---------------------------------------------------------------- shell

bool CreateShortcut(const std::wstring& link, const std::wstring& target,
                    const std::wstring& directory, const std::wstring& description) {
    IShellLinkW* shellLink = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                                reinterpret_cast<void**>(&shellLink))))
        return false;

    shellLink->SetPath(target.c_str());
    shellLink->SetWorkingDirectory(directory.c_str());
    shellLink->SetDescription(description.c_str());
    shellLink->SetIconLocation(target.c_str(), 0);

    IPersistFile* persist = nullptr;
    bool saved = false;
    if (SUCCEEDED(shellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&persist)))) {
        saved = SUCCEEDED(persist->Save(link.c_str(), TRUE));
        persist->Release();
    }
    shellLink->Release();
    return saved;
}

bool WriteUninstallEntry(const std::wstring& directory, const std::wstring& uninstaller) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kUninstallKey, 0, nullptr, 0, KEY_WRITE, nullptr, &key,
                        nullptr) != ERROR_SUCCESS)
        return false;

    auto setText = [&](const wchar_t* name, const std::wstring& value) {
        RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()),
                       static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
    };

    setText(L"DisplayName", kAppName);
    setText(L"DisplayVersion", kDisplayVersion);
    setText(L"Publisher", kPublisher);
    setText(L"InstallLocation", directory);
    setText(L"DisplayIcon", directory + L"\\GamerTCP.exe");
    setText(L"UninstallString", L"\"" + uninstaller + L"\" --uninstall");
    setText(L"Comments", L"Windows 10/11 network optimization toolkit");

    const DWORD one = 1;
    RegSetValueExW(key, L"NoModify", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&one),
                   sizeof(one));
    RegSetValueExW(key, L"NoRepair", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&one),
                   sizeof(one));
    RegCloseKey(key);
    return true;
}

// ---------------------------------------------------------------- install

bool Install(const std::wstring& directory, const std::wstring& source, bool quiet) {
    std::wstring error;
    if (SHCreateDirectoryExW(nullptr, directory.c_str(), nullptr) != ERROR_SUCCESS &&
        GetFileAttributesW(directory.c_str()) == INVALID_FILE_ATTRIBUTES) {
        Line(L"  FAILED  cannot create " + directory);
        return false;
    }

    const std::wstring app = directory + L"\\GamerTCP.exe";
    const std::wstring uninstaller = directory + L"\\Uninstall.exe";

    if (!ExtractResource(GetModuleHandleW(nullptr), kResourceApp, app, &error) ||
        !ExtractResource(GetModuleHandleW(nullptr), kResourceReadme, directory + L"\\README.md",
                         &error) ||
        !ExtractResource(GetModuleHandleW(nullptr), kResourceChanges,
                         directory + L"\\CHANGES.md", &error) ||
        !ExtractResource(GetModuleHandleW(nullptr), kResourceLicense,
                         directory + L"\\LICENSE.md", &error)) {
        Line(L"  FAILED  " + error);
        return false;
    }
    Line(L"  OK      " + app);

    if (!CopyFileW(source.c_str(), uninstaller.c_str(), FALSE)) {
        Line(L"  FAILED  cannot write " + uninstaller);
        return false;
    }

    const std::wstring shortcut = StartMenuShortcut();
    if (CreateShortcut(shortcut, app, directory, L"GamerTCP 4 - network optimization")) {
        Line(L"  OK      " + shortcut);
    } else {
        Line(L"  WARNING  could not create the Start Menu shortcut");
    }

    if (WriteUninstallEntry(directory, uninstaller)) {
        Line(L"  OK      uninstall entry (Settings > Apps)");
    } else {
        Line(L"  WARNING  could not register the uninstall entry");
    }

    if (!quiet) {
        Line();
        Line(L"GamerTCP 4 is installed. Start it from the Start Menu,");
        Line(L"or remove it later from Settings > Apps.");
    }
    return true;
}

// ---------------------------------------------------------------- uninstall

std::wstring InstalledDirectory() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kUninstallKey, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        wchar_t value[MAX_PATH * 2] = {0};
        DWORD size = sizeof(value);
        DWORD type = 0;
        const LONG status =
            RegQueryValueExW(key, L"InstallLocation", nullptr, &type,
                             reinterpret_cast<BYTE*>(value), &size);
        RegCloseKey(key);
        if (status == ERROR_SUCCESS && type == REG_SZ && value[0]) return value;
    }
    return ParentOf(ExecutablePath());
}

bool Uninstall(bool quiet) {
    const std::wstring directory = InstalledDirectory();
    const std::wstring shortcut = StartMenuShortcut();

    if (DeleteFileW(shortcut.c_str()))
        Line(L"  OK      removed " + shortcut);
    else if (GetFileAttributesW(shortcut.c_str()) != INVALID_FILE_ATTRIBUTES)
        Line(L"  WARNING  could not remove " + shortcut);

    RegDeleteTreeW(HKEY_CURRENT_USER, kUninstallKey);
    Line(L"  OK      removed the uninstall entry");

    const wchar_t* const files[] = {L"\\GamerTCP.exe", L"\\README.md", L"\\CHANGES.md",
                                    L"\\LICENSE.md"};
    for (const wchar_t* name : files) {
        const std::wstring file = directory + name;
        if (!DeleteFileW(file.c_str()) && GetFileAttributesW(file.c_str()) != INVALID_FILE_ATTRIBUTES)
            Line(L"  WARNING  could not delete " + file);
    }
    Line(L"  OK      removed the installed files");

    if (!quiet) Line();
    Line(L"GamerTCP 4 has been removed.");

    // The running uninstaller cannot delete itself, so the last cleanup runs in
    // a detached command that removes the directory once this process exits.
    const std::wstring self = ExecutablePath();
    const bool insideTarget = _wcsicmp(ParentOf(self).c_str(), directory.c_str()) == 0;
    if (insideTarget) {
        std::wstring command = L"cmd.exe /c ping -n 3 127.0.0.1 >nul & rd /s /q \"" + directory +
                               L"\"";
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION process{};
        if (CreateProcessW(nullptr, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                           nullptr, nullptr, &startup, &process)) {
            CloseHandle(process.hThread);
            CloseHandle(process.hProcess);
        }
    } else if (RemoveDirectoryW(directory.c_str())) {
        Line(L"  OK      removed " + directory);
    }
    return true;
}

// ---------------------------------------------------------------- main

int Run() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 1;

    bool uninstall = false;
    bool assumeYes = false;
    bool quiet = false;
    std::wstring directory;

    for (int i = 1; i < argc; ++i) {
        const std::wstring argument = argv[i];
        if (argument == L"--uninstall" || argument == L"/uninstall")
            uninstall = true;
        else if (argument == L"--yes" || argument == L"/yes" || argument == L"/S")
            assumeYes = true;
        else if (argument == L"--quiet" || argument == L"/quiet")
            quiet = true;
        else if ((argument == L"--dir" || argument == L"/dir") && i + 1 < argc)
            directory = AbsolutePath(argv[++i]);
        else if (argument == L"--help" || argument == L"/?") {
            Line(L"GamerTCP 4 installer");
            Line();
            Line(L"  GamerTCP-Setup.exe                     install for this user");
            Line(L"  GamerTCP-Setup.exe --yes               install without prompting");
            Line(L"  GamerTCP-Setup.exe --dir <path>        install to another folder");
            Line(L"  GamerTCP-Setup.exe --uninstall --yes   remove the installation");
            LocalFree(argv);
            return 0;
        }
    }
    LocalFree(argv);

    if (quiet) assumeYes = true;

    Line();
    Line(std::wstring(kAppName) + L" installer - version " + kDisplayVersion);
    Line(L"Windows 10/11 network optimization toolkit - BSD 3-Clause");
    Line();

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    int result = 0;
    if (uninstall) {
        const std::wstring installed = InstalledDirectory();
        Line(L"Removing GamerTCP 4 from:");
        Line(L"  " + installed);
        Line();
        if (!assumeYes && !Ask(L"Continue?")) {
            Line(L"Cancelled.");
            result = 2;
        } else {
            result = Uninstall(quiet) ? 0 : 1;
        }
    } else {
        if (directory.empty()) directory = DefaultInstallDirectory();
        Line(L"Installing to:");
        Line(L"  " + directory);
        Line();
        if (!assumeYes && !Ask(L"Install GamerTCP 4 for this user?")) {
            Line(L"Cancelled.");
            result = 2;
        } else {
            result = Install(directory, ExecutablePath(), quiet) ? 0 : 1;
        }
    }

    CoUninitialize();

    if (!quiet) {
        Line();
        Say(L"Press Enter to close.");
        HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
        wchar_t buffer[8] = {0};
        DWORD read = 0;
        if (in && in != INVALID_HANDLE_VALUE) ReadConsoleW(in, buffer, 7, &read, nullptr);
    }
    return result;
}

}  // namespace

int main() { return Run(); }
