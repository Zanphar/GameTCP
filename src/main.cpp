// GamerTCP - Windows 10/11 network optimization toolkit
// Native C++ reimplementation of the GameTCP3 tweak set (BSD 3-Clause).
//
// main.cpp - entry point. No arguments (or --gui / gui) opens the GUI,
// anything else is handled by the command line front end.

#include "engine.h"

#include "cli.h"
#include "gui.h"

#include <shellapi.h>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    bool gui = (argc <= 1);
    if (argv && !gui) {
        for (int i = 1; i < argc; ++i) {
            if (!argv[i]) continue;
            const std::wstring argument = gtc::Lower(argv[i]);
            if (argument == L"--gui" || argument == L"gui") {
                gui = true;
                break;
            }
        }
    }

    int code = 0;
    if (gui) {
        code = gtc::RunGui(instance, showCommand);
    } else {
        code = gtc::RunCli(argc, argv);
    }

    if (argv) LocalFree(argv);
    return code;
}
