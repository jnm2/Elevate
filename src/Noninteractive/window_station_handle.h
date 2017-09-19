#pragma once
#include <memory>
#include <Windows.h>

class window_station_handle
{
    const std::shared_ptr<void> handle;

public:
    explicit window_station_handle(HWINSTA);
    operator HWINSTA() const;
    operator bool() const;
};
