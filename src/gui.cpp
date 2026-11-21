// GamerTCP - Windows 10/11 network optimization toolkit
// Native C++ reimplementation of the GameTCP3 tweak set (BSD 3-Clause).
//
// gui.cpp - Win32 graphical front end (no third-party libraries).

#include "gui.h"

#include "../res/resource.h"
#include "engine.h"

#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <powrprof.h>
#include <richedit.h>
#include <uxtheme.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "powrprof.lib")
#endif

#ifndef MSFTEDIT_CLASS
#define MSFTEDIT_CLASS L"RICHEDIT50W"
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace gtc {
namespace {

const wchar_t* const kWindowClass = L"GamerTCPMainWindow";
const wchar_t* const kSettingsClass = L"GamerTCPAppearanceWindow";
const wchar_t* const kPowerClass = L"GamerTCPPowerWindow";
const wchar_t* const kBaseTitle = L"GamerTCP 4 - Windows 10/11 network optimization";

enum ControlId {
    ID_SECTIONS = 1001,
    ID_LOG,
    ID_STATUS,
    ID_TITLE,
    ID_SUBTITLE,
    ID_APPLY,
    ID_PREVIEW,
    ID_SCAN,
    ID_REFRESH,
    ID_RESET,
    ID_BACKUP,
    ID_RESTORE,
    ID_TEREDO,
    ID_CLEAR,
    ID_SELECT_ALL,
    ID_SELECT_NONE,
    ID_ELEVATE,
    ID_ABOUT,
    ID_ZGT_SETUP,
    ID_SETTINGS,
    ID_POWER
};

enum LogKind { LOG_DEFAULT = 0, LOG_OK = 1, LOG_WARN = 2, LOG_ERROR = 3, LOG_DETAIL = 4, LOG_HEADING = 5 };

const UINT kLogMessage = WM_APP + 1;
const UINT kDoneMessage = WM_APP + 2;

const int kButtonCount = 11;

HWND g_window = nullptr;
HWND g_tree = nullptr;
HWND g_log = nullptr;
HWND g_status = nullptr;
HWND g_title = nullptr;
HWND g_subtitle = nullptr;
HWND g_elevate = nullptr;
HWND g_about = nullptr;
HWND g_setup = nullptr;
HWND g_buttons[kButtonCount] = {};
HWND g_settingsButton = nullptr;
HWND g_powerButton = nullptr;
HWND g_settings = nullptr;
HWND g_power = nullptr;
int  g_settingsHover = -1;
bool g_settingsTracking = false;
RECT g_settingsOptions[3] = {};
RECT g_settingsDone = {0, 0, 0, 0};
std::set<HWND> g_hotButtons;
RECT g_sectionsCard = {0, 0, 0, 0};
RECT g_logCard = {0, 0, 0, 0};
int  g_headerHeight = 0;
int  g_statusTop = 0;
HICON g_headerIcon = nullptr;
bool  g_headerIconOwned = false;
int   g_performancePercent = 0;
int   g_performanceMatched = 0;
int   g_performanceValues = 0;
double g_displayPercent = 0.0;

const UINT_PTR kMeterTimer = 1;

HFONT g_fontUi = nullptr;
HFONT g_fontUiBold = nullptr;
HFONT g_fontTitle = nullptr;
HFONT g_fontIcon = nullptr;
HFONT g_fontLog = nullptr;
HBRUSH g_background = nullptr;
HINSTANCE g_instance = nullptr;

int  g_dpi = 96;
bool g_elevated = false;
bool g_busy = false;
bool g_richEdit = false;

std::vector<const Section*> g_sections;

// Tree nodes point back at the section/tweak they represent so the ticked
// leaves can be turned into a per-value filter for the engine.
struct NodeInfo {
    const Section* section   = nullptr;
    const Tweak*   tweak     = nullptr;
    bool           isSection = false;
};

std::vector<NodeInfo*> g_nodes;

std::wstring TweakText(const Tweak& tweak) {
    if (tweak.isString) return tweak.sz ? tweak.sz : L"";
    return std::to_wstring(tweak.dw);
}

// ------------------------------------------------------------- helpers

int Px(int value) { return MulDiv(value, g_dpi, 96); }

int QueryDpi() {
    HDC dc = GetDC(nullptr);
    const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : 96;
    if (dc) ReleaseDC(nullptr, dc);
    return dpi > 0 ? dpi : 96;
}

HFONT MakeFont(int points, bool bold, const wchar_t* face) {
    LOGFONTW font{};
    font.lfHeight = -MulDiv(points, g_dpi, 72);
    font.lfWeight = bold ? FW_SEMIBOLD : FW_NORMAL;
    font.lfCharSet = DEFAULT_CHARSET;
    font.lfQuality = CLEARTYPE_QUALITY;
    lstrcpynW(font.lfFaceName, face, LF_FACESIZE);
    return CreateFontIndirectW(&font);
}

std::vector<const Section*> AllSections() {
    std::vector<const Section*> all;
    for (const Section& section : Sections()) all.push_back(&section);
    return all;
}

std::wstring FirstLine(const std::wstring& text) {
    const size_t end = text.find_first_of(L"\r\n");
    const std::wstring line = Trim(end == std::wstring::npos ? text : text.substr(0, end));
    return line.empty() ? text : line;
}

// ------------------------------------------------------------- theming

// The window follows the system scheme by default; the footer buttons switch
// to a fixed light or dark palette, and the choice is remembered per user.
enum ThemeMode { THEME_SYSTEM = 0, THEME_LIGHT = 1, THEME_DARK = 2 };

const wchar_t* const kSettingsKey = L"Software\\GamerTCP4";

struct Palette {
    COLORREF background;
    COLORREF panel;
    COLORREF border;
    COLORREF text;
    COLORREF muted;
    COLORREF dim;
    COLORREF accent;
    COLORREF accentText;
    COLORREF hover;
    COLORREF pressed;
    COLORREF success;
    COLORREF warning;
    COLORREF danger;
};

ThemeMode g_themeMode = THEME_SYSTEM;
bool      g_dark = false;

bool SystemUsesDarkMode() {
    DWORD light = 1;
    DWORD size = sizeof(light);
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
                      KEY_READ, &key) == ERROR_SUCCESS) {
        RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, nullptr,
                         reinterpret_cast<BYTE*>(&light), &size);
        RegCloseKey(key);
    }
    return light == 0;
}

Palette  g_palette{};
COLORREF g_systemAccent = CLR_INVALID;
bool     g_hasSystemAccent = false;

COLORREF Shade(COLORREF color, int delta) {
    auto clamp = [](int value) { return value < 0 ? 0 : (value > 255 ? 255 : value); };
    return RGB(clamp(GetRValue(color) + delta), clamp(GetGValue(color) + delta),
               clamp(GetBValue(color) + delta));
}

int Luminance(COLORREF color) {
    return (GetRValue(color) * 299 + GetGValue(color) * 587 + GetBValue(color) * 114) / 1000;
}

COLORREF ReadSystemAccent() {
    auto readDword = [](const wchar_t* subkey, const wchar_t* name, DWORD* out) {
        HKEY key = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, subkey, 0, KEY_READ, &key) != ERROR_SUCCESS)
            return false;
        DWORD value = 0;
        DWORD size = sizeof(value);
        const LONG status = RegQueryValueExW(key, name, nullptr, nullptr,
                                             reinterpret_cast<BYTE*>(&value), &size);
        RegCloseKey(key);
        if (status != ERROR_SUCCESS || size != sizeof(value) || value == 0) return false;
        *out = value;
        return true;
    };

    // Windows stores the personalisation accent as 0xAABBGGRR.
    DWORD value = 0;
    if (readDword(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent",
                  L"AccentColorMenu", &value)) {
        return RGB(value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF);
    }
    if (readDword(L"Software\\Microsoft\\Windows\\DWM", L"AccentColor", &value)) {
        return RGB(value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF);
    }

    DWORD colorization = 0;
    BOOL opaque = FALSE;
    if (SUCCEEDED(DwmGetColorizationColor(&colorization, &opaque))) {
        return RGB((colorization >> 16) & 0xFF, (colorization >> 8) & 0xFF, colorization & 0xFF);
    }
    return CLR_INVALID;
}

void BuildPalette() {
    if (g_dark) {
        g_palette = {RGB(7, 11, 20), RGB(16, 24, 41), RGB(35, 48, 72), RGB(230, 237, 251),
                     RGB(154, 168, 196), RGB(111, 125, 153), RGB(90, 209, 255), RGB(4, 18, 28),
                     RGB(24, 36, 58), RGB(31, 46, 73), RGB(74, 222, 128), RGB(251, 191, 36),
                     RGB(248, 113, 113)};
    } else {
        g_palette = {RGB(244, 246, 251), RGB(255, 255, 255), RGB(222, 228, 238), RGB(23, 32, 51),
                     RGB(94, 106, 130), RGB(138, 148, 168), RGB(10, 126, 186), RGB(255, 255, 255),
                     RGB(237, 243, 250), RGB(226, 235, 245), RGB(21, 128, 61), RGB(180, 83, 9),
                     RGB(185, 28, 28)};
    }

    COLORREF accent = g_hasSystemAccent ? g_systemAccent : g_palette.accent;
    if (g_dark && Luminance(accent) < 70) accent = Shade(accent, 60);
    if (!g_dark && Luminance(accent) > 205) accent = Shade(accent, -45);
    g_palette.accent = accent;
    g_palette.accentText = Luminance(accent) > 145 ? RGB(8, 14, 24) : RGB(255, 255, 255);
}

const Palette& Colors() { return g_palette; }

void RefreshSystemAccent() {
    g_systemAccent = ReadSystemAccent();
    g_hasSystemAccent = g_systemAccent != CLR_INVALID;
}

COLORREF LogColor(int kind) {
    const Palette& c = Colors();
    switch (kind) {
        case LOG_OK: return c.success;
        case LOG_WARN: return c.warning;
        case LOG_ERROR: return c.danger;
        case LOG_DETAIL: return c.muted;
        case LOG_HEADING: return c.accent;
        default: return c.text;
    }
}

void LoadThemePreference() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, KEY_READ, &key) != ERROR_SUCCESS) return;
    wchar_t value[32] = {0};
    DWORD size = sizeof(value);
    if (RegQueryValueExW(key, L"Theme", nullptr, nullptr, reinterpret_cast<BYTE*>(value), &size) ==
        ERROR_SUCCESS) {
        const std::wstring text = Lower(value);
        if (text == L"light")
            g_themeMode = THEME_LIGHT;
        else if (text == L"dark")
            g_themeMode = THEME_DARK;
        else
            g_themeMode = THEME_SYSTEM;
    }
    RegCloseKey(key);
}

void SaveThemePreference() {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, nullptr, 0, KEY_WRITE, nullptr, &key,
                        nullptr) != ERROR_SUCCESS)
        return;
    const wchar_t* value = g_themeMode == THEME_LIGHT ? L"light"
                           : g_themeMode == THEME_DARK ? L"dark"
                                                       : L"system";
    RegSetValueExW(key, L"Theme", 0, REG_SZ, reinterpret_cast<const BYTE*>(value),
                   static_cast<DWORD>((lstrlenW(value) + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
}

void RebuildLog();

void ApplyTheme() {
    g_dark = g_themeMode == THEME_DARK || (g_themeMode == THEME_SYSTEM && SystemUsesDarkMode());
    RefreshSystemAccent();
    BuildPalette();
    const Palette& c = Colors();

    if (g_background) DeleteObject(g_background);
    g_background = CreateSolidBrush(c.background);

    if (g_window) {
        const BOOL dark = g_dark ? TRUE : FALSE;
        if (FAILED(DwmSetWindowAttribute(g_window, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark,
                                         sizeof(dark)))) {
            DwmSetWindowAttribute(g_window, 19, &dark, sizeof(dark));
        }
    }

    if (g_tree) {
        SetWindowTheme(g_tree, g_dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
        TreeView_SetBkColor(g_tree, c.panel);
        TreeView_SetTextColor(g_tree, c.text);
        TreeView_SetLineColor(g_tree, c.border);
    }

    if (g_log) {
        SetWindowTheme(g_log, g_dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
        SendMessageW(g_log, EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(c.panel));
        RebuildLog();
    }

    HWND statics[] = {g_title, g_subtitle, g_status};
    for (HWND control : statics) {
        if (control) InvalidateRect(control, nullptr, TRUE);
    }
    for (HWND button : g_buttons) {
        if (button) InvalidateRect(button, nullptr, TRUE);
    }
    if (g_settings) {
        const BOOL dark = g_dark ? TRUE : FALSE;
        if (FAILED(DwmSetWindowAttribute(g_settings, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark,
                                         sizeof(dark)))) {
            DwmSetWindowAttribute(g_settings, 19, &dark, sizeof(dark));
        }
        InvalidateRect(g_settings, nullptr, TRUE);
    }
    if (g_power) {
        const BOOL dark = g_dark ? TRUE : FALSE;
        if (FAILED(DwmSetWindowAttribute(g_power, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark,
                                         sizeof(dark)))) {
            DwmSetWindowAttribute(g_power, 19, &dark, sizeof(dark));
        }
        InvalidateRect(g_power, nullptr, TRUE);
    }
    if (g_window) InvalidateRect(g_window, nullptr, TRUE);
}

void SetThemeMode(ThemeMode mode) {
    if (g_themeMode == mode) return;
    g_themeMode = mode;
    SaveThemePreference();
    ApplyTheme();
}

// ------------------------------------------------------------- placement

RECT g_normalBounds{};
bool g_normalBoundsValid = false;

bool LoadWindowPlacement(RECT* rect, bool* maximized) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, KEY_READ, &key) != ERROR_SUCCESS)
        return false;

    auto readDword = [&](const wchar_t* name, DWORD* out) {
        DWORD size = sizeof(DWORD);
        return RegQueryValueExW(key, name, nullptr, nullptr, reinterpret_cast<BYTE*>(out), &size) ==
               ERROR_SUCCESS;
    };

    DWORD left = 0, top = 0, width = 0, height = 0, maximizedValue = 0;
    const bool ok = readDword(L"WindowX", &left) && readDword(L"WindowY", &top) &&
                    readDword(L"WindowWidth", &width) && readDword(L"WindowHeight", &height);
    readDword(L"WindowMaximized", &maximizedValue);
    RegCloseKey(key);

    if (!ok || width < 200 || height < 200) return false;
    rect->left = static_cast<LONG>(left);
    rect->top = static_cast<LONG>(top);
    rect->right = rect->left + static_cast<LONG>(width);
    rect->bottom = rect->top + static_cast<LONG>(height);
    *maximized = maximizedValue != 0;
    return true;
}

void SaveWindowPlacement(HWND window) {
    RECT rect{};
    if (IsIconic(window) || IsZoomed(window)) {
        if (!g_normalBoundsValid) return;
        rect = g_normalBounds;
    } else if (!GetWindowRect(window, &rect)) {
        return;
    }

    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, nullptr, 0, KEY_WRITE, nullptr, &key,
                        nullptr) != ERROR_SUCCESS)
        return;

    auto write = [&](const wchar_t* name, DWORD value) {
        RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value),
                       sizeof(value));
    };
    write(L"WindowX", static_cast<DWORD>(rect.left));
    write(L"WindowY", static_cast<DWORD>(rect.top));
    write(L"WindowWidth", static_cast<DWORD>(rect.right - rect.left));
    write(L"WindowHeight", static_cast<DWORD>(rect.bottom - rect.top));
    write(L"WindowMaximized", IsZoomed(window) ? 1u : 0u);
    RegCloseKey(key);
}

// ------------------------------------------------------------- performance

// The meter behind the ZGT icon shows how much of the recommended set already
// matches the registry; it climbs as values are applied or restored.
std::wstring PerformanceText() {
    if (g_performanceValues <= 0) return L"Windows 10/11 network optimization";
    return L"Performance " + std::to_wstring(g_performancePercent) + L"%  -  " +
           std::to_wstring(g_performanceMatched) + L" of " +
           std::to_wstring(g_performanceValues) + L" values at target";
}

RECT MeterArea() {
    const int ring = Px(54);
    const int left = Px(16);
    const int top = (g_headerHeight - ring) / 2;
    RECT area = {left, top, left + ring, top + ring};
    return area;
}

void PaintPerformanceMeter(HDC dc) {
    const Palette& c = Colors();
    const RECT area = MeterArea();
    const int centreX = (area.left + area.right) / 2;
    const int centreY = (area.top + area.bottom) / 2;
    const int radius = (area.right - area.left) / 2 - Px(2);
    const int thickness = Px(4);

    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));

    HPEN track = CreatePen(PS_SOLID, thickness, c.border);
    HGDIOBJ oldPen = SelectObject(dc, track);
    Ellipse(dc, centreX - radius, centreY - radius, centreX + radius, centreY + radius);
    SelectObject(dc, oldPen);
    DeleteObject(track);

    if (g_displayPercent > 0.5) {
        LOGBRUSH brush{};
        brush.lbStyle = BS_SOLID;
        brush.lbColor = c.accent;
        HPEN progress = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND, thickness, &brush,
                                     0, nullptr);
        if (progress) {
            oldPen = SelectObject(dc, progress);
            if (g_displayPercent >= 99.5) {
                Ellipse(dc, centreX - radius, centreY - radius, centreX + radius, centreY + radius);
            } else {
                const double radians =
                    (-90.0 + g_displayPercent * 3.6) * 3.14159265358979323846 / 180.0;
                const int endX = centreX + static_cast<int>(std::lround(std::cos(radians) * radius));
                const int endY = centreY + static_cast<int>(std::lround(std::sin(radians) * radius));
                SetArcDirection(dc, AD_CLOCKWISE);
                Arc(dc, centreX - radius, centreY - radius, centreX + radius, centreY + radius,
                    centreX, centreY - radius, endX, endY);
            }
            SelectObject(dc, oldPen);
            DeleteObject(progress);
        }
    }

    SelectObject(dc, oldBrush);

    if (g_headerIcon) {
        const int iconSize = Px(32);
        DrawIconEx(dc, centreX - iconSize / 2, centreY - iconSize / 2, g_headerIcon, iconSize,
                   iconSize, 0, nullptr, DI_NORMAL);
    }
}

void UpdatePerformanceMeter() {
    std::vector<ScanRow> rows;
    ScanSections(AllSections(), rows);

    // Per-interface tweaks appear once per interface; count each tweak as one
    // value, at target only when every interface matches, so the meter uses the
    // same 106-value set the rest of the interface reports.
    std::map<const Tweak*, bool> values;
    for (const ScanRow& row : rows) {
        if (!row.tweak) continue;
        const bool atTarget = !row.failed && row.matches;
        auto found = values.find(row.tweak);
        if (found == values.end())
            values[row.tweak] = atTarget;
        else
            found->second = found->second && atTarget;
    }

    int matched = 0;
    for (const auto& value : values) {
        if (value.second) ++matched;
    }
    const int total = static_cast<int>(values.size());
    g_performanceMatched = matched;
    g_performanceValues = total;
    g_performancePercent = total > 0 ? (matched * 100 + total / 2) / total : 0;

    if (g_subtitle) {
        SetWindowTextW(g_subtitle, PerformanceText().c_str());
        InvalidateRect(g_subtitle, nullptr, TRUE);
    }
    if (g_window) {
        SetTimer(g_window, kMeterTimer, 16, nullptr);
        InvalidateRect(g_window, nullptr, TRUE);
    }
}

// ------------------------------------------------------------- logging

// The log is kept as data so it can be recoloured when the scheme changes.
struct LogEntry {
    std::wstring text;
    int          kind = LOG_DEFAULT;
    bool         bold = false;
};

std::vector<LogEntry> g_logEntries;

void AppendToControl(const std::wstring& text, int kind, bool bold) {
    if (!g_log) return;

    if (!g_richEdit) {
        std::wstring payload = text + L"\r\n";
        const int length = GetWindowTextLengthW(g_log);
        SendMessageW(g_log, EM_SETREADONLY, FALSE, 0);
        SendMessageW(g_log, EM_SETSEL, static_cast<WPARAM>(length), static_cast<LPARAM>(length));
        SendMessageW(g_log, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(payload.c_str()));
        SendMessageW(g_log, EM_SETREADONLY, TRUE, 0);
        SendMessageW(g_log, EM_SCROLLCARET, 0, 0);
        return;
    }

    const int length = GetWindowTextLengthW(g_log);
    SendMessageW(g_log, EM_SETSEL, static_cast<WPARAM>(length), static_cast<LPARAM>(length));

    CHARFORMAT2W format{};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_COLOR | CFM_BOLD;
    format.crTextColor = LogColor(kind);
    format.dwEffects = bold ? CFE_BOLD : 0;
    SendMessageW(g_log, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));

    std::wstring payload = text + L"\r\n";
    SendMessageW(g_log, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(payload.c_str()));
    SendMessageW(g_log, EM_SCROLLCARET, 0, 0);
}

void ApplyLogFont() {
    if (!g_log || !g_richEdit) return;
    CHARFORMAT2W format{};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_FACE | CFM_SIZE | CFM_CHARSET;
    format.yHeight = 9 * 20;
    format.bCharSet = DEFAULT_CHARSET;
    lstrcpynW(format.szFaceName, L"Consolas", LF_FACESIZE);
    SendMessageW(g_log, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&format));
}

void RebuildLog() {
    if (!g_log) return;
    SendMessageW(g_log, WM_SETREDRAW, FALSE, 0);
    SetWindowTextW(g_log, L"");
    ApplyLogFont();
    for (const LogEntry& entry : g_logEntries) AppendToControl(entry.text, entry.kind, entry.bold);
    SendMessageW(g_log, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_log, nullptr, TRUE);
}

void AppendLog(const std::wstring& text, int kind, bool bold = false) {
    if (!g_log) return;
    if (g_logEntries.size() >= 4000) {
        g_logEntries.erase(g_logEntries.begin(), g_logEntries.begin() + 1000);
        RebuildLog();
    }
    g_logEntries.push_back(LogEntry{text, kind, bold});
    AppendToControl(text, kind, bold);
}

void PostLog(int kind, const std::wstring& text, bool bold = false) {
    const WPARAM payload = static_cast<WPARAM>(kind | (bold ? 0x100 : 0));
    PostMessageW(g_window, kLogMessage, payload, reinterpret_cast<LPARAM>(new std::wstring(text)));
}

// ------------------------------------------------------------- state

NodeInfo* ItemInfo(HTREEITEM item) {
    if (!item) return nullptr;
    TVITEMW info{};
    info.mask = TVIF_PARAM;
    info.hItem = item;
    if (!TreeView_GetItem(g_tree, &info)) return nullptr;
    return reinterpret_cast<NodeInfo*>(info.lParam);
}

bool ItemChecked(HTREEITEM item) {
    if (!item) return false;
    const UINT state = static_cast<UINT>(SendMessageW(
        g_tree, TVM_GETITEMSTATE, reinterpret_cast<WPARAM>(item), TVIS_STATEIMAGEMASK));
    return ((state >> 12) & 0x0F) == 2;
}

void SetItemChecked(HTREEITEM item, bool checked) {
    if (!item) return;
    TVITEMW info{};
    info.mask = TVIF_STATE;
    info.hItem = item;
    info.stateMask = TVIS_STATEIMAGEMASK;
    info.state = INDEXTOSTATEIMAGEMASK(checked ? 2 : 1);
    TreeView_SetItem(g_tree, &info);
}

int TotalValueCount() {
    int total = 0;
    for (const Section& section : Sections()) total += section.count;
    return total;
}

std::set<const Tweak*> CheckedTweaks() {
    std::set<const Tweak*> picked;
    for (HTREEITEM section = TreeView_GetRoot(g_tree); section;
         section = TreeView_GetNextSibling(g_tree, section)) {
        for (HTREEITEM child = TreeView_GetChild(g_tree, section); child;
             child = TreeView_GetNextSibling(g_tree, child)) {
            const NodeInfo* info = ItemInfo(child);
            if (info && info->tweak && ItemChecked(child)) picked.insert(info->tweak);
        }
    }
    return picked;
}

std::vector<const Section*> SectionsWithChecked(const std::set<const Tweak*>& picked) {
    std::vector<const Section*> out;
    for (const Section* section : g_sections) {
        for (int i = 0; i < section->count; ++i) {
            if (picked.count(&section->tweaks[i]) > 0) {
                out.push_back(section);
                break;
            }
        }
    }
    return out;
}

TweakFilter FilterFor(const std::set<const Tweak*>& picked) {
    return [picked](const Section&, const Tweak& tweak) { return picked.count(&tweak) > 0; };
}

void UpdateStatus(const std::wstring& message = std::wstring()) {
    if (!g_status) return;
    std::wstring text = g_elevated ? L"Administrator" : L"Not elevated - read-only";
    text += L"   |   " + std::to_wstring(CheckedTweaks().size()) + L" of " +
            std::to_wstring(TotalValueCount()) + L" values selected";
    if (!message.empty()) text += L"   |   " + message;
    SetWindowTextW(g_status, text.c_str());
    InvalidateRect(g_status, nullptr, TRUE);
}

void SyncSectionState(HTREEITEM leaf) {
    const HTREEITEM parent = TreeView_GetParent(g_tree, leaf);
    if (!parent) return;
    bool allChecked = true;
    int children = 0;
    for (HTREEITEM child = TreeView_GetChild(g_tree, parent); child;
         child = TreeView_GetNextSibling(g_tree, child)) {
        ++children;
        if (!ItemChecked(child)) allChecked = false;
    }
    SetItemChecked(parent, children > 0 && allChecked);
}

void ToggleNode(HTREEITEM item) {
    const NodeInfo* info = ItemInfo(item);
    if (!info) return;
    if (info->isSection) {
        const bool state = !ItemChecked(item);
        SetItemChecked(item, state);
        for (HTREEITEM child = TreeView_GetChild(g_tree, item); child;
             child = TreeView_GetNextSibling(g_tree, child)) {
            SetItemChecked(child, state);
        }
    } else {
        SetItemChecked(item, !ItemChecked(item));
        SyncSectionState(item);
    }
    UpdateStatus();
}

void EnableControl(int id, bool enabled) {
    HWND control = GetDlgItem(g_window, id);
    if (control) EnableWindow(control, enabled ? TRUE : FALSE);
}

void UpdateControls() {
    const bool canWrite = g_elevated && !g_busy;
    EnableControl(ID_APPLY, canWrite);
    EnableControl(ID_ZGT_SETUP, !g_busy);
    EnableControl(ID_RESET, canWrite);
    EnableControl(ID_RESTORE, canWrite);
    EnableControl(ID_TEREDO, canWrite);
    EnableControl(ID_PREVIEW, !g_busy);
    EnableControl(ID_SCAN, !g_busy);
    EnableControl(ID_REFRESH, !g_busy);
    EnableControl(ID_BACKUP, !g_busy);
    EnableControl(ID_CLEAR, !g_busy);
    EnableControl(ID_SELECT_ALL, !g_busy);
    EnableControl(ID_SELECT_NONE, !g_busy);
    if (g_tree) EnableWindow(g_tree, g_busy ? FALSE : TRUE);
    if (g_elevate) ShowWindow(g_elevate, g_elevated ? SW_HIDE : SW_SHOW);
}

void StartWorker(const std::function<void()>& work) {
    if (g_busy) return;
    g_busy = true;
    UpdateControls();
    UpdateStatus(L"working...");
    std::thread([work]() {
        work();
        PostMessageW(g_window, kDoneMessage, 0, 0);
    }).detach();
}

// ------------------------------------------------------------- chrome

void FillRounded(HDC dc, const RECT& rect, int radius, COLORREF fill, COLORREF border) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void DrawLabel(HDC dc, const wchar_t* text, RECT rect, HFONT font, COLORREF color, UINT flags) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    HGDIOBJ oldFont = SelectObject(dc, font);
    DrawTextW(dc, text, -1, &rect, flags);
    SelectObject(dc, oldFont);
}

void PaintCard(HDC dc, const RECT& rect, const wchar_t* title) {
    if (rect.right <= rect.left || rect.bottom <= rect.top) return;
    const Palette& c = Colors();
    FillRounded(dc, rect, Px(12), c.panel, c.border);
    RECT label = {rect.left + Px(15), rect.top + Px(4), rect.right - Px(15), rect.top + Px(28)};
    DrawLabel(dc, title, label, g_fontUiBold, c.muted,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void PaintChrome(HDC dc, const RECT& client) {
    const Palette& c = Colors();
    const int margin = Px(16);

    PaintPerformanceMeter(dc);

    HPEN pen = CreatePen(PS_SOLID, 1, c.border);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    MoveToEx(dc, margin, g_headerHeight - 1, nullptr);
    LineTo(dc, client.right - margin, g_headerHeight - 1);
    MoveToEx(dc, margin, g_statusTop, nullptr);
    LineTo(dc, client.right - margin, g_statusTop);
    SelectObject(dc, oldPen);
    DeleteObject(pen);

    PaintCard(dc, g_sectionsCard, L"Sections");
    PaintCard(dc, g_logCard, L"Activity");
}

// ------------------------------------------------------------- layout

void Layout(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    const int width = client.right;
    const int height = client.bottom;

    const int margin = Px(16);
    g_headerHeight = Px(72);
    const int cardGap = Px(12);
    const int cardPadding = Px(14);
    const int cardHeading = Px(30);
    const int statusHeight = Px(34);
    const int buttonHeight = Px(34);
    const int buttonGap = Px(8);
    const int columns = 5;
    const int buttonRows = (kButtonCount + columns - 1) / columns;
    const int buttonsHeight = buttonRows * buttonHeight + (buttonRows - 1) * buttonGap;

    g_statusTop = height - statusHeight;
    const int buttonsTop = g_statusTop - Px(10) - buttonsHeight;
    const int contentTop = g_headerHeight + Px(12);
    const int contentHeight = buttonsTop - cardGap - contentTop;

    const int contentWidth = width - margin * 2;
    int sectionsWidth = Px(440);
    if (sectionsWidth > contentWidth * 46 / 100) sectionsWidth = contentWidth * 46 / 100;
    const int logX = margin + sectionsWidth + cardGap;
    const int logWidth = width - margin - logX;

    g_sectionsCard = {margin, contentTop, margin + sectionsWidth, contentTop + contentHeight};
    g_logCard = {logX, contentTop, logX + logWidth, contentTop + contentHeight};

    const int aboutWidth = Px(84);
    const int elevateWidth = Px(186);
    const int setupWidth = Px(206);
    const int settingsWidth = Px(36);
    const int powerWidth = Px(36);
    const int headerGap = Px(8);
    const int headerReserve = aboutWidth + elevateWidth + setupWidth + settingsWidth + powerWidth +
                              headerGap * 4 + Px(20);
    const int aboutX = width - margin - aboutWidth;
    const int settingsX = aboutX - headerGap - settingsWidth;
    const int powerX = settingsX - headerGap - powerWidth;
    const int elevateX = powerX - headerGap - elevateWidth;
    const int setupX = elevateX - headerGap - setupWidth;
    if (g_about) MoveWindow(g_about, aboutX, Px(22), aboutWidth, Px(28), TRUE);
    if (g_settingsButton)
        MoveWindow(g_settingsButton, settingsX, Px(22), settingsWidth, Px(28), TRUE);
    if (g_powerButton) MoveWindow(g_powerButton, powerX, Px(22), powerWidth, Px(28), TRUE);
    if (g_elevate) MoveWindow(g_elevate, elevateX, Px(22), elevateWidth, Px(28), TRUE);
    if (g_setup) MoveWindow(g_setup, setupX, Px(22), setupWidth, Px(28), TRUE);
    MoveWindow(g_title, margin + Px(84), Px(10), width - margin * 2 - headerReserve, Px(32), TRUE);
    MoveWindow(g_subtitle, margin + Px(84), Px(42), width - margin * 2 - headerReserve, Px(22), TRUE);

    if (g_tree)
        MoveWindow(g_tree, g_sectionsCard.left + cardPadding, g_sectionsCard.top + cardHeading,
                   sectionsWidth - cardPadding * 2, contentHeight - cardHeading - cardPadding, TRUE);
    if (g_log)
        MoveWindow(g_log, g_logCard.left + cardPadding, g_logCard.top + cardHeading,
                   logWidth - cardPadding * 2, contentHeight - cardHeading - cardPadding, TRUE);

    const int buttonWidth = (contentWidth - buttonGap * (columns - 1)) / columns;
    for (int i = 0; i < kButtonCount; ++i) {
        const int column = i % columns;
        const int row = i / columns;
        MoveWindow(g_buttons[i], margin + column * (buttonWidth + buttonGap),
                   buttonsTop + row * (buttonHeight + buttonGap), buttonWidth, buttonHeight, TRUE);
    }

    if (g_status)
        MoveWindow(g_status, margin, g_statusTop + Px(8), width - margin * 2, Px(20), TRUE);
}

// ------------------------------------------------------------- controls

HWND CreateControl(const wchar_t* klass, const wchar_t* text, DWORD style, int id,
                   DWORD exStyle = 0) {
    return CreateWindowExW(exStyle, klass, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0,
                           g_window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                           g_instance, nullptr);
}

LRESULT CALLBACK ButtonSubclass(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
                                UINT_PTR, DWORD_PTR) {
    switch (message) {
        case WM_MOUSEMOVE:
            if (g_hotButtons.insert(hwnd).second) {
                TRACKMOUSEEVENT track{};
                track.cbSize = sizeof(track);
                track.dwFlags = TME_LEAVE;
                track.hwndTrack = hwnd;
                TrackMouseEvent(&track);
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            break;
        case WM_MOUSELEAVE:
            g_hotButtons.erase(hwnd);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, ButtonSubclass, 1);
            break;
        default:
            break;
    }
    return DefSubclassProc(hwnd, message, wParam, lParam);
}

void DrawButton(const DRAWITEMSTRUCT& item) {
    const Palette& c = Colors();
    const bool disabled = (item.itemState & ODS_DISABLED) != 0;
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;
    const bool focused = (item.itemState & ODS_FOCUS) != 0;
    const bool hot = g_hotButtons.count(item.hwndItem) > 0;

    const bool accentButton = item.CtlID == ID_APPLY || item.CtlID == ID_ZGT_SETUP;

    COLORREF fill = c.panel;
    COLORREF border = c.border;
    COLORREF text = c.text;
    if (disabled) {
        fill = c.panel;
        text = c.dim;
    } else if (accentButton) {
        fill = pressed ? Shade(c.accent, g_dark ? -18 : -22)
                       : (hot ? Shade(c.accent, g_dark ? 16 : -10) : c.accent);
        border = fill;
        text = c.accentText;
    } else if (pressed) {
        fill = c.pressed;
    } else if (hot) {
        fill = c.hover;
    }

    RECT rect = item.rcItem;
    FillRounded(item.hDC, rect, Px(8), fill, border);

    wchar_t label[128] = {0};
    GetWindowTextW(item.hwndItem, label, 128);
    RECT textRect = rect;
    textRect.left += Px(8);
    textRect.right -= Px(8);
    HFONT font = g_fontUi;
    if (item.CtlID == ID_APPLY || item.CtlID == ID_ZGT_SETUP)
        font = g_fontUiBold;
    else if (item.CtlID == ID_SETTINGS || item.CtlID == ID_POWER)
        font = g_fontIcon;
    DrawLabel(item.hDC, label, textRect, font, text,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    if (focused && !disabled) {
        RECT focus = rect;
        InflateRect(&focus, -Px(4), -Px(4));
        DrawFocusRect(item.hDC, &focus);
    }
}

HWND CreateButton(int id, const wchar_t* text) {
    HWND button = CreateControl(L"BUTTON", text, WS_TABSTOP | BS_OWNERDRAW, id);
    if (button) {
        SendMessageW(button, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);
        SetWindowSubclass(button, ButtonSubclass, 1, 0);
    }
    return button;
}

// Tree rows show what every value holds right now next to the target it would
// be set to, e.g. "DefaultTTL = 255 -> 64". The values are read through the
// engine, which also walks the per-interface keys, so the recursive sections
// report every interface instead of only their target.
struct ValueSnapshot {
    std::vector<std::pair<std::wstring, int>> distinct;  // current text, instances
    int instances = 0;
};

std::map<const Tweak*, ValueSnapshot> g_valueSnapshots;

// Reads every value of every section, including one row per network interface.
void ReadCurrentValues() {
    g_valueSnapshots.clear();
    std::vector<ScanRow> rows;
    ScanSections(AllSections(), rows);
    for (const ScanRow& row : rows) {
        if (!row.tweak) continue;  // section-level failure, already reported
        ValueSnapshot& snapshot = g_valueSnapshots[row.tweak];
        ++snapshot.instances;
        bool seen = false;
        for (auto& item : snapshot.distinct) {
            if (item.first == row.current) {
                ++item.second;
                seen = true;
                break;
            }
        }
        if (!seen) snapshot.distinct.emplace_back(row.current, 1);
    }
}

std::wstring CurrentTextFor(const Section& section, const Tweak& tweak) {
    const auto found = g_valueSnapshots.find(&tweak);
    if (found == g_valueSnapshots.end())
        return ReadValue(tweak.root, tweak.path, tweak.name).Display();

    const ValueSnapshot& snapshot = found->second;
    if (snapshot.instances == 0) return L"(not set)";

    std::wstring text;
    for (size_t i = 0; i < snapshot.distinct.size(); ++i) {
        if (i > 0) text += L", ";
        text += snapshot.distinct[i].first;
        if (snapshot.distinct.size() > 1)
            text += L" x" + std::to_wstring(snapshot.distinct[i].second);
    }
    if (section.mode != SectionMode::Fixed) {
        text += L"  (" + std::to_wstring(snapshot.instances) +
                (snapshot.instances == 1 ? L" interface)" : L" interfaces)");
    }
    return text;
}

std::wstring LeafLabel(const Section& section, const Tweak& tweak) {
    return std::wstring(tweak.name) + L" = " + CurrentTextFor(section, tweak) + L" -> " +
           TweakText(tweak);
}

// Re-reads every value and rewrites the labels, so the tree is current after an
// apply, a restore, a scan or a manual refresh.
void RefreshTreeValues() {
    if (!g_tree) return;
    ReadCurrentValues();
    for (HTREEITEM sectionItem = TreeView_GetRoot(g_tree); sectionItem;
         sectionItem = TreeView_GetNextSibling(g_tree, sectionItem)) {
        const NodeInfo* sectionNode = ItemInfo(sectionItem);
        if (!sectionNode || !sectionNode->section) continue;
        for (HTREEITEM child = TreeView_GetChild(g_tree, sectionItem); child;
             child = TreeView_GetNextSibling(g_tree, child)) {
            const NodeInfo* leaf = ItemInfo(child);
            if (!leaf || !leaf->tweak) continue;
            std::wstring text = LeafLabel(*sectionNode->section, *leaf->tweak);
            TVITEMW info{};
            info.mask = TVIF_TEXT;
            info.hItem = child;
            info.pszText = const_cast<wchar_t*>(text.c_str());
            TreeView_SetItem(g_tree, &info);
        }
    }
}

void PopulateTree() {
    for (const Section& section : Sections()) {
        NodeInfo* sectionNode = new NodeInfo{&section, nullptr, true};
        g_nodes.push_back(sectionNode);

        std::wstring label = section.name;
        if (section.mode == SectionMode::Fixed) {
            label += L"  (" + std::to_wstring(section.count) + L" values)";
        } else {
            label += L"  (" + std::to_wstring(section.count) + L" per interface)";
        }

        TVINSERTSTRUCTW insert{};
        insert.hParent = TVI_ROOT;
        insert.hInsertAfter = TVI_LAST;
        insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
        insert.item.stateMask = TVIS_STATEIMAGEMASK;
        insert.item.state = INDEXTOSTATEIMAGEMASK(section.defaultOn ? 2 : 1);
        insert.item.pszText = const_cast<wchar_t*>(label.c_str());
        insert.item.lParam = reinterpret_cast<LPARAM>(sectionNode);
        const HTREEITEM sectionItem = TreeView_InsertItem(g_tree, &insert);
        if (!sectionItem) continue;
        // Insert-time state is not always honoured for TVS_CHECKBOXES, so set
        // the check state explicitly after the item exists.
        SetItemChecked(sectionItem, section.defaultOn);

        for (int i = 0; i < section.count; ++i) {
            const Tweak& tweak = section.tweaks[i];
            NodeInfo* leaf = new NodeInfo{&section, &tweak, false};
            g_nodes.push_back(leaf);

            std::wstring text = LeafLabel(section, tweak);
            insert.hParent = sectionItem;
            insert.item.state = INDEXTOSTATEIMAGEMASK(section.defaultOn ? 2 : 1);
            insert.item.pszText = const_cast<wchar_t*>(text.c_str());
            insert.item.lParam = reinterpret_cast<LPARAM>(leaf);
            const HTREEITEM leafItem = TreeView_InsertItem(g_tree, &insert);
            SetItemChecked(leafItem, section.defaultOn);
        }

        TreeView_Expand(g_tree, sectionItem, TVE_EXPAND);
    }
}

void CreateControls(HWND window) {
    g_window = window;

    g_fontUi = MakeFont(9, false, L"Segoe UI");
    g_fontUiBold = MakeFont(9, true, L"Segoe UI");
    g_fontTitle = MakeFont(16, true, L"Segoe UI");
    g_fontIcon = MakeFont(11, false, L"Segoe MDL2 Assets");
    g_fontLog = MakeFont(9, false, L"Consolas");

    g_headerIcon = static_cast<HICON>(LoadImageW(g_instance, MAKEINTRESOURCEW(IDI_APP_ICON),
                                                 IMAGE_ICON, Px(32), Px(32), LR_DEFAULTCOLOR));
    g_headerIconOwned = g_headerIcon != nullptr;
    if (!g_headerIcon)
        g_headerIcon = static_cast<HICON>(LoadIconW(g_instance, MAKEINTRESOURCEW(IDI_APP_ICON)));

    g_title = CreateControl(L"STATIC", L"GamerTCP 4", SS_LEFT, ID_TITLE);
    g_subtitle = CreateControl(
        L"STATIC",
        L"Windows 10/11 network optimization - 106 registry values across 15 sections",
        SS_LEFT | SS_ENDELLIPSIS, ID_SUBTITLE);

    SendMessageW(g_title, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontTitle), TRUE);
    SendMessageW(g_subtitle, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    g_elevate = CreateButton(ID_ELEVATE, L"Restart as administrator");
    g_about = CreateButton(ID_ABOUT, L"About...");
    g_setup = CreateButton(ID_ZGT_SETUP, L"ZGT Performance Setup");
    g_settingsButton = CreateButton(ID_SETTINGS, L"\xE713");
    if (g_settingsButton) {
        SendMessageW(g_settingsButton, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontIcon), TRUE);
    }
    g_powerButton = CreateButton(ID_POWER, L"\xE945");
    if (g_powerButton) {
        SendMessageW(g_powerButton, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontIcon), TRUE);
    }

    g_tree = CreateControl(L"SYSTREEVIEW32", L"",
                           TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_FULLROWSELECT |
                               TVS_SHOWSELALWAYS | TVS_CHECKBOXES | TVS_INFOTIP | WS_TABSTOP,
                           ID_SECTIONS);
    SendMessageW(g_tree, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);
    ReadCurrentValues();
    PopulateTree();

    LoadLibraryW(L"Msftedit.dll");
    g_log = CreateWindowExW(0, MSFTEDIT_CLASS, L"",
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_READONLY |
                                ES_AUTOVSCROLL | ES_NOHIDESEL | WS_VSCROLL,
                            0, 0, 0, 0, window,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LOG)), g_instance,
                            nullptr);
    g_richEdit = g_log != nullptr;
    if (!g_log) {
        g_log = CreateWindowExW(0, L"EDIT", L"",
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_READONLY |
                                    ES_AUTOVSCROLL | ES_NOHIDESEL | WS_VSCROLL,
                                0, 0, 0, 0, window,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LOG)), g_instance,
                                nullptr);
    }
    if (g_log) {
        SendMessageW(g_log, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontLog), TRUE);
        SendMessageW(g_log, EM_LIMITTEXT, static_cast<WPARAM>(1024 * 1024), 0);
        ApplyLogFont();
    }

    g_status = CreateControl(L"STATIC", L"", SS_LEFT | SS_ENDELLIPSIS, ID_STATUS);
    SendMessageW(g_status, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    g_buttons[0] = CreateButton(ID_APPLY, L"Apply Selected");
    g_buttons[1] = CreateButton(ID_PREVIEW, L"Preview (dry run)");
    g_buttons[2] = CreateButton(ID_SCAN, L"Scan Current State");
    g_buttons[3] = CreateButton(ID_REFRESH, L"Refresh Values");
    g_buttons[4] = CreateButton(ID_BACKUP, L"Backup...");
    g_buttons[5] = CreateButton(ID_RESTORE, L"Restore...");
    g_buttons[6] = CreateButton(ID_RESET, L"Reset Network Stack");
    g_buttons[7] = CreateButton(ID_TEREDO, L"Teredo...");
    g_buttons[8] = CreateButton(ID_SELECT_ALL, L"Select All");
    g_buttons[9] = CreateButton(ID_SELECT_NONE, L"Select None");
    g_buttons[10] = CreateButton(ID_CLEAR, L"Clear Log");

    if (g_sections.empty()) g_sections = AllSections();

    ApplyTheme();

    AppendLog(L"GamerTCP " + std::wstring(VersionString()), LOG_HEADING, true);
    AppendLog(L"Expand a section to see and tick individual registry values.", LOG_DEFAULT);
    AppendLog(L"Press \"Apply Selected\" to write the ticked values, or \"Preview\" first.",
              LOG_DEFAULT);
    AppendLog(L"", LOG_DEFAULT);
    if (!g_elevated) {
        AppendLog(L"GamerTCP 4 is not running as administrator: applying, restoring, resetting and "
                  L"changing Teredo are disabled until you restart elevated.",
                  LOG_WARN);
    }

    UpdatePerformanceMeter();
}

// ------------------------------------------------------------- actions

std::wstring EntryLine(const Entry& entry) {
    std::wstring line = entry.sub.empty() ? L"  " : L"  [" + entry.sub + L"] ";
    line += entry.name + L": " + entry.oldData + L" -> " + entry.newData;
    return line;
}

void SetAllChecks(bool checked);

void BeginApply(bool dryRun, bool performanceSetup = false) {
    if (g_busy) return;
    if (performanceSetup) SetAllChecks(true);
    const std::set<const Tweak*> picked = CheckedTweaks();
    if (picked.empty()) {
        MessageBoxW(g_window, L"Tick at least one registry value first.", kBaseTitle,
                    MB_OK | MB_ICONINFORMATION);
        return;
    }
    const std::vector<const Section*> sections = SectionsWithChecked(picked);
    if (!dryRun && !g_elevated) {
        if (performanceSetup) {
            MessageBoxW(g_window,
                        L"The ZGT performance setup needs administrator rights.\r\n\r\n"
                        L"Click \"Restart as administrator\" and try again.",
                        L"ZGT Performance Setup", MB_OK | MB_ICONWARNING);
        } else {
            MessageBoxW(g_window,
                        L"Applying registry tweaks needs administrator rights.\r\n\r\n"
                        L"Click \"Restart as administrator\" and try again.",
                        kBaseTitle, MB_OK | MB_ICONWARNING);
        }
        return;
    }

    if (!dryRun) {
        const std::wstring question =
            performanceSetup
                ? L"Run the ZGT performance setup?\r\n\r\n"
                  L"Every recommended value is selected (" +
                      std::to_wstring(picked.size()) + L" values across " +
                      std::to_wstring(sections.size()) +
                      L" sections). A backup is written first, then the global TCP settings "
                      L"are re-applied and the values are written. The destructive stack "
                      L"reset (netsh int ip reset and neighbours) is not part of this: it "
                      L"discards interface keys and static IP configuration, so it only "
                      L"runs from Reset Network Stack.\r\n\r\n"
                      L"A reboot is recommended; Restore... with the backup undoes the setup."
                : L"Apply " + std::to_wstring(picked.size()) + L" selected value(s) from " +
                      std::to_wstring(sections.size()) + L" section(s)?\r\n\r\n"
                      L"A backup of every current value is written first, then the global TCP "
                      L"settings are re-applied and the values are written. The destructive "
                      L"stack reset is not run; Reset Network Stack does that separately, "
                      L"because it discards interface keys and static IP configuration."
                      L"\r\n\r\nA reboot is recommended afterwards.";
        const wchar_t* title = performanceSetup ? L"ZGT Performance Setup" : L"Apply network tweaks";
        if (MessageBoxW(g_window, question.c_str(), title,
                        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
            return;
        }
    }

    AppendLog(L"", LOG_DEFAULT);
    if (performanceSetup) {
        AppendLog(L"ZGT Performance Setup - applying every recommended value", LOG_HEADING, true);
    } else {
        AppendLog(dryRun ? L"Preview - no changes are written" : L"Applying the selected values",
                  LOG_HEADING, true);
    }

    StartWorker([sections, picked, dryRun]() {
        const TweakFilter filter = FilterFor(picked);
        if (!dryRun) {
            const std::wstring file = DefaultBackupPath();
            std::wstring error;
            if (BackupSections(sections, file, &error, filter)) {
                PostLog(LOG_DETAIL, L"Backup written: " + file);
            } else {
                PostLog(LOG_WARN, L"WARNING: backup failed (" + error + L"); continuing.");
            }
        }

        // Only the non-destructive part of the netsh set runs here: the global
        // TCP parameters and the cache flushes. netsh int ip reset (and its tcp,
        // ipv4, ipv6 and winsock neighbours) is equivalent to removing and
        // reinstalling TCP/IP and deletes the per-interface keys, so it stays
        // behind Reset Network Stack and never rides along with an apply.
        if (!dryRun) {
            PostLog(LOG_HEADING, L"Applying the global TCP settings");
            const CommandSummary tune = RunCommands(
                TuneCommands(), false, [](const CommandResult& result) {
                    if (result.ok) {
                        PostLog(LOG_DETAIL, L"  OK    " + result.command);
                    } else {
                        PostLog(LOG_ERROR, L"  FAIL  " + result.command +
                                               (result.output.empty()
                                                    ? std::wstring()
                                                    : L" - " + FirstLine(result.output)));
                    }
                });
            PostLog(tune.failed > 0 ? LOG_WARN : LOG_OK,
                    L"Global TCP settings: " + std::to_wstring(tune.succeeded) +
                        L" succeeded, " + std::to_wstring(tune.failed) + L" failed.");
            PostLog(LOG_DETAIL,
                    L"Network stack reset skipped: it discards interface keys and static IP "
                    L"configuration, so only Reset Network Stack runs it.");
            PostLog(LOG_DEFAULT, L"");
        }

        ApplyOptions options;
        options.dryRun = dryRun;
        options.filter = filter;

        // Collect the entries instead of logging them as they happen: every
        // written value is read back once all of them have been applied, so a
        // later step that silently undoes a write is reported as a failure
        // rather than a change.
        std::vector<Entry> entries;
        for (const Section* section : sections) {
            ApplySections({section}, options,
                          [&](const Entry& entry) { entries.push_back(entry); });
        }

        if (!dryRun) {
            const int reverted = VerifyEntries(entries);
            if (reverted > 0) {
                PostLog(LOG_ERROR, L"Verification: " + std::to_wstring(reverted) +
                                       L" value(s) did not keep the value that was written.");
            }
        }

        int changed = 0;
        int unchanged = 0;
        int failed = 0;
        int sectionChanged = 0;
        int sectionUnchanged = 0;
        int sectionFailed = 0;
        std::wstring lastSection;
        auto flushSection = [&]() {
            if (lastSection.empty()) return;
            PostLog(LOG_DETAIL, L"  " + std::to_wstring(sectionChanged) + L" changed, " +
                                    std::to_wstring(sectionUnchanged) + L" unchanged, " +
                                    std::to_wstring(sectionFailed) + L" failed");
        };

        for (const Entry& entry : entries) {
            if (entry.section != lastSection) {
                flushSection();
                lastSection = entry.section;
                sectionChanged = 0;
                sectionUnchanged = 0;
                sectionFailed = 0;
                PostLog(LOG_HEADING, std::wstring(L"[") + entry.section + L"]");
            }
            if (entry.state == State::Changed) {
                ++changed;
                ++sectionChanged;
                PostLog(LOG_OK, EntryLine(entry) + (dryRun ? L"   (would set)" : L""));
            } else if (entry.state == State::Failed) {
                ++failed;
                ++sectionFailed;
                PostLog(LOG_ERROR, L"FAILED " + EntryLine(entry) + L" - " + entry.error);
            } else {
                ++unchanged;
                ++sectionUnchanged;
            }
        }
        flushSection();

        PostLog(LOG_DEFAULT, L"");
        PostLog(failed > 0 ? LOG_WARN : LOG_OK,
                L"Done - " + std::to_wstring(changed) + L" changed, " +
                    std::to_wstring(unchanged) + L" unchanged, " + std::to_wstring(failed) +
                    L" failed.",
                true);
        if (!dryRun) PostLog(LOG_WARN, L"Reboot recommended.");
    });
}

void DoScan() {
    if (g_busy) return;
    const std::set<const Tweak*> picked = CheckedTweaks();
    if (picked.empty()) {
        MessageBoxW(g_window, L"Tick at least one registry value first.", kBaseTitle,
                    MB_OK | MB_ICONINFORMATION);
        return;
    }
    const std::vector<const Section*> sections = SectionsWithChecked(picked);

    std::vector<ScanRow> rows;
    const Summary summary = ScanSections(sections, rows, FilterFor(picked));

    AppendLog(L"", LOG_DEFAULT);
    AppendLog(L"Current values", LOG_HEADING, true);

    std::wstring lastSection;
    for (const ScanRow& row : rows) {
        if (row.section != lastSection) {
            AppendLog(L"[" + row.section + L"]", LOG_HEADING);
            lastSection = row.section;
        }
        const std::wstring name = row.sub.empty() ? row.name : L"[" + row.sub + L"] " + row.name;
        if (row.failed) {
            AppendLog(L"  " + name + L": unreadable", LOG_ERROR);
        } else if (row.matches) {
            AppendLog(L"  " + name + L" = " + row.current, LOG_DETAIL);
        } else {
            AppendLog(L"  " + name + L" = " + row.current + L"  -> target " + row.target, LOG_WARN);
        }
    }

    AppendLog(L"Values: " + std::to_wstring(summary.changed + summary.unchanged) +
                  L", matching: " + std::to_wstring(summary.unchanged) +
                  L", differing: " + std::to_wstring(summary.changed),
              summary.changed > 0 ? LOG_WARN : LOG_OK);

    RefreshTreeValues();
    UpdatePerformanceMeter();
}

// Re-reads every value and repaints the tree, so the current values can be seen
// at any time without applying, previewing or scanning first.
void DoRefresh() {
    if (g_busy) return;
    RefreshTreeValues();
    UpdatePerformanceMeter();
    UpdateStatus(L"values refreshed");
    AppendLog(L"Refreshed the current value of all " + std::to_wstring(TotalValueCount()) +
                  L" values.",
              LOG_DETAIL);
}

bool ChooseBackupPath(std::wstring* path) {
    CreateDirectoryW(DefaultBackupDirectory().c_str(), nullptr);

    wchar_t buffer[MAX_PATH * 2] = {0};
    lstrcpynW(buffer, DefaultBackupPath().c_str(), MAX_PATH * 2);

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_window;
    dialog.lpstrFilter = L"GamerTCP backup (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = MAX_PATH * 2;
    dialog.lpstrDefExt = L"txt";
    dialog.lpstrTitle = L"Save a backup of the current registry values";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (!GetSaveFileNameW(&dialog)) return false;
    *path = buffer;
    return true;
}

bool ChooseBackupToRestore(std::wstring* path) {
    wchar_t buffer[MAX_PATH * 2] = {0};

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_window;
    dialog.lpstrFilter = L"GamerTCP backup (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = MAX_PATH * 2;
    dialog.lpstrTitle = L"Open a GamerTCP backup";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&dialog)) return false;
    *path = buffer;
    return true;
}

void BeginBackup() {
    if (g_busy) return;
    const std::set<const Tweak*> picked = CheckedTweaks();
    if (picked.empty()) {
        MessageBoxW(g_window, L"Tick at least one registry value first.", kBaseTitle,
                    MB_OK | MB_ICONINFORMATION);
        return;
    }
    const std::vector<const Section*> sections = SectionsWithChecked(picked);

    std::wstring file;
    if (!ChooseBackupPath(&file)) return;

    StartWorker([sections, picked, file]() {
        std::wstring error;
        if (!BackupSections(sections, file, &error, FilterFor(picked))) {
            PostLog(LOG_ERROR, L"Backup failed: " + error);
            return;
        }
        std::vector<BackupRow> rows;
        std::wstring readError;
        ReadBackupFile(file, rows, &readError);
        PostLog(LOG_OK, L"Backup written: " + file + L" (" + std::to_wstring(rows.size()) +
                            L" values)");
    });
}

void BeginRestore() {
    if (g_busy) return;
    if (!g_elevated) {
        MessageBoxW(g_window,
                    L"Restoring registry values needs administrator rights.\r\n\r\n"
                    L"Click \"Restart as administrator\" and try again.",
                    kBaseTitle, MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring file;
    if (!ChooseBackupToRestore(&file)) return;

    std::wstring error;
    std::vector<BackupRow> rows;
    if (!ReadBackupFile(file, rows, &error)) {
        MessageBoxW(g_window, (L"Cannot read this backup:\r\n\r\n" + error).c_str(), kBaseTitle,
                    MB_OK | MB_ICONERROR);
        return;
    }

    const std::wstring question = L"Restore " + std::to_wstring(rows.size()) +
                                  L" registry value(s) from this backup?\r\n\r\n" + file;
    if (MessageBoxW(g_window, question.c_str(), L"Restore values",
                    MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) {
        return;
    }

    AppendLog(L"", LOG_DEFAULT);
    AppendLog(L"Restoring " + file, LOG_HEADING, true);

    StartWorker([file]() {
        const std::wstring safety = DefaultBackupPath();
        std::wstring error;
        if (BackupSections(AllSections(), safety, &error)) {
            PostLog(LOG_DETAIL, L"Pre-restore backup: " + safety);
        } else {
            PostLog(LOG_WARN, L"Warning: pre-restore backup failed (" + error + L").");
        }

        Summary summary;
        const bool restored = RestoreBackup(
            file,
            [](const Entry& entry) {
                if (entry.state == State::Changed) {
                    PostLog(LOG_OK, L"  " + entry.name + L": " + entry.oldData + L" -> " +
                                        entry.newData);
                } else if (entry.state == State::Deleted) {
                    PostLog(LOG_OK, L"  " + entry.name + L": deleted");
                } else if (entry.state == State::Failed) {
                    PostLog(LOG_ERROR, L"  FAILED " + entry.name + L": " + entry.error);
                }
            },
            &summary, &error);

        if (!restored) {
            PostLog(LOG_ERROR, L"Restore failed: " + error);
            return;
        }
        PostLog(summary.failed > 0 ? LOG_WARN : LOG_OK,
                L"Restored " + std::to_wstring(summary.changed) + L" value(s); " +
                    std::to_wstring(summary.unchanged) + L" already matched, " +
                    std::to_wstring(summary.failed) + L" failed.",
                true);
        PostLog(LOG_WARN, L"Reboot recommended.");
    });
}

void BeginReset() {
    if (g_busy || !g_elevated) return;
    if (MessageBoxW(g_window,
                    L"Reset the TCP/IP stack and Winsock catalog?\r\n\r\n"
                    L"This is the destructive reset: netsh int ip reset is equivalent to "
                    L"removing and reinstalling TCP/IP. It deletes the per-interface keys, so "
                    L"static addresses, DHCP options and per-interface tweaks are lost and the "
                    L"connection may have to be set up again. Connections drop for a few "
                    L"seconds and a reboot is required.\r\n\r\n"
                    L"No registry values are applied here; use Apply Selected for those.",
                    L"Reset network stack", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
        return;
    }

    AppendLog(L"", LOG_DEFAULT);
    AppendLog(L"Resetting the network stack", LOG_HEADING, true);

    StartWorker([]() {
        const CommandSummary summary = RunCommands(
            ResetStackCommands(), false, [](const CommandResult& result) {
                if (result.ok) {
                    PostLog(LOG_DETAIL, L"  OK    " + result.command);
                } else {
                    PostLog(LOG_ERROR, L"  FAIL  " + result.command +
                                           (result.output.empty()
                                                ? std::wstring()
                                                : L" - " + FirstLine(result.output)));
                }
            });
        PostLog(summary.failed > 0 ? LOG_WARN : LOG_OK,
                L"Network stack reset: " + std::to_wstring(summary.succeeded) + L" succeeded, " +
                    std::to_wstring(summary.failed) + L" failed.",
                true);
        PostLog(LOG_WARN, L"Reboot recommended.");
    });
}

void ShowTeredoMenu() {
    if (g_busy) return;

    bool statusOk = false;
    std::wstring status;
    TeredoShowState(&status, &statusOk);
    if (status.empty()) status = L"Teredo status is not available on this system.";

    int optionCount = 0;
    const TeredoOption* options = TeredoOptions(&optionCount);

    TASKDIALOG_BUTTON buttons[3] = {};
    for (int i = 0; i < optionCount && i < 3; ++i) {
        buttons[i].nButtonID = 100 + i;
        buttons[i].pszButtonText = options[i].label;
    }

    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.hwndParent = g_window;
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
    config.pszWindowTitle = L"GamerTCP 4 - Teredo";
    config.pszMainIcon = TD_INFORMATION_ICON;
    config.pszMainInstruction = L"Teredo tunneling";
    config.pszContent = status.c_str();
    config.pButtons = buttons;
    config.cButtons = static_cast<UINT>(optionCount);
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;

    int pressed = IDCANCEL;
    const HRESULT result = TaskDialogIndirect(&config, &pressed, nullptr, nullptr);
    if (FAILED(result)) {
        if (MessageBoxW(g_window, L"Disable Teredo?", L"Teredo",
                        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
            pressed = 100;
        } else {
            return;
        }
    }
    if (pressed < 100 || pressed >= 100 + optionCount) return;

    const TeredoOption option = options[pressed - 100];
    AppendLog(L"", LOG_DEFAULT);
    AppendLog(std::wstring(L"Teredo: ") + option.label, LOG_HEADING, true);

    StartWorker([option]() {
        std::wstring output;
        DWORD code = 1;
        const bool ok = TeredoSetState(option.netsh, &output, &code);
        if (!output.empty()) {
            for (const std::wstring& line : SplitList(output, L'\n'))
                PostLog(LOG_DETAIL, L"  " + line);
        }
        PostLog(ok ? LOG_OK : LOG_ERROR,
                ok ? L"Teredo state updated."
                   : L"Failed to set the Teredo state (exit code " + std::to_wstring(code) + L").");
    });
}

// Full BSD 3-Clause text, shown in the About dialog's details area. Kept in
// step with LICENSE.md.
const wchar_t* const kLicenseText =
    L"BSD 3-Clause License\r\n"
    L"\r\n"
    L"GamerTCP 4 - native C++ reimplementation of the GameTCP3 tweak set.\r\n"
    L"Copyright (c) 2026, GamerTCP contributors.\r\n"
    L"\r\n"
    L"The registry tweak set, values, section contents, netsh command list and Teredo\r\n"
    L"options are derived from GameTCP / GameTCP3:\r\n"
    L"Copyright (c) 2022, Charles McDonald. All rights reserved.\r\n"
    L"\r\n"
    L"Redistribution and use in source and binary forms, with or without\r\n"
    L"modification, are permitted provided that the following conditions are met:\r\n"
    L"\r\n"
    L"1. Redistributions of source code must retain the above copyright notice, this\r\n"
    L"   list of conditions and the following disclaimer.\r\n"
    L"\r\n"
    L"2. Redistributions in binary form must reproduce the above copyright notice,\r\n"
    L"   this list of conditions and the following disclaimer in the documentation\r\n"
    L"   and/or other materials provided with the distribution.\r\n"
    L"\r\n"
    L"3. Neither the name of the copyright holder nor the names of its contributors\r\n"
    L"   may be used to endorse or promote products derived from this software\r\n"
    L"   without specific prior written permission.\r\n"
    L"\r\n"
    L"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\r\n"
    L"AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\r\n"
    L"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\r\n"
    L"DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\r\n"
    L"FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\r\n"
    L"DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\r\n"
    L"SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\r\n"
    L"CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\r\n"
    L"OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\r\n"
    L"OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.";

void ShowAbout() {
    const std::wstring version = std::wstring(L"GamerTCP ") + VersionString();

    const std::wstring content =
        L"Windows 10/11 network optimization toolkit\r\n\r\n"
        L"BSD 3-Clause License\r\n"
        L"Copyright (c) 2026, GamerTCP contributors.\r\n"
        L"Tweak set derived from GameTCP / GameTCP3, Copyright (c) 2022 Charles McDonald."
        L"\r\n\r\n"
        L"Open \"Show licence details\" for the full licence text, or see LICENSE.md in the "
        L"project folder.";

    const std::wstring footer =
        L"Experimental software: registry and TCP/IP tuning is not universally beneficial and "
        L"comes with no warranty.";

    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.hwndParent = g_window;
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
    config.pszWindowTitle = L"About GamerTCP 4";
    config.pszMainIcon = TD_INFORMATION_ICON;
    config.pszMainInstruction = version.c_str();
    config.pszContent = content.c_str();
    config.pszExpandedInformation = kLicenseText;
    config.pszExpandedControlText = L"Hide licence details";
    config.pszCollapsedControlText = L"Show licence details";
    config.pszFooter = footer.c_str();
    config.dwCommonButtons = TDCBF_OK_BUTTON;

    if (FAILED(TaskDialogIndirect(&config, nullptr, nullptr, nullptr))) {
        const std::wstring fallback = version + L"\r\n\r\n" +
                                      L"BSD 3-Clause License. See LICENSE.md for the full "
                                      L"licence text.";
        MessageBoxW(g_window, fallback.c_str(), L"About GamerTCP 4", MB_OK | MB_ICONINFORMATION);
    }
}

void SetAllChecks(bool checked) {
    for (HTREEITEM section = TreeView_GetRoot(g_tree); section;
         section = TreeView_GetNextSibling(g_tree, section)) {
        SetItemChecked(section, checked);
        for (HTREEITEM child = TreeView_GetChild(g_tree, section); child;
             child = TreeView_GetNextSibling(g_tree, child)) {
            SetItemChecked(child, checked);
        }
    }
    UpdateStatus();
}

void RestartElevated() {
    std::wstring error;
    if (RelaunchElevated(L"--gui", &error)) {
        DestroyWindow(g_window);
        return;
    }
    MessageBoxW(g_window, (L"Could not restart with administrator rights:\r\n\r\n" + error).c_str(),
                kBaseTitle, MB_OK | MB_ICONERROR);
}

// ------------------------------------------------------------- appearance

ThemeMode ModeForOption(int index) {
    if (index == 0) return THEME_LIGHT;
    if (index == 1) return THEME_DARK;
    return THEME_SYSTEM;
}

void LayoutSettings(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    const int margin = Px(22);
    const int rowHeight = Px(58);
    const int gap = Px(10);
    int top = Px(84);
    for (int i = 0; i < 3; ++i) {
        g_settingsOptions[i] = {margin, top, client.right - margin, top + rowHeight};
        top += rowHeight + gap;
    }
    const int buttonWidth = Px(96);
    const int buttonHeight = Px(32);
    g_settingsDone = {client.right - margin - buttonWidth, client.bottom - Px(18) - buttonHeight,
                      client.right - margin, client.bottom - Px(18)};
}

int SettingsHitTest(POINT point) {
    for (int i = 0; i < 3; ++i) {
        if (PtInRect(&g_settingsOptions[i], point)) return i;
    }
    if (PtInRect(&g_settingsDone, point)) return 3;
    return -1;
}

void PaintSettings(HDC dc, HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    const Palette& c = Colors();
    FillRect(dc, &client, g_background ? g_background : GetSysColorBrush(COLOR_WINDOW));

    RECT heading = {Px(22), Px(14), client.right - Px(22), Px(46)};
    DrawLabel(dc, L"Appearance", heading, g_fontTitle, c.text,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT subtitle = {Px(22), Px(48), client.right - Px(22), Px(70)};
    DrawLabel(dc, L"Choose a colour scheme. System follows the Windows app theme.", subtitle,
              g_fontUi, c.muted, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

    static const wchar_t* const kTitles[3] = {L"Light", L"Dark", L"System"};
    static const wchar_t* const kDetails[3] = {
        L"Bright surfaces with dark text.",
        L"Dark surfaces with accent highlights.",
        L"Match the Windows app theme automatically.",
    };

    for (int i = 0; i < 3; ++i) {
        const RECT row = g_settingsOptions[i];
        const bool active = g_themeMode == ModeForOption(i);
        const bool hot = g_settingsHover == i;
        FillRounded(dc, row, Px(10), hot ? c.hover : c.panel, active ? c.accent : c.border);

        const int centreX = row.left + Px(22);
        const int centreY = (row.top + row.bottom) / 2;
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
        HPEN ring = CreatePen(PS_SOLID, Px(2), active ? c.accent : c.muted);
        HGDIOBJ oldPen = SelectObject(dc, ring);
        Ellipse(dc, centreX - Px(7), centreY - Px(7), centreX + Px(7), centreY + Px(7));
        SelectObject(dc, oldPen);
        DeleteObject(ring);
        if (active) {
            HBRUSH dot = CreateSolidBrush(c.accent);
            oldBrush = SelectObject(dc, dot);
            Ellipse(dc, centreX - Px(4), centreY - Px(4), centreX + Px(4), centreY + Px(4));
            SelectObject(dc, oldBrush);
            DeleteObject(dot);
        } else {
            SelectObject(dc, oldBrush);
        }

        RECT title = {row.left + Px(44), row.top + Px(8), row.right - Px(16), row.top + Px(30)};
        DrawLabel(dc, kTitles[i], title, g_fontUiBold, c.text,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        RECT detail = {row.left + Px(44), row.top + Px(30), row.right - Px(16), row.bottom - Px(6)};
        DrawLabel(dc, kDetails[i], detail, g_fontUi, c.muted,
                  DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    const bool doneHot = g_settingsHover == 3;
    FillRounded(dc, g_settingsDone, Px(8),
                doneHot ? Shade(c.accent, g_dark ? 16 : -10) : c.accent, c.accent);
    DrawLabel(dc, L"Done", g_settingsDone, g_fontUiBold, c.accentText,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

LRESULT CALLBACK SettingsProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            LayoutSettings(window);
            return 0;

        case WM_SIZE:
            LayoutSettings(window);
            InvalidateRect(window, nullptr, TRUE);
            return 0;

        case WM_ERASEBKGND: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            RECT client{};
            GetClientRect(window, &client);
            FillRect(dc, &client, g_background ? g_background : GetSysColorBrush(COLOR_WINDOW));
            return 1;
        }

        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            RECT client{};
            GetClientRect(window, &client);
            HDC buffer = CreateCompatibleDC(dc);
            HBITMAP bitmap = CreateCompatibleBitmap(dc, client.right, client.bottom);
            HGDIOBJ oldBitmap = SelectObject(buffer, bitmap);
            PaintSettings(buffer, window);
            BitBlt(dc, 0, 0, client.right, client.bottom, buffer, 0, 0, SRCCOPY);
            SelectObject(buffer, oldBitmap);
            DeleteObject(bitmap);
            DeleteDC(buffer);
            EndPaint(window, &paint);
            return 0;
        }

        case WM_MOUSEMOVE: {
            POINT point{};
            point.x = static_cast<short>(LOWORD(lParam));
            point.y = static_cast<short>(HIWORD(lParam));
            const int hover = SettingsHitTest(point);
            if (hover != g_settingsHover) {
                g_settingsHover = hover;
                InvalidateRect(window, nullptr, TRUE);
            }
            if (!g_settingsTracking) {
                TRACKMOUSEEVENT track{};
                track.cbSize = sizeof(track);
                track.dwFlags = TME_LEAVE;
                track.hwndTrack = window;
                TrackMouseEvent(&track);
                g_settingsTracking = true;
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            g_settingsTracking = false;
            g_settingsHover = -1;
            InvalidateRect(window, nullptr, TRUE);
            return 0;

        case WM_LBUTTONUP: {
            POINT point{};
            point.x = static_cast<short>(LOWORD(lParam));
            point.y = static_cast<short>(HIWORD(lParam));
            const int hit = SettingsHitTest(point);
            if (hit >= 0 && hit < 3) {
                SetThemeMode(ModeForOption(hit));
                InvalidateRect(window, nullptr, TRUE);
            } else if (hit == 3) {
                DestroyWindow(window);
            }
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE || wParam == VK_RETURN) {
                DestroyWindow(window);
                return 0;
            }
            if (wParam == VK_UP || wParam == VK_DOWN) {
                int mode = static_cast<int>(g_themeMode);
                mode = wParam == VK_UP ? (mode + 2) % 3 : (mode + 1) % 3;
                SetThemeMode(static_cast<ThemeMode>(mode));
                return 0;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(window);
            return 0;

        case WM_DESTROY:
            g_settings = nullptr;
            g_settingsHover = -1;
            g_settingsTracking = false;
            if (g_window) {
                InvalidateRect(g_window, nullptr, TRUE);
                SetForegroundWindow(g_window);
            }
            return 0;

        default:
            break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void ShowAppearance() {
    if (g_settings) {
        ShowWindow(g_settings, SW_SHOW);
        SetForegroundWindow(g_settings);
        return;
    }

    RECT parent{};
    GetWindowRect(g_window, &parent);
    RECT frame = {0, 0, Px(440), Px(372)};
    AdjustWindowRectEx(&frame, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME);
    const int width = frame.right - frame.left;
    const int height = frame.bottom - frame.top;
    const int x = parent.left + ((parent.right - parent.left) - width) / 2;
    const int y = parent.top + ((parent.bottom - parent.top) - height) / 2;

    g_settings = CreateWindowExW(WS_EX_DLGMODALFRAME, kSettingsClass, L"Appearance",
                                 WS_POPUP | WS_CAPTION | WS_SYSMENU, x, y, width, height, g_window,
                                 nullptr, g_instance, nullptr);
    if (!g_settings) return;
    ShowWindow(g_settings, SW_SHOW);
    SetForegroundWindow(g_settings);
    SetFocus(g_settings);
}

// ------------------------------------------------------------- power plans

struct PowerPlan {
    GUID         guid{};
    std::wstring name;
    std::wstring description;
    bool         active = false;
};

std::vector<PowerPlan> g_powerPlans;
std::wstring g_powerActiveName;
int  g_powerSelected = 0;
int  g_powerHover = -1;
int  g_powerScroll = 0;
bool g_powerTracking = false;
RECT g_powerRowsArea = {0, 0, 0, 0};
RECT g_powerApply = {0, 0, 0, 0};
RECT g_powerClose = {0, 0, 0, 0};

std::wstring ReadPowerText(const GUID& guid, bool description) {
    DWORD size = 0;
    DWORD status = description
                       ? PowerReadDescription(nullptr, &guid, nullptr, nullptr, nullptr, &size)
                       : PowerReadFriendlyName(nullptr, &guid, nullptr, nullptr, nullptr, &size);
    if (status != ERROR_SUCCESS || size == 0) return L"";

    std::vector<BYTE> buffer(size + sizeof(wchar_t) * 2, 0);
    size = static_cast<DWORD>(buffer.size());
    status = description
                 ? PowerReadDescription(nullptr, &guid, nullptr, nullptr, buffer.data(), &size)
                 : PowerReadFriendlyName(nullptr, &guid, nullptr, nullptr, buffer.data(), &size);
    if (status != ERROR_SUCCESS) return L"";
    return std::wstring(reinterpret_cast<const wchar_t*>(buffer.data()));
}

std::vector<PowerPlan> LoadPowerPlans(std::wstring* activeName) {
    std::vector<PowerPlan> plans;

    GUID activeGuid{};
    bool haveActive = false;
    GUID* active = nullptr;
    if (PowerGetActiveScheme(nullptr, &active) == ERROR_SUCCESS && active) {
        activeGuid = *active;
        haveActive = true;
        LocalFree(active);
    }

    for (DWORD index = 0;; ++index) {
        GUID guid{};
        DWORD size = sizeof(guid);
        const DWORD status = PowerEnumerate(nullptr, nullptr, nullptr, ACCESS_SCHEME, index,
                                            reinterpret_cast<PUCHAR>(&guid), &size);
        if (status == ERROR_NO_MORE_ITEMS) break;
        if (status != ERROR_SUCCESS) break;

        bool duplicate = false;
        for (const PowerPlan& plan : plans) {
            if (IsEqualGUID(plan.guid, guid)) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;

        PowerPlan plan;
        plan.guid = guid;
        plan.name = ReadPowerText(guid, false);
        if (plan.name.empty()) continue;
        plan.description = ReadPowerText(guid, true);
        plan.active = haveActive && IsEqualGUID(guid, activeGuid);
        if (plan.active && activeName) *activeName = plan.name;
        plans.push_back(std::move(plan));
    }

    return plans;
}

bool SetPowerPlan(const GUID& guid, std::wstring* error) {
    const DWORD status = PowerSetActiveScheme(nullptr, &guid);
    if (status == ERROR_SUCCESS) return true;
    if (error) *error = FormatWin32Error(static_cast<LONG>(status));
    return false;
}

int PowerRowStep() { return Px(58) + Px(8); }

int VisiblePowerRows(HWND window) {
    (void)window;
    const int height = g_powerRowsArea.bottom - g_powerRowsArea.top;
    if (height <= 0) return 1;
    const int rows = (height + Px(8)) / PowerRowStep();
    return rows > 0 ? rows : 1;
}

void LayoutPower(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    const int margin = Px(22);
    const int buttonHeight = Px(32);
    g_powerApply = {client.right - margin - Px(104), client.bottom - Px(16) - buttonHeight,
                    client.right - margin, client.bottom - Px(16)};
    g_powerClose = {g_powerApply.left - Px(8) - Px(88), g_powerApply.top,
                    g_powerApply.left - Px(8), g_powerApply.bottom};
    g_powerRowsArea = {margin, Px(84), client.right - margin, client.bottom - Px(64)};
}

void ClampPowerScroll(HWND window) {
    const int visible = VisiblePowerRows(window);
    const int maxScroll = (std::max)(0, static_cast<int>(g_powerPlans.size()) - visible);
    if (g_powerScroll > maxScroll) g_powerScroll = maxScroll;
    if (g_powerScroll < 0) g_powerScroll = 0;
    if (g_powerSelected >= 0) {
        if (g_powerSelected < g_powerScroll)
            g_powerScroll = g_powerSelected;
        else if (g_powerSelected >= g_powerScroll + visible)
            g_powerScroll = g_powerSelected - visible + 1;
    }
}

int PowerHitTest(HWND window, POINT point) {
    if (PtInRect(&g_powerApply, point)) return -2;
    if (PtInRect(&g_powerClose, point)) return -3;

    const int visible = VisiblePowerRows(window);
    const bool scrolling = static_cast<int>(g_powerPlans.size()) > visible;
    const int right = g_powerRowsArea.right - (scrolling ? Px(10) : 0);
    for (int row = 0; row < visible; ++row) {
        const int index = g_powerScroll + row;
        if (index >= static_cast<int>(g_powerPlans.size())) break;
        const int top = g_powerRowsArea.top + row * PowerRowStep();
        RECT rect = {g_powerRowsArea.left, top, right, top + Px(58)};
        if (PtInRect(&rect, point)) return index;
    }
    return -1;
}

void PaintPower(HDC dc, HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    const Palette& c = Colors();
    FillRect(dc, &client, g_background ? g_background : GetSysColorBrush(COLOR_WINDOW));

    RECT heading = {Px(22), Px(14), client.right - Px(22), Px(46)};
    DrawLabel(dc, L"Power plan", heading, g_fontTitle, c.text,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    const std::wstring subtitle = g_powerActiveName.empty()
                                      ? L"Windows did not report an active power plan."
                                      : L"Currently active: " + g_powerActiveName;
    RECT sub = {Px(22), Px(48), client.right - Px(22), Px(70)};
    DrawLabel(dc, subtitle.c_str(), sub, g_fontUi, c.muted,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

    if (g_powerPlans.empty()) {
        RECT empty = {Px(22), Px(88), client.right - Px(22), Px(150)};
        DrawLabel(dc, L"No power plans were reported by Windows.", empty, g_fontUi, c.warning,
                  DT_LEFT | DT_TOP | DT_WORDBREAK);
    } else {
        const int visible = VisiblePowerRows(window);
        const bool scrolling = static_cast<int>(g_powerPlans.size()) > visible;
        const int rowRight = g_powerRowsArea.right - (scrolling ? Px(10) : 0);

        const int saved = SaveDC(dc);
        HRGN clip = CreateRectRgn(g_powerRowsArea.left, g_powerRowsArea.top,
                                  g_powerRowsArea.right, g_powerRowsArea.bottom);
        SelectClipRgn(dc, clip);

        for (int row = 0; row < visible; ++row) {
            const int index = g_powerScroll + row;
            if (index >= static_cast<int>(g_powerPlans.size())) break;
            const PowerPlan& plan = g_powerPlans[index];
            const int top = g_powerRowsArea.top + row * PowerRowStep();
            RECT rect = {g_powerRowsArea.left, top, rowRight, top + Px(58)};
            const bool hot = g_powerHover == index;
            const bool selected = index == g_powerSelected;
            const bool active = plan.active;
            FillRounded(dc, rect, Px(10), hot ? c.hover : c.panel,
                        (selected || active) ? c.accent : c.border);

            const int centreX = rect.left + Px(22);
            const int centreY = (rect.top + rect.bottom) / 2;
            HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
            HPEN ring = CreatePen(PS_SOLID, Px(2), active ? c.accent : c.muted);
            HGDIOBJ oldPen = SelectObject(dc, ring);
            Ellipse(dc, centreX - Px(7), centreY - Px(7), centreX + Px(7), centreY + Px(7));
            SelectObject(dc, oldPen);
            DeleteObject(ring);
            if (active) {
                HBRUSH dot = CreateSolidBrush(c.accent);
                oldBrush = SelectObject(dc, dot);
                Ellipse(dc, centreX - Px(4), centreY - Px(4), centreX + Px(4), centreY + Px(4));
                SelectObject(dc, oldBrush);
                DeleteObject(dot);
            } else {
                SelectObject(dc, oldBrush);
            }

            RECT title = {rect.left + Px(44), rect.top + Px(8), rect.right - Px(16),
                          rect.top + Px(30)};
            DrawLabel(dc, plan.name.c_str(), title, g_fontUiBold, c.text,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            RECT detail = {rect.left + Px(44), rect.top + Px(30), rect.right - Px(16),
                           rect.bottom - Px(6)};
            const std::wstring description =
                plan.description.empty() ? L"Windows power plan" : plan.description;
            DrawLabel(dc, description.c_str(), detail, g_fontUi, c.muted,
                      DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

            if (active) {
                RECT badge = {rect.right - Px(74), rect.top + Px(8), rect.right - Px(20),
                              rect.top + Px(28)};
                DrawLabel(dc, L"Active", badge, g_fontUiBold, c.accent,
                          DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            }
        }

        SelectClipRgn(dc, nullptr);
        DeleteObject(clip);
        RestoreDC(dc, saved);

        if (scrolling) {
            const int trackHeight = g_powerRowsArea.bottom - g_powerRowsArea.top;
            const int thumbHeight = (std::max)(
                Px(24), trackHeight * visible / static_cast<int>(g_powerPlans.size()));
            const int maxScroll = static_cast<int>(g_powerPlans.size()) - visible;
            const int thumbTop =
                g_powerRowsArea.top +
                (trackHeight - thumbHeight) * g_powerScroll / (std::max)(1, maxScroll);
            RECT thumb = {g_powerRowsArea.right - Px(6), thumbTop, g_powerRowsArea.right,
                          thumbTop + thumbHeight};
            FillRounded(dc, thumb, Px(3), c.border, c.border);
        }
    }

    const bool canApply = g_powerSelected >= 0 &&
                          g_powerSelected < static_cast<int>(g_powerPlans.size()) &&
                          !g_powerPlans[g_powerSelected].active;
    const bool applyHot = g_powerHover == -2 && canApply;
    FillRounded(dc, g_powerApply, Px(8),
                canApply ? (applyHot ? Shade(c.accent, g_dark ? 16 : -10) : c.accent) : c.panel,
                canApply ? c.accent : c.border);
    DrawLabel(dc, L"Apply", g_powerApply, g_fontUiBold,
              canApply ? c.accentText : c.dim, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    const bool closeHot = g_powerHover == -3;
    FillRounded(dc, g_powerClose, Px(8), closeHot ? c.hover : c.panel, c.border);
    DrawLabel(dc, L"Close", g_powerClose, g_fontUi, c.text,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void ApplyPowerSelection(HWND window) {
    if (g_powerSelected < 0 || g_powerSelected >= static_cast<int>(g_powerPlans.size())) return;
    PowerPlan& plan = g_powerPlans[g_powerSelected];
    if (plan.active) return;

    std::wstring error;
    if (!SetPowerPlan(plan.guid, &error)) {
        MessageBoxW(window,
                    (L"Windows did not accept the power plan change:\r\n\r\n" + error).c_str(),
                    L"Power plan", MB_OK | MB_ICONERROR);
        return;
    }

    for (PowerPlan& item : g_powerPlans) item.active = IsEqualGUID(item.guid, plan.guid);
    g_powerActiveName = plan.name;
    AppendLog(L"", LOG_DEFAULT);
    AppendLog(L"Power plan: " + plan.name, LOG_OK, true);
    InvalidateRect(window, nullptr, TRUE);
}

LRESULT CALLBACK PowerProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            LayoutPower(window);
            ClampPowerScroll(window);
            return 0;

        case WM_SIZE:
            LayoutPower(window);
            ClampPowerScroll(window);
            InvalidateRect(window, nullptr, TRUE);
            return 0;

        case WM_ERASEBKGND: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            RECT client{};
            GetClientRect(window, &client);
            FillRect(dc, &client, g_background ? g_background : GetSysColorBrush(COLOR_WINDOW));
            return 1;
        }

        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            RECT client{};
            GetClientRect(window, &client);
            HDC buffer = CreateCompatibleDC(dc);
            HBITMAP bitmap = CreateCompatibleBitmap(dc, client.right, client.bottom);
            HGDIOBJ oldBitmap = SelectObject(buffer, bitmap);
            PaintPower(buffer, window);
            BitBlt(dc, 0, 0, client.right, client.bottom, buffer, 0, 0, SRCCOPY);
            SelectObject(buffer, oldBitmap);
            DeleteObject(bitmap);
            DeleteDC(buffer);
            EndPaint(window, &paint);
            return 0;
        }

        case WM_MOUSEMOVE: {
            POINT point{};
            point.x = static_cast<short>(LOWORD(lParam));
            point.y = static_cast<short>(HIWORD(lParam));
            const int hover = PowerHitTest(window, point);
            if (hover != g_powerHover) {
                g_powerHover = hover;
                InvalidateRect(window, nullptr, TRUE);
            }
            if (!g_powerTracking) {
                TRACKMOUSEEVENT track{};
                track.cbSize = sizeof(track);
                track.dwFlags = TME_LEAVE;
                track.hwndTrack = window;
                TrackMouseEvent(&track);
                g_powerTracking = true;
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            g_powerTracking = false;
            g_powerHover = -1;
            InvalidateRect(window, nullptr, TRUE);
            return 0;

        case WM_LBUTTONUP: {
            POINT point{};
            point.x = static_cast<short>(LOWORD(lParam));
            point.y = static_cast<short>(HIWORD(lParam));
            const int hit = PowerHitTest(window, point);
            if (hit >= 0) {
                g_powerSelected = hit;
                InvalidateRect(window, nullptr, TRUE);
            } else if (hit == -2) {
                ApplyPowerSelection(window);
            } else if (hit == -3) {
                DestroyWindow(window);
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            const int visible = VisiblePowerRows(window);
            if (static_cast<int>(g_powerPlans.size()) > visible) {
                const int lines = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA * 3;
                g_powerScroll -= lines;
                ClampPowerScroll(window);
                InvalidateRect(window, nullptr, TRUE);
            }
            return 0;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                DestroyWindow(window);
                return 0;
            }
            if (wParam == VK_RETURN) {
                ApplyPowerSelection(window);
                return 0;
            }
            if (!g_powerPlans.empty()) {
                const int last = static_cast<int>(g_powerPlans.size()) - 1;
                int selected = g_powerSelected;
                if (wParam == VK_UP) selected = selected > 0 ? selected - 1 : 0;
                else if (wParam == VK_DOWN) selected = selected < last ? selected + 1 : last;
                else if (wParam == VK_HOME) selected = 0;
                else if (wParam == VK_END) selected = last;
                else break;
                g_powerSelected = selected;
                ClampPowerScroll(window);
                InvalidateRect(window, nullptr, TRUE);
            }
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(window);
            return 0;

        case WM_DESTROY:
            g_power = nullptr;
            g_powerHover = -1;
            g_powerTracking = false;
            if (g_window) {
                InvalidateRect(g_window, nullptr, TRUE);
                SetForegroundWindow(g_window);
            }
            return 0;

        default:
            break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void ShowPowerPlans() {
    if (g_power) {
        ShowWindow(g_power, SW_SHOW);
        SetForegroundWindow(g_power);
        SetFocus(g_power);
        return;
    }

    g_powerActiveName.clear();
    g_powerPlans = LoadPowerPlans(&g_powerActiveName);
    g_powerSelected = 0;
    for (int i = 0; i < static_cast<int>(g_powerPlans.size()); ++i) {
        if (g_powerPlans[i].active) g_powerSelected = i;
    }
    g_powerHover = -1;
    g_powerScroll = 0;

    const int visible = g_powerPlans.empty()
                            ? 1
                            : (std::min)(6, static_cast<int>(g_powerPlans.size()));
    const int clientHeight = Px(84) + visible * Px(58) + (visible - 1) * Px(8) + Px(64);

    RECT parent{};
    GetWindowRect(g_window, &parent);
    RECT frame = {0, 0, Px(500), clientHeight};
    AdjustWindowRectEx(&frame, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME);
    const int width = frame.right - frame.left;
    const int height = frame.bottom - frame.top;
    const int x = parent.left + ((parent.right - parent.left) - width) / 2;
    const int y = parent.top + ((parent.bottom - parent.top) - height) / 2;

    g_power = CreateWindowExW(WS_EX_DLGMODALFRAME, kPowerClass, L"Power plan",
                              WS_POPUP | WS_CAPTION | WS_SYSMENU, x, y, width, height, g_window,
                              nullptr, g_instance, nullptr);
    if (!g_power) return;
    ShowWindow(g_power, SW_SHOW);
    SetForegroundWindow(g_power);
    SetFocus(g_power);
}

// ------------------------------------------------------------- window proc

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            CreateControls(window);
            Layout(window);
            UpdateControls();
            UpdateStatus();
            // The tree is populated before it has a size, which can leave it
            // scrolled to the last section; put it back at the top.
            SendMessageW(g_tree, WM_VSCROLL, SB_TOP, 0);
            SendMessageW(g_tree, WM_HSCROLL, SB_LEFT, 0);
            return 0;

        case WM_MOVE:
            if (!IsIconic(window) && !IsZoomed(window)) {
                GetWindowRect(window, &g_normalBounds);
                g_normalBoundsValid = true;
            }
            return 0;

        case WM_SIZE:
            if (g_tree) Layout(window);
            if (!IsIconic(window) && !IsZoomed(window)) {
                GetWindowRect(window, &g_normalBounds);
                g_normalBoundsValid = true;
            }
            return 0;

        case WM_GETMINMAXINFO: {
            MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = Px(920);
            info->ptMinTrackSize.y = Px(680);
            return 0;
        }

        case WM_ERASEBKGND: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            RECT client{};
            GetClientRect(window, &client);
            FillRect(dc, &client, g_background ? g_background : GetSysColorBrush(COLOR_WINDOW));
            return 1;
        }

        case WM_TIMER:
            if (wParam == kMeterTimer) {
                const double diff = g_performancePercent - g_displayPercent;
                if (std::fabs(diff) <= 0.5) {
                    g_displayPercent = g_performancePercent;
                    KillTimer(window, kMeterTimer);
                } else {
                    g_displayPercent += diff * 0.15;
                }
                RECT area = MeterArea();
                InflateRect(&area, Px(4), Px(4));
                InvalidateRect(window, &area, FALSE);
                return 0;
            }
            break;

        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            RECT client{};
            GetClientRect(window, &client);
            HDC buffer = CreateCompatibleDC(dc);
            HBITMAP bitmap = CreateCompatibleBitmap(dc, client.right, client.bottom);
            HGDIOBJ oldBitmap = SelectObject(buffer, bitmap);
            FillRect(buffer, &client, g_background ? g_background : GetSysColorBrush(COLOR_WINDOW));
            PaintChrome(buffer, client);
            BitBlt(dc, 0, 0, client.right, client.bottom, buffer, 0, 0, SRCCOPY);
            SelectObject(buffer, oldBitmap);
            DeleteObject(bitmap);
            DeleteDC(buffer);
            EndPaint(window, &paint);
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            const Palette& c = Colors();
            HDC dc = reinterpret_cast<HDC>(wParam);
            HWND control = reinterpret_cast<HWND>(lParam);
            SetBkColor(dc, c.background);
            if (control == g_subtitle) {
                SetTextColor(dc, c.muted);
            } else if (control == g_status) {
                SetTextColor(dc, g_elevated ? c.success : c.warning);
            } else {
                SetTextColor(dc, c.text);
            }
            return reinterpret_cast<LRESULT>(g_background);
        }

        case WM_DRAWITEM:
            if (lParam) {
                DrawButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
                return TRUE;
            }
            break;

        case WM_SETTINGCHANGE:
            if (lParam && lstrcmpW(reinterpret_cast<const wchar_t*>(lParam), L"ImmersiveColorSet") == 0) {
                ApplyTheme();
            }
            break;

        case WM_DWMCOLORIZATIONCOLORCHANGED:
            ApplyTheme();
            return 0;

        case WM_COMMAND: {
            const int id = LOWORD(wParam);
            switch (id) {
                case ID_APPLY: BeginApply(false); return 0;
                case ID_PREVIEW: BeginApply(true); return 0;
                case ID_ZGT_SETUP: BeginApply(false, true); return 0;
                case ID_SETTINGS: ShowAppearance(); return 0;
                case ID_POWER: ShowPowerPlans(); return 0;
                case ID_SCAN: DoScan(); return 0;
                case ID_REFRESH: DoRefresh(); return 0;
                case ID_RESET: BeginReset(); return 0;
                case ID_BACKUP: BeginBackup(); return 0;
                case ID_RESTORE: BeginRestore(); return 0;
                case ID_TEREDO: ShowTeredoMenu(); return 0;
                case ID_CLEAR:
                    g_logEntries.clear();
                    if (g_log) SetWindowTextW(g_log, L"");
                    return 0;
                case ID_SELECT_ALL: SetAllChecks(true); return 0;
                case ID_SELECT_NONE: SetAllChecks(false); return 0;
                case ID_ELEVATE: RestartElevated(); return 0;
                case ID_ABOUT: ShowAbout(); return 0;
                default: break;
            }
            break;
        }

        case WM_NOTIFY: {
            LPNMHDR header = reinterpret_cast<LPNMHDR>(lParam);
            if (header && header->idFrom == ID_SECTIONS) {
                if (header->code == NM_CLICK) {
                    LPNMITEMACTIVATE activate = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
                    TVHITTESTINFO hit{};
                    hit.pt = activate->ptAction;
                    const HTREEITEM item = TreeView_HitTest(g_tree, &hit);
                    if (item && (hit.flags & TVHT_ONITEMSTATEICON)) ToggleNode(item);
                } else if (header->code == TVN_KEYDOWN) {
                    LPNMTVKEYDOWN key = reinterpret_cast<LPNMTVKEYDOWN>(lParam);
                    if (key->wVKey == VK_SPACE) {
                        const HTREEITEM item = TreeView_GetSelection(g_tree);
                        if (item) ToggleNode(item);
                    }
                } else if (header->code == TVN_GETINFOTIP) {
                    LPNMTVGETINFOTIP tip = reinterpret_cast<LPNMTVGETINFOTIP>(lParam);
                    const NodeInfo* info = ItemInfo(tip->hItem);
                    if (!info) break;
                    const wchar_t* note = nullptr;
                    if (info->isSection && info->section)
                        note = info->section->summary;
                    else if (info->tweak)
                        note = info->tweak->note;
                    if (note && *note) lstrcpynW(tip->pszText, note, tip->cchTextMax);
                }
            }
            break;
        }

        case WM_APP + 1: {  // kLogMessage
            std::wstring* text = reinterpret_cast<std::wstring*>(lParam);
            if (text) {
                const int kind = static_cast<int>(wParam) & 0xFF;
                const bool bold = (static_cast<int>(wParam) & 0x100) != 0;
                AppendLog(*text, kind, bold);
                delete text;
            }
            return 0;
        }

        case WM_APP + 2:  // kDoneMessage
            g_busy = false;
            UpdateControls();
            UpdateStatus(L"done");
            RefreshTreeValues();
            UpdatePerformanceMeter();
            return 0;

        case WM_CLOSE:
            if (g_busy) {
                MessageBoxW(window,
                            L"An operation is still running.\r\n\r\n"
                            L"Wait for it to finish, then close GamerTCP.",
                            kBaseTitle, MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            DestroyWindow(window);
            return 0;

        case WM_DESTROY:
            SaveWindowPlacement(window);
            KillTimer(window, kMeterTimer);
            if (g_headerIcon) {
                if (g_headerIconOwned) DestroyIcon(g_headerIcon);
                g_headerIcon = nullptr;
                g_headerIconOwned = false;
            }
            if (g_fontUi) DeleteObject(g_fontUi);
            if (g_fontUiBold) DeleteObject(g_fontUiBold);
            if (g_fontTitle) DeleteObject(g_fontTitle);
            if (g_fontIcon) DeleteObject(g_fontIcon);
            if (g_fontLog) DeleteObject(g_fontLog);
            if (g_background) DeleteObject(g_background);
            g_fontUi = g_fontUiBold = g_fontTitle = g_fontIcon = g_fontLog = nullptr;
            g_background = nullptr;
            g_hotButtons.clear();
            g_logEntries.clear();
            for (NodeInfo* node : g_nodes) delete node;
            g_nodes.clear();
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

}  // namespace

int RunGui(HINSTANCE instance, int showCommand) {
    g_instance = instance;
    g_dpi = QueryDpi();
    g_elevated = IsElevated();
    g_sections = AllSections();
    LoadThemePreference();

    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_TREEVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&controls);

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    if (!windowClass.hIcon) windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kWindowClass;
    windowClass.hIconSm = windowClass.hIcon;
    if (!RegisterClassExW(&windowClass)) return 1;

    WNDCLASSEXW settingsClass = windowClass;
    settingsClass.lpszClassName = kSettingsClass;
    settingsClass.lpfnWndProc = SettingsProc;
    settingsClass.hbrBackground = nullptr;
    if (!RegisterClassExW(&settingsClass)) return 1;

    WNDCLASSEXW powerClass = windowClass;
    powerClass.lpszClassName = kPowerClass;
    powerClass.lpfnWndProc = PowerProc;
    powerClass.hbrBackground = nullptr;
    if (!RegisterClassExW(&powerClass)) return 1;

    RECT frame{0, 0, Px(1080), Px(760)};
    AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);
    int width = frame.right - frame.left;
    int height = frame.bottom - frame.top;
    int x = 0;
    int y = 0;
    bool maximized = false;

    RECT stored{};
    if (LoadWindowPlacement(&stored, &maximized)) {
        width = stored.right - stored.left;
        height = stored.bottom - stored.top;

        MONITORINFO monitor{};
        monitor.cbSize = sizeof(monitor);
        GetMonitorInfoW(MonitorFromRect(&stored, MONITOR_DEFAULTTONEAREST), &monitor);
        const int minWidth = Px(920);
        const int minHeight = Px(680);
        const int maxWidth = monitor.rcWork.right - monitor.rcWork.left;
        const int maxHeight = monitor.rcWork.bottom - monitor.rcWork.top;
        if (width < minWidth) width = minWidth;
        if (height < minHeight) height = minHeight;
        if (width > maxWidth) width = maxWidth;
        if (height > maxHeight) height = maxHeight;

        x = stored.left;
        y = stored.top;
        if (x < monitor.rcWork.left) x = monitor.rcWork.left;
        if (y < monitor.rcWork.top) y = monitor.rcWork.top;
        if (x + width > monitor.rcWork.right) x = monitor.rcWork.right - width;
        if (y + height > monitor.rcWork.bottom) y = monitor.rcWork.bottom - height;
        if (x < monitor.rcWork.left) x = monitor.rcWork.left;
        if (y < monitor.rcWork.top) y = monitor.rcWork.top;
    } else {
        RECT work{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        x = work.left + ((work.right - work.left) - width) / 2;
        y = work.top + ((work.bottom - work.top) - height) / 2;
    }

    g_window = CreateWindowExW(WS_EX_CONTROLPARENT, kWindowClass, kBaseTitle, WS_OVERLAPPEDWINDOW,
                               x, y, width, height, nullptr, nullptr, instance, nullptr);
    if (!g_window) return 1;

    const bool normalShow = showCommand == SW_SHOWDEFAULT || showCommand == SW_SHOWNORMAL;
    ShowWindow(g_window, maximized && normalShow
                             ? SW_SHOWMAXIMIZED
                             : (showCommand == SW_SHOWDEFAULT ? SW_SHOWNORMAL : showCommand));
    UpdateWindow(g_window);

    // F5 re-reads every value. The accelerator is translated before
    // IsDialogMessageW so it works whatever control has the focus.
    ACCEL refreshKey{};
    refreshKey.fVirt = FVIRTKEY;
    refreshKey.key = VK_F5;
    refreshKey.cmd = ID_REFRESH;
    HACCEL accelerators = CreateAcceleratorTableW(&refreshKey, 1);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (accelerators && TranslateAcceleratorW(g_window, accelerators, &message)) continue;
        if (!IsDialogMessageW(g_window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    if (accelerators) DestroyAcceleratorTable(accelerators);
    return static_cast<int>(message.wParam);
}

}  // namespace gtc
