#include <SDKDDKVer.h>
#include <iostream>
#include <Windows.h>
#include "Utils.h"
#include "smart_handle.h"
#include "desktop_handle.h"
#include "window_station_handle.h"
#include "Win32Exception.h"

using namespace std;


wstring GetDefaultCommandLine()
{
    return L'"' + Utils::GetProcessPath(Utils::GetParentProcessId(GetCurrentProcessId())) + L'"';
}

void ExecuteCommand(const wstring rawCommandLineArgs, const wstring* desktop)
{
    auto si = STARTUPINFO { sizeof(STARTUPINFO) };
    if (desktop) si.lpDesktop = const_cast<LPWSTR>(desktop->c_str());

    auto pi = PROCESS_INFORMATION { };

    if (!CreateProcess(nullptr, const_cast<LPWSTR>(rawCommandLineArgs.c_str()), nullptr, nullptr, false, CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si, &pi))
        throw Win32Exception();

    const auto hProcess = smart_handle(pi.hProcess);
    const auto hThread = smart_handle(pi.hThread);

    if (WaitForSingleObject(hProcess, INFINITE) == WAIT_FAILED) throw Win32Exception();
}

int main()
{
    return Utils::StandardMain([]()
    {
        auto args = wstring(Utils::GetRawCommandLineArgs());
        if (Utils::IsWhiteSpace(args))
            args = GetDefaultCommandLine();

        const auto originalWindowStation = GetProcessWindowStation();

        auto flags = USEROBJECTFLAGS { };
        if (!GetUserObjectInformation(originalWindowStation, UOI_FLAGS, &flags, sizeof(flags), nullptr)) throw Win32Exception();

        if (flags.dwFlags & WSF_VISIBLE)
        {
            const auto nonInteractiveDesktopName = L"Noninteractive.exe";

            const auto nonInteractiveStation = window_station_handle(CreateWindowStation(nullptr, 0, GENERIC_READ, nullptr));
            if (nonInteractiveStation == nullptr) throw Win32Exception();

            if (!SetProcessWindowStation(nonInteractiveStation)) throw Win32Exception();

            const auto desktop = desktop_handle(CreateDesktop(nonInteractiveDesktopName, nullptr, nullptr, 0, GENERIC_ALL, nullptr));
            if (desktop == nullptr) throw Win32Exception();

            if (!SetProcessWindowStation(originalWindowStation)) throw Win32Exception();

            const auto desktopFullName = wstring(Utils::GetUserObjectName(nonInteractiveStation) + L'\\' + nonInteractiveDesktopName);
            ExecuteCommand(args, &desktopFullName);
        }
        else
        {
            ExecuteCommand(args, nullptr);
        }
    },
    [](const char* error)
    {
         cout << error;
    });
}
