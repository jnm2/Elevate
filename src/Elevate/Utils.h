#pragma once

namespace Utils
{
    bool IsElevated();
    LPWSTR GetRawCommandLineArgs();
    std::wstring GetCurrentProcessPath();
    std::wstring GetProcessPath(DWORD pid);
    bool IsWhiteSpace(std::wstring value);
    DWORD GetParentProcessId(DWORD pid);


    template<typename TImplementation, typename TErrorHandler>
    int StandardMain(const TImplementation implementation, const TErrorHandler errorHandler)
    {
        try
        {
            implementation();
        }
        catch (const std::exception& ex)
        {
            errorHandler(ex.what());
            return -1;
        }
        catch (...)
        {
            errorHandler("Unknown error\r\n");
            return -1;
        }

        return 0;
    }
}