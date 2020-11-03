#pragma once

#include "pch.h"

#include "SpinLock.h"

#include <exception>

namespace LibraryInternal {

    class SharedMemoryPool {
    public:
        explicit SharedMemoryPool(const std::string& SharedMemName, size_t size);

        ~SharedMemoryPool();

        void Create();

        void OpenExisting();

        inline size_t GetSize() { return m_MemSize_; }

        inline void* GetMemAddr() { return m_pMappedMemAddr_; }

        inline const std::string& GetName() { return m_SharedMemName_; }

    private:
        const std::string m_SharedMemName_;

        size_t m_MemSize_;

        void* m_pMappedMemAddr_;

        HANDLE m_hMappedFile_;
    };


    class Semaphore {
    public:
        Semaphore(const std::string& SemaphoreName);

        ~Semaphore();

        void Create(size_t MaxCount);

        void OpenExisting();

        // Add count by 1. Non-Blocking.
        bool TryReleaseOne() noexcept;

        // Decrease count by 1. Blocking.
        // If false is returned, an unexpected error occurs. Use GetLastError() to find out the error.
        void WaitOne();

    private:
        const std::string m_SemaphoreName;

        HANDLE m_hSemaphore_;
    };

}

class WaitTimeoutException : public std::exception {
    const char* what() const override {
        return "WaitTimeoutException! Producer may have crashed!";
    }
};

class WinApiException : public std::exception {
public:
    explicit WinApiException(const char *prefix = nullptr)
    {
        if (prefix) {
            m_what_.append(prefix);
            m_what_.append(": ");
        }

        DWORD dwErrorCode = GetLastError();

        //Get the error message, if any.
        if (dwErrorCode == 0)
            m_what_ = "No Error. May be triggered wrongly.";

        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, dwErrorCode, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        m_what_ = std::string(messageBuffer, size);

        //Free the buffer.
        LocalFree(messageBuffer);
    }

    const char* what() const override {
        return m_what_.c_str();
    }
private:
    std::string m_what_;
};

template<typename T>
class InterProcessQueue {
private:
    static_assert(std::is_standard_layout<T>(), "InterProcessQueue only support standard layout types!");


    static constexpr size_t DEFAULT_QUEUE_CAPACITY = 10;

    struct QueueMeta {
        volatile uint16_t head;
        volatile uint16_t tail;

        LibraryInternal::SpinLock lock;
    };

    static_assert(std::is_standard_layout<QueueMeta>(), "QueueMeta shall be standard layout!");

    struct QueueMeta* m_pQueueMeta_;
    T* m_pElemArray_;

    size_t m_capacity_;

    LibraryInternal::SharedMemoryPool m_SharedMemory_;

    LibraryInternal::Semaphore m_Semaphore_;

public:
    InterProcessQueue(const std::string& Name, size_t capacity = DEFAULT_QUEUE_CAPACITY)
        : m_pQueueMeta_{ nullptr }, m_pElemArray_{ nullptr }, m_capacity_{ capacity },
        m_SharedMemory_{ Name, sizeof(QueueMeta) + sizeof(T) * capacity },
        m_Semaphore_{ Name + "_SEMAPHORE" }
    {
    }

    void OpenExisting() {
        m_SharedMemory_.OpenExisting();
        m_Semaphore_.OpenExisting();

        m_pQueueMeta_ = reinterpret_cast<QueueMeta*>(m_SharedMemory_.GetMemAddr());
        m_pElemArray_ = reinterpret_cast<T*>(&m_pQueueMeta_[1]);
    }

    void Create() {
        m_SharedMemory_.Create();
        m_Semaphore_.Create(m_capacity_);

        m_pQueueMeta_ = reinterpret_cast<QueueMeta*>(m_SharedMemory_.GetMemAddr());
        m_pElemArray_ = reinterpret_cast<T*>(&m_pQueueMeta_[1]);

        m_pQueueMeta_->head = 0;
        m_pQueueMeta_->tail = 0;

        new(&m_pQueueMeta_->lock) LibraryInternal::SpinLock;
    }

    template <typename... Args>
    bool TryEmplaceBack(Args&&... args) {
        // The frame arrives every 1/144 ~ 1/60 second. Contention should be low.
        m_pQueueMeta_->lock.Lock();

        if (!m_Semaphore_.TryReleaseOne()) {
            m_pQueueMeta_->lock.Unlock();
            return false;
        }

        new(&m_pElemArray_[m_pQueueMeta_->tail]) T{ std::forward<Args>(args)... };

        m_pQueueMeta_->tail++;
        m_pQueueMeta_->tail %= m_capacity_;

        m_pQueueMeta_->lock.Unlock();

        return true;
    }

    void PopFront(T* pElem) {
        m_Semaphore_.WaitOne();

        // The frame arrives every 1/144 ~ 1/60 second. Contention should be low.
        m_pQueueMeta_->lock.Lock();

        *pElem = m_pElemArray_[m_pQueueMeta_->head];

        m_pQueueMeta_->head++;
        m_pQueueMeta_->head %= m_capacity_;

        m_pQueueMeta_->lock.Unlock();
    }
};

