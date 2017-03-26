#include <SDKDDKVer.h>
#include <iostream>
#include <Windows.h>
#include "Utils.h"

using namespace std;


const auto attachMarker = wstring(L"ELEVATE__DO_ATTACH_SELF_TO_PARENT_CONSOLE ");

bool PopShouldAttachSelfToParent(wstring* rawCommandLineArgs)
{
    if (wcsncmp(rawCommandLineArgs->c_str(), attachMarker.c_str(), attachMarker.length()) != 0)
        return false;

    *rawCommandLineArgs = rawCommandLineArgs->substr(attachMarker.length());
    return true;
}

void AttachSelfToParent()
{
    FreeConsole();
    AttachConsole(ATTACH_PARENT_PROCESS);
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

    if (!ShellExecuteEx(&info))
        throw runtime_error("TODO: error handling");

    if (WaitForSingleObject(info.hProcess, INFINITE) == WAIT_FAILED)
        throw runtime_error("TODO: error handling");

    if (!CloseHandle(info.hProcess))
        throw runtime_error("TODO: error handling");
}

void ExecuteCommand(wstring rawCommandLineArgs)
{
    auto si = STARTUPINFO { sizeof(STARTUPINFO) };
    auto pi = PROCESS_INFORMATION { };

    if (!CreateProcess(nullptr, const_cast<LPWSTR>(rawCommandLineArgs.c_str()), nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi))
        throw runtime_error("TODO: error handling");

    if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED)
        throw runtime_error("TODO: error handling");

    if (!CloseHandle(pi.hProcess))
        throw runtime_error("TODO: error handling");

    if (!CloseHandle(pi.hThread))
        throw runtime_error("TODO: error handling");
}

int main()
{
    return Utils::StandardMain([]() -> void
    {
        // TODO: RAII win32 cleanup

        auto args = wstring(Utils::GetRawCommandLineArgs());

        if (PopShouldAttachSelfToParent(&args))
            AttachSelfToParent();

        if (Utils::IsWhiteSpace(args))
            args = GetDefaultCommandLine();

        // Elevate only after default command line has been set from current parent
        if (!Utils::IsElevated())
            return ElevateSelf(args);

        ExecuteCommand(args);
    });
}