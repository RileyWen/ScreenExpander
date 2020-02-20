#pragma once

#include "pch.h"

class SpinLock {
public:
    SpinLock();

    void Lock();

    bool TryLock();

    void Unlock();

private:
    volatile UINT32 m_LockValue;
};


