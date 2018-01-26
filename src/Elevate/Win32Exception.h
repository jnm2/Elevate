#pragma once
#include <stdexcept>
#include <Windows.h>

class Win32Exception : public std::runtime_error
{
    DWORD errorCode;

public:
    Win32Exception();
    Win32Exception(DWORD errorCode);
    DWORD ErrorCode() const;
};
