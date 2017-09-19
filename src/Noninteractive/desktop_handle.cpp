#include "desktop_handle.h"
#include <stdexcept>
#include "Win32Exception.h"

desktop_handle::desktop_handle(const HDESK value) : handle(std::shared_ptr<void>(value, [](const HDESK value)
{
    if (value && !CloseDesktop(value))
        throw Win32Exception();
}))
{
}

desktop_handle::operator HDESK() const
{
    return HDESK(handle.get());
}

desktop_handle::operator bool() const
{
    return handle.get();
}
