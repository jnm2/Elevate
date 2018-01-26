#pragma once
#include <memory>
#include <Windows.h>

class EnvironmentStrings
{
    const std::shared_ptr<wchar_t> handle;

public:
    EnvironmentStrings();
    operator LPWCH() const;
    DWORD CalculateSize() const;
};
