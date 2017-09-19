#pragma once
#include <memory>
#include <Windows.h>

class desktop_handle
{
    const std::shared_ptr<void> handle;

public:
    explicit desktop_handle(HDESK);
    operator HDESK() const;
    operator bool() const;
};
