#include <iostream>
#include <memory>
#include <Windows.h>
#include <TlHelp32.h>
#include "smart_handle.h"
#include "Win32Exception.h"

namespace Utils
{
    using namespace std;

    bool IsElevated()
    {
        auto hTokenOutParameter = HANDLE(nullptr);
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTokenOutParameter))
            throw Win32Exception();
        const auto hToken = smart_handle(hTokenOutParameter);

        auto elevation = TOKEN_ELEVATION { };
        auto cbSize = DWORD(sizeof(TOKEN_ELEVATION));
        if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize))
            throw Win32Exception();

        return elevation.TokenIsElevated;
    }

    LPWSTR GetRawCommandLineArgs()
    {
        auto c = GetCommandLine();
        if (!c) return nullptr;

        if (*c == '"')
        {
            c++;
            while (*c && *c++ != '"') { }
        }
        else
        {
            while (*c && !isspace(*c++)) { }
        }

        while (*c && isspace(*c)) c++;

        return c;
    }

    wstring GetCurrentProcessPath()
    {
        for (auto bufferSize = MAX_PATH; ; bufferSize *= 2)
        {
            const auto buffer = make_unique<wchar_t[]>(bufferSize);
            const auto nameSize = GetModuleFileName(nullptr, buffer.get(), bufferSize);
            if (nameSize == 0) throw Win32Exception();

            if (!(nameSize == bufferSize && buffer[nameSize - 1]) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                return wstring(buffer.get(), nameSize);
        }
    }

    wstring GetProcessPath(const DWORD pid)
    {
        const auto hProcess = smart_handle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid));
        if (hProcess == nullptr) throw Win32Exception();

        for (auto bufferSize = MAX_PATH; ; bufferSize *= 2)
        {
            const auto buffer = make_unique<wchar_t[]>(bufferSize);
            auto nameSize = DWORD(bufferSize);
            if (!QueryFullProcessImageName(hProcess, 0, buffer.get(), &nameSize))
            {
                const auto error = GetLastError();
                if (error == ERROR_INSUFFICIENT_BUFFER) continue;
                throw Win32Exception(error);
            }

            return wstring(buffer.get(), nameSize);
        }
    }

    bool IsWhiteSpace(const wstring value)
    {
        for (auto &c : value)
            if (!isspace(c)) return false;
        return true;
    }

    DWORD GetParentProcessId(const DWORD pid)
    {
        const auto hSnapshot = smart_handle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
        if (hSnapshot == INVALID_HANDLE_VALUE) throw Win32Exception();

        auto processEntry = PROCESSENTRY32 { sizeof(PROCESSENTRY32) };
        if (Process32First(hSnapshot, &processEntry))
        {
            do
            {
                if (processEntry.th32ProcessID == pid)
                    return processEntry.th32ParentProcessID;
            } while (Process32Next(hSnapshot, &processEntry));
        }

        return 0;
    }
}
