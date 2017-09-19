#include "window_station_handle.h"
#include <stdexcept>
#include "Win32Exception.h"

window_station_handle::window_station_handle(const HWINSTA value) : handle(std::shared_ptr<void>(value, [](const HWINSTA value)
{
    if (value && !CloseWindowStation(value))
        throw Win32Exception();
}))
{
}

window_station_handle::operator HWINSTA() const
{
    return HWINSTA(handle.get());
}

window_station_handle::operator bool() const
{
    return handle.get();
}
