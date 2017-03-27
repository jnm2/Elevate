#include "smart_handle.h"
#include <stdexcept>

smart_handle::smart_handle(const HANDLE value) : handle(std::shared_ptr<void>(value, [](const HANDLE value)
{
    if (value && value != INVALID_HANDLE_VALUE && !CloseHandle(value))
        throw std::runtime_error("TODO: error handling");
}))
{
}

smart_handle::operator HANDLE() const
{
    return handle.get();
}

smart_handle::operator bool() const
{
    return handle.get() && handle.get() != INVALID_HANDLE_VALUE;
}
