#include <SDKDDKVer.h>
#include <iostream>
#include <Windows.h>
#include "Utils.h"
#include "smart_handle.h"
#include "Win32Exception.h"
#include "EnvironmentStrings.h"
#include "FileMappingView.h"
#include <unordered_set>

using namespace std;


const auto attachMarker = wstring(L"ELEVATE__DO_ATTACH_SELF_TO_PARENT_CONSOLE:");
auto mustReopenStdout = false;

bool PopShouldAttachSelfToParent(wstring* rawCommandLineArgs, wstring* fileMappingName)
{
    if (wcsncmp(rawCommandLineArgs->c_str(), attachMarker.c_str(), attachMarker.length()) != 0)
        return false;

    *fileMappingName = rawCommandLineArgs->substr(attachMarker.length(), 38);
    *rawCommandLineArgs = rawCommandLineArgs->substr(attachMarker.length() + 39);
    return true;
}

void AttachSelfToParent(wstring fileMappingName)
{
    FreeConsole(); // It's okay if this fails

    if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        const auto exception = Win32Exception();
        // We don't have a console, so MessageBox this one
        const auto message = string("AttachConsole failed: ") + exception.what();
        MessageBoxA(nullptr, message.c_str(), nullptr, MB_ICONERROR);
        throw exception;
    }

    // Reopening now causes intermittent "application failed to initialize properly (0xc0000142)"
    // errors in the process started by CreateProcess, and is a waste unless there is an error.
    mustReopenStdout = true;


    // Set the environment variables in this process rather than just passing to CreateProcess
    // in case it should be affecting the command line passed to CreateProcess.
    const auto hMapping = smart_handle(OpenFileMapping(FILE_MAP_READ, false, fileMappingName.c_str()));
    if (!hMapping) throw Win32Exception();

    const auto hMappingView = FileMappingView(hMapping, FILE_MAP_READ, 0, 0, 0);


    auto leftOverNames = unordered_set<wstring>();
    {
        const auto environmentStrings = EnvironmentStrings();

        for (auto variable = LPWCH(environmentStrings); *variable; variable += wcslen(variable) + 1)
        {
            auto separator = wcschr(variable, L'=');
            if (separator == variable) continue; // Missing name
            leftOverNames.emplace(variable, separator);
        }
    }

    // Add new variables
    for (auto variable = LPWCH(LPVOID(hMappingView)); *variable; variable += wcslen(variable) + 1)
    {
        auto separator = wcschr(variable, L'=');
        if (separator == variable) continue; // Missing name, causes SetEnvironmentVariable to error

        const auto name = wstring(variable, separator);
        if (!SetEnvironmentVariable(name.c_str(), separator + 1))
            throw Win32Exception();
        leftOverNames.erase(name);
    }

    // Remove existing variables for consistency
    for (auto &name : leftOverNames)
        if (!SetEnvironmentVariable(name.c_str(), nullptr))
            throw Win32Exception();
}

wstring GetDefaultCommandLine()
{
    return L'"' + Utils::GetProcessPath(Utils::GetParentProcessId(GetCurrentProcessId())) + L'"';
}

wstring GetNewGuidString()
{
    auto mappingName = GUID { };
    CoCreateGuid(&mappingName);

    const auto mappingNameString = make_unique<wchar_t[]>(39);
    StringFromGUID2(mappingName, mappingNameString.get(), 39);

    return wstring(mappingNameString.get(), 38);
}

void ElevateSelf(wstring rawCommandLineArgs)
{
    const auto environmentStrings = EnvironmentStrings();
    const auto environmentSize = environmentStrings.CalculateSize() * DWORD(sizeof(wchar_t));

    const auto mappingName = GetNewGuidString();
    const auto hMapping = smart_handle(CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, environmentSize, mappingName.c_str()));
    if (!hMapping) throw Win32Exception();

    {
        const auto hMappingView = FileMappingView(hMapping, FILE_MAP_WRITE, 0, 0, 0);
        if (!hMappingView) throw Win32Exception();
        memcpy(hMappingView, environmentStrings, environmentSize);
    }


    auto info = SHELLEXECUTEINFO { sizeof(SHELLEXECUTEINFO) };
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    info.lpVerb = L"runas";

    const auto currentProcessPath = Utils::GetCurrentProcessPath();
    info.lpFile = currentProcessPath.c_str();

    const auto parameters = attachMarker + mappingName + L" " + rawCommandLineArgs;
    info.lpParameters = parameters.c_str();

    if (!ShellExecuteEx(&info)) throw Win32Exception();

    const auto hProcess = smart_handle(info.hProcess);

    if (WaitForSingleObject(hProcess, INFINITE) == WAIT_FAILED) throw Win32Exception();
}

void ExecuteCommand(wstring rawCommandLineArgs)
{
    auto si = STARTUPINFO { sizeof(STARTUPINFO) };
    auto pi = PROCESS_INFORMATION { };

    // TODO: pass entire enviroment block to CreateProcess to enable cmd.exe's use of nameless variables to remember working directory per drive
    // https://blogs.msdn.microsoft.com/oldnewthing/20100506-00/?p=14133

    if (!CreateProcess(nullptr, const_cast<LPWSTR>(rawCommandLineArgs.c_str()), nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi))
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

        auto fileMappingName = wstring { };
        if (PopShouldAttachSelfToParent(&args, &fileMappingName))
            AttachSelfToParent(fileMappingName);

        if (Utils::IsWhiteSpace(args))
            args = GetDefaultCommandLine();

        // Elevate only after default command line has been set from current parent
        if (!Utils::IsElevated())
            return ElevateSelf(args);

        ExecuteCommand(args);
    },
        [](const char* error)
    {
        if (mustReopenStdout)
        {
            auto fp = static_cast<FILE*>(nullptr);
            freopen_s(&fp, "CONOUT$", "w+", stdout); // (Previously important: "w+" instead of "w" or else child process output will screw up)

            cout << error;

            // No cleanup necessary because the process is going down next
        }
        else
        {
            cout << error;
        }
    });
}