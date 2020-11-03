#include "pch.h"
#include "InterProcessQueue.h"

namespace LibraryInternal {
    SharedMemoryPool::SharedMemoryPool(const std::string& SharedMemName, size_t size)
        : m_SharedMemName_(SharedMemName), m_MemSize_(size), m_hMappedFile_(INVALID_HANDLE_VALUE), m_pMappedMemAddr_(nullptr)
    {
    }

    SharedMemoryPool::~SharedMemoryPool()
    {
        if (m_pMappedMemAddr_ != nullptr)
            UnmapViewOfFile(m_pMappedMemAddr_);

        if (m_hMappedFile_ != INVALID_HANDLE_VALUE)
            CloseHandle(m_hMappedFile_);
    }

    void SharedMemoryPool::Create()
    {
#pragma warning( push )
#pragma warning( disable : 4267 )

        m_hMappedFile_ = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            m_MemSize_,
            m_SharedMemName_.c_str()
        );

#pragma warning( pop )

        if (m_hMappedFile_ == NULL)
            throw WinApiException("[SharedMemoryPool] CreateFileMappingA");

        m_pMappedMemAddr_ = MapViewOfFile(
            m_hMappedFile_,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            m_MemSize_
        );

        if (m_pMappedMemAddr_ == nullptr) {
            CloseHandle(m_hMappedFile_);

            throw WinApiException("[SharedMemoryPool] MapViewOfFile");
        }
    }

    void SharedMemoryPool::OpenExisting()
    {
        m_hMappedFile_ = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, m_SharedMemName_.c_str());

        if (m_hMappedFile_ == NULL) {
            m_hMappedFile_ = INVALID_HANDLE_VALUE;

            throw WinApiException("[SharedMemoryPool] OpenFileMappingA");
        }

        m_pMappedMemAddr_ = MapViewOfFile(
            m_hMappedFile_,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            m_MemSize_
        );

        if (m_pMappedMemAddr_ == nullptr) {
            CloseHandle(m_hMappedFile_);
            m_hMappedFile_ = INVALID_HANDLE_VALUE;

            throw WinApiException("[SharedMemoryPool] MapViewOfFile");
        }
    }

    Semaphore::Semaphore(const std::string& SemaphoreName)
        : m_SemaphoreName(SemaphoreName), m_hSemaphore_(INVALID_HANDLE_VALUE)
    {
    }

    Semaphore::~Semaphore()
    {
        if (m_hSemaphore_ == INVALID_HANDLE_VALUE)
            CloseHandle(m_hSemaphore_);
    }

    void Semaphore::Create(size_t MaxCount)
    {
#pragma warning( push )
#pragma warning( disable : 4267 )

        m_hSemaphore_ = CreateSemaphoreA(
            NULL,                       // default security attributes
            0,                          // initial count
            MaxCount,                   // maximum count
            m_SemaphoreName.c_str());   // unnamed semaphore

#pragma warning( pop )

        if (m_hSemaphore_ == NULL)
            throw WinApiException("[Semaphore] CreateSemphoreA");
    }

    void Semaphore::OpenExisting()
    {
        m_hSemaphore_ = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, m_SemaphoreName.c_str());

        if (m_hSemaphore_ == NULL)
            throw WinApiException("[Semaphore] OpenSemphoreA");
    }

    bool Semaphore::TryReleaseOne() noexcept
    {
        return static_cast<bool>(ReleaseSemaphore(m_hSemaphore_, 1L, NULL));
    }

    void Semaphore::WaitOne()
    {
        DWORD dwResult;

        dwResult = WaitForSingleObject(m_hSemaphore_, 1000 * 5);

        switch (dwResult)
        {
        case WAIT_OBJECT_0:
            return;

        case WAIT_TIMEOUT:
            throw WaitTimeoutException();
        case WAIT_FAILED:
        default:
            throw WinApiException("[Semaphore] WaitOne");
        }
    }
}
