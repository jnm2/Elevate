#pragma once
#include <memory>
#include <Windows.h>

class smart_handle
{
    const std::shared_ptr<void> handle;

public:
    explicit smart_handle(HANDLE);
    operator HANDLE() const;
    operator bool() const;
};