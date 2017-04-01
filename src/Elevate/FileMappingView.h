#pragma once
#include <memory>
#include <Windows.h>
#include "smart_handle.h"

class FileMappingView
{
    const std::shared_ptr<void> handle;

public:
    FileMappingView(
        smart_handle hFileMappingObject,
        DWORD dwDesiredAccess,
        DWORD dwFileOffsetHigh,
        DWORD dwFileOffsetLow,
        SIZE_T dwNumberOfBytesToMap
    );
    operator LPVOID() const;
};
