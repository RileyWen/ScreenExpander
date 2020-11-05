#include "_SpinLock.h"
#include "ErrorOutput.h"

_SpinLock::_SpinLock() : m_LockValue(0U) { }

void _SpinLock::Lock()
{
    // If and only if the original value of m_LockValue is 0 and 
    // after CAS its value becomes 1, we successfully acquire the lock.
    while (InterlockedExchange(&m_LockValue, 1U))
        while (m_LockValue);
}

bool _SpinLock::TryLock()
{
    return InterlockedExchange(&m_LockValue, 1U) == 0;
}

void _SpinLock::Unlock()
{
    m_LockValue = 0U;
}
