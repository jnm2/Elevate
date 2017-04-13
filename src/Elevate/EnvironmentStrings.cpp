#include "EnvironmentStrings.h"
#include "Win32Exception.h"

EnvironmentStrings::EnvironmentStrings()
    : handle(std::shared_ptr<wchar_t>(GetEnvironmentStrings(), [](const LPWCH value)
{
    if (value && !FreeEnvironmentStrings(value))
        throw Win32Exception();
}))
{
}

EnvironmentStrings::operator LPWCH() const
{
    return handle.get();
}

DWORD EnvironmentStrings::CalculateSize() const
{
    for (auto start = handle.get(), current = start;;)
    {
        const auto stringSize = wcslen(current);
        current += stringSize + 1;
        if (stringSize == 0) return static_cast<DWORD>(current - start);
    }
}
