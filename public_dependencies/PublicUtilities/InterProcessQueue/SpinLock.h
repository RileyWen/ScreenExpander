#pragma once

#include "pch.h"

namespace LibraryInternal {

    class SpinLock {
    public:
        SpinLock();

        SpinLock(const SpinLock&) = delete;
        SpinLock& operator= (const SpinLock&) = delete;

        void Lock();

        bool TryLock();

        void Unlock();

    private:
        volatile UINT32 m_LockValue;
    };

    static_assert(std::is_standard_layout<LibraryInternal::SpinLock>() == true,
        "SpinLock shall be standard layout!");
}
