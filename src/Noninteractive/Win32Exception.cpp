#include "Win32Exception.h"
#include <memory>


std::string GetMessage(DWORD errorCode)
{
    auto pBufferOutParameter = LPSTR(nullptr);
    const auto bufferLength = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        nullptr,
        errorCode,
        0,
        reinterpret_cast<LPSTR>(&pBufferOutParameter),
        0,
        nullptr);


    struct LocalDeleter
    {
        void operator()(HLOCAL toFree) const
        {
            LocalFree(toFree);
        }
    };
    const auto pBuffer = std::unique_ptr<char, LocalDeleter>(pBufferOutParameter);

    return std::string(pBuffer.get(), bufferLength);
}

Win32Exception::Win32Exception(DWORD errorCode) : runtime_error(GetMessage(errorCode)), errorCode(errorCode)
{
}
Win32Exception::Win32Exception() : Win32Exception(GetLastError())
{
}

DWORD Win32Exception::ErrorCode() const
{
    return errorCode;
}
