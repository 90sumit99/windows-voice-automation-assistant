#include "ApplicationLauncher.h"
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <algorithm>
#include <iostream>
#include <string>

static std::wstring toWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
    return w;
}

static std::string toNarrow(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1,
                                  nullptr, 0, nullptr, nullptr);
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], len,
                        nullptr, nullptr);
    return s;
}

ApplicationLauncher::ApplicationLauncher() {
    loadAliases();
}

void ApplicationLauncher::loadAliases() {
    appAliases = {
        // Browsers
        { "chrome",         "chrome.exe"        },
        { "google chrome",  "chrome.exe"        },
        { "firefox",        "firefox.exe"       },
        { "edge",           "msedge.exe"        },
        { "microsoft edge", "msedge.exe"        },
        { "brave",          "brave.exe"         },
        { "opera",          "opera.exe"         },

        // Microsoft Office
        // NOTE: Word process is WINWORD.EXE
        { "word",           "WINWORD.EXE"       },
        { "excel",          "EXCEL.EXE"         },
        { "powerpoint",     "POWERPNT.EXE"      },
        { "outlook",        "OUTLOOK.EXE"       },
        { "onenote",        "ONENOTE.EXE"       },
        { "teams",          "Teams.exe"         },

        // Dev tools
        { "visual studio",  "devenv.exe"        },
        { "vs code",        "Code.exe"          },
        { "vscode",         "Code.exe"          },
        { "notepad",        "notepad.exe"       },
        { "notepad++",      "notepad++.exe"     },
        { "cmd",            "cmd.exe"           },
        { "terminal",       "wt.exe"            },
        { "powershell",     "powershell.exe"    },
        { "git bash",       "git-bash.exe"      },

        // Media
        // NOTE: Spotify real process is Spotify.exe (capital S)
        { "spotify",        "Spotify.exe"       },
        { "vlc",            "vlc.exe"           },
        { "media player",   "wmplayer.exe"      },

        // System tools
        { "task manager",   "taskmgr.exe"       },
        { "calculator",     "calc.exe"          },
        { "paint",          "mspaint.exe"       },
        { "snipping tool",  "SnippingTool.exe"  },
        { "control panel",  "control.exe"       },
        { "settings",       "ms-settings:"      },
        { "file explorer",  "explorer.exe"      },
        { "explorer",       "explorer.exe"      },

        // Communication
        { "discord",        "Discord.exe"       },
        { "whatsapp",       "WhatsApp.exe"      },
        { "telegram",       "Telegram.exe"      },
        { "zoom",           "Zoom.exe"          },
        { "slack",          "slack.exe"         },
    };
}

std::string ApplicationLauncher::normalizeExe(std::string name) const {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find('.') == std::string::npos)
        name += ".exe";
    return name;
}

std::string ApplicationLauncher::resolveAlias(const std::string& input) const {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    auto it = appAliases.find(lower);
    if (it != appAliases.end()) return it->second;
    return normalizeExe(input);
}

std::string ApplicationLauncher::resolveExecutable(const std::string& exeName) const {
    wchar_t buffer[MAX_PATH];
    std::wstring wExe = toWide(exeName);
    DWORD result = SearchPathW(nullptr, wExe.c_str(), nullptr,
                               MAX_PATH, buffer, nullptr);
    if (result > 0 && result < MAX_PATH)
        return toNarrow(buffer);
    return "";
}

// ─────────────────────────────────────────────────────────────
//  IS RUNNING — case-insensitive process name check
//  Also checks without extension so "WINWORD" matches "WINWORD.EXE"
// ─────────────────────────────────────────────────────────────
bool ApplicationLauncher::isRunning(const std::string& exeName) {
    // normalize target: lowercase, strip .exe for comparison
    std::string target = exeName;
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);
    // strip extension for loose matching
    std::string targetNoExt = target;
    auto dotPos = targetNoExt.rfind('.');
    if (dotPos != std::string::npos)
        targetNoExt = targetNoExt.substr(0, dotPos);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(PROCESSENTRY32W);

    bool found = false;
    if (Process32FirstW(snapshot, &pe)) {
        do {
            std::string procName = toNarrow(pe.szExeFile);
            std::transform(procName.begin(), procName.end(),
                           procName.begin(), ::tolower);

            // strip extension from process name too
            std::string procNoExt = procName;
            auto pDot = procNoExt.rfind('.');
            if (pDot != std::string::npos)
                procNoExt = procNoExt.substr(0, pDot);

            // match either full name or name without extension
            if (procName == target || procNoExt == targetNoExt) {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &pe));
    }

    CloseHandle(snapshot);
    return found;
}

bool ApplicationLauncher::open(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "[Launcher] No application name provided.\n";
        return false;
    }

    std::string input;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) input += " ";
        input += args[i];
    }

    std::string exe = resolveAlias(input);

    if (isRunning(exe)) {
        std::cout << "[Launcher] " << exe << " is already running.\n";
        return true;
    }

    std::wstring wExe = toWide(exe);
    HINSTANCE res = ShellExecuteW(nullptr, L"open", wExe.c_str(),
                                  nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<intptr_t>(res) > 32) {
        std::cout << "[Launcher] Opened: " << exe << "\n";
        return true;
    }

    std::string fullPath = resolveExecutable(exe);
    if (!fullPath.empty()) {
        std::wstring wFull = toWide(fullPath);
        res = ShellExecuteW(nullptr, L"open", wFull.c_str(),
                            nullptr, nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<intptr_t>(res) > 32) {
            std::cout << "[Launcher] Opened via PATH: " << fullPath << "\n";
            return true;
        }
    }

    std::cout << "[Launcher] Could not open: " << input << "\n";
    return false;
}

bool ApplicationLauncher::openWithArgs(const std::string& exe,
                                       const std::string& params) {
    std::wstring wExe    = toWide(exe);
    std::wstring wParams = toWide(params);
    HINSTANCE res = ShellExecuteW(nullptr, L"open", wExe.c_str(),
                                  wParams.c_str(), nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<intptr_t>(res) > 32) {
        std::cout << "[Launcher] Opened " << exe << " with: " << params << "\n";
        return true;
    }
    std::cout << "[Launcher] Failed to open with params.\n";
    return false;
}

// ─────────────────────────────────────────────────────────────
//  CLOSE — tries taskkill by both alias exe name AND
//          scanning process list for partial match
// ─────────────────────────────────────────────────────────────
bool ApplicationLauncher::close(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "[Launcher] No application name provided.\n";
        return false;
    }

    std::string input;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) input += " ";
        input += args[i];
    }

    std::string exe = resolveAlias(input);

    // Find the ACTUAL running process name (handles case differences)
    std::string actualExe = findRunningProcess(exe);
    if (actualExe.empty()) {
        std::cout << "[Launcher] " << exe << " is not running.\n";
        return false;
    }

    // Use actual process name for taskkill
    std::string cmdStr = "taskkill /IM \"" + actualExe + "\" /F";
    std::wstring wCmd  = toWide(cmdStr);
    std::vector<wchar_t> buf(wCmd.begin(), wCmd.end());
    buf.push_back(L'\0');

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    BOOL ok = CreateProcessW(nullptr, buf.data(),
                             nullptr, nullptr, FALSE,
                             CREATE_NO_WINDOW,
                             nullptr, nullptr, &si, &pi);
    if (!ok) {
        std::cout << "[Launcher] Failed to close: " << actualExe << "\n";
        return false;
    }

    WaitForSingleObject(pi.hProcess, 5000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode == 0) {
        std::cout << "[Launcher] Closed: " << actualExe << "\n";
        return true;
    }

    std::cout << "[Launcher] Could not close (UWP or protected): " << actualExe << "\n";
    return false;
}

// ─────────────────────────────────────────────────────────────
//  FIND RUNNING PROCESS — returns the EXACT process name
//  as it appears in Task Manager (correct case)
// ─────────────────────────────────────────────────────────────
std::string ApplicationLauncher::findRunningProcess(const std::string& exeName) {
    std::string target = exeName;
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);
    std::string targetNoExt = target;
    auto dotPos = targetNoExt.rfind('.');
    if (dotPos != std::string::npos)
        targetNoExt = targetNoExt.substr(0, dotPos);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return "";

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(PROCESSENTRY32W);

    std::string result;
    if (Process32FirstW(snapshot, &pe)) {
        do {
            std::string procName = toNarrow(pe.szExeFile);
            std::string procLower = procName;
            std::transform(procLower.begin(), procLower.end(),
                           procLower.begin(), ::tolower);

            std::string procNoExt = procLower;
            auto pDot = procNoExt.rfind('.');
            if (pDot != std::string::npos)
                procNoExt = procNoExt.substr(0, pDot);

            if (procLower == target || procNoExt == targetNoExt) {
                result = procName; // return exact name from OS
                break;
            }
        } while (Process32NextW(snapshot, &pe));
    }

    CloseHandle(snapshot);
    return result;
}
