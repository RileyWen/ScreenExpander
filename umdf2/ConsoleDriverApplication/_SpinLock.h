#pragma once

#include "PublicHeader.h"

class _SpinLock {
public:
    _SpinLock();

    void Lock();

    bool TryLock();

    void Unlock();

private:
    volatile UINT32 m_LockValue;
};


