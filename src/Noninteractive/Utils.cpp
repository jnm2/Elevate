#include <iostream>
#include <memory>
#include <Windows.h>
#include <TlHelp32.h>
#include "smart_handle.h"
#include "Win32Exception.h"

namespace Utils
{
    using namespace std;

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

    wstring GetUserObjectName(const HANDLE hObj)
    {
        auto bufferByteSize = DWORD();
        GetUserObjectInformation(hObj, UOI_NAME, nullptr, 0, &bufferByteSize);
        const auto error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER) throw Win32Exception(error);

        const auto buffer = make_unique<wchar_t[]>(bufferByteSize / 2);
        auto nameByteSize = DWORD();
        if (!GetUserObjectInformation(hObj, UOI_NAME, buffer.get(), bufferByteSize, &nameByteSize)) throw Win32Exception();

        return wstring(buffer.get(), (nameByteSize / 2) - 1);
    }
}
