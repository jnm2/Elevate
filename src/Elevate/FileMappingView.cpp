#include "FileMappingView.h"
#include "Win32Exception.h"
#include "smart_handle.h"

FileMappingView::FileMappingView(
    smart_handle hFileMappingObject,
    DWORD dwDesiredAccess,
    DWORD dwFileOffsetHigh,
    DWORD dwFileOffsetLow,
    SIZE_T dwNumberOfBytesToMap
)
    : handle(std::shared_ptr<void>(MapViewOfFile(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap), [](const LPVOID value)
{
    if (value && !UnmapViewOfFile(value))
        throw Win32Exception();
}))
{
    if (!handle.get()) throw Win32Exception();
}

FileMappingView::operator LPVOID() const
{
    return handle.get();
}
