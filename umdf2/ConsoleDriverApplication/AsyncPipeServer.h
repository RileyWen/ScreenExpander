#pragma once

#include "PublicHeader.h"
#include "_SpinLock.h"

class AsyncPipeServer {
public:
    AsyncPipeServer();
    ~AsyncPipeServer();

    bool InitConnectedPipe(_In_ DWORD dwAdvisoryBufSize);

    DWORD WriteBytes(LPCVOID pBase, DWORD dwLength);

    static LPCTSTR lpszPipeName;

private:
    DWORD EnablePipeListening();
    
    // Just a formal function required by WriteFileEx.
    static void CALLBACK WriteCompleted(
        _In_    DWORD dwErrorCode,
        _In_    DWORD dwNumberOfBytesTransfered,
        _Inout_ LPOVERLAPPED lpOverlapped);

    LPOVERLAPPED m_pConnectOverlappedArg;
    LPOVERLAPPED m_pWriteOverlappedArg;

    HANDLE m_PipeHandle;

    // Used in multi-monitor situation to guarantee
    // exclusive write operation on m_PipeHandle.
    _SpinLock m_WriteLock;
};
