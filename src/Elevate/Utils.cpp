#include <iostream>
#include <memory>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>

namespace Utils
{
    using namespace std;

    bool IsElevated()
    {
        auto hToken = HANDLE(nullptr);
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            throw runtime_error("TODO: error handling");

        auto elevation = TOKEN_ELEVATION { };
        auto cbSize = DWORD(sizeof(TOKEN_ELEVATION));
        if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize))
            throw runtime_error("TODO: error handling");

        if (hToken && !CloseHandle(hToken))
            throw runtime_error("TODO: error handling");

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
            if (nameSize == 0) throw runtime_error("TODO: error handling");

            if (!(nameSize == bufferSize && buffer[nameSize - 1]) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                return wstring(buffer.get(), nameSize);
        }
    }

    wstring GetProcessPath(const DWORD pid)
    {
        const auto hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
        if (hProcess == nullptr) throw runtime_error("TODO: error handling");

        for (auto bufferSize = MAX_PATH; ; bufferSize *= 2)
        {
            const auto buffer = make_unique<wchar_t[]>(bufferSize);
            auto nameSize = DWORD(bufferSize);
            if (!QueryFullProcessImageName(hProcess, 0, buffer.get(), &nameSize))
            {
                const auto error = GetLastError();
                if (error == ERROR_INSUFFICIENT_BUFFER) continue;
                throw runtime_error("TODO: error handling");
            }

            if (!CloseHandle(hProcess))
                throw runtime_error("TODO: error handling");

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
        const auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
            throw runtime_error("TODO: error handling");

        auto processEntry = PROCESSENTRY32 { sizeof(PROCESSENTRY32) };
        auto parentId = 0;
        if (Process32First(hSnapshot, &processEntry))
        {
            do
            {
                if (processEntry.th32ProcessID == pid)
                {
                    parentId = processEntry.th32ParentProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &processEntry));
        }

        if (!CloseHandle(hSnapshot))
            throw runtime_error("TODO: error handling");

        return parentId;
    }
}