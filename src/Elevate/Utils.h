#pragma once

namespace Utils
{
    bool IsElevated();
    LPWSTR GetRawCommandLineArgs();
    std::wstring GetCurrentProcessPath();
    std::wstring GetProcessPath(DWORD pid);
    bool IsWhiteSpace(std::wstring value);
    DWORD GetParentProcessId(DWORD pid);


    template<typename Func>
    int StandardMain(const Func implementation)
    {
        try
        {
            implementation();
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what();
            return -1;
        }
        catch (...)
        {
            std::cout << "Unknown error\r\n";
            return -1;
        }

        return 0;
    }
}