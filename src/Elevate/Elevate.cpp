#include <SDKDDKVer.h>
#include <iostream>
#include <Windows.h>
#include "Utils.h"
#include "smart_handle.h"
#include "Win32Exception.h"

using namespace std;


const auto attachMarker = wstring(L"ELEVATE__DO_ATTACH_SELF_TO_PARENT_CONSOLE ");
auto mustReopenStdout = false;

bool PopShouldAttachSelfToParent(wstring* rawCommandLineArgs)
{
    if (wcsncmp(rawCommandLineArgs->c_str(), attachMarker.c_str(), attachMarker.length()) != 0)
        return false;

    *rawCommandLineArgs = rawCommandLineArgs->substr(attachMarker.length());
    return true;
}

void AttachSelfToParent()
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

    // TODO: pull environment block for CreateProcess
}

wstring GetDefaultCommandLine()
{
    return L'"' + Utils::GetProcessPath(Utils::GetParentProcessId(GetCurrentProcessId())) + L'"';
}

void ElevateSelf(wstring rawCommandLineArgs)
{
    auto info = SHELLEXECUTEINFO { sizeof(SHELLEXECUTEINFO) };
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    info.lpVerb = L"runas";

    const auto currentProcessPath = Utils::GetCurrentProcessPath();
    info.lpFile = currentProcessPath.c_str();

    const auto parameters = attachMarker + rawCommandLineArgs;
    info.lpParameters = parameters.c_str();

    if (!ShellExecuteEx(&info)) throw Win32Exception();

    const auto hProcess = smart_handle(info.hProcess);

    if (WaitForSingleObject(hProcess, INFINITE) == WAIT_FAILED) throw Win32Exception();
}

void ExecuteCommand(wstring rawCommandLineArgs)
{
    auto si = STARTUPINFO { sizeof(STARTUPINFO) };
    auto pi = PROCESS_INFORMATION { };

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

        if (PopShouldAttachSelfToParent(&args))
            AttachSelfToParent();

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