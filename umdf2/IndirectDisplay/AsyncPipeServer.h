#pragma once

#include "pch.h"
#include "SpinLock.h"

class AsyncPipeServer {
public:
    AsyncPipeServer();
    ~AsyncPipeServer();
    
    // Make the pipe start listening connection from clients.
    // If failed, a false is returned and this function guarantee that 
    // no pipe is actually created.
    bool InitConnectedPipe(_In_ DWORD dwAdvisoryBufSize);

    // This function return S_OK when the write operation on the pipe
    // is suucess or when the pipe is on listening state. If the pipe
    // is on listening state, this function just drop all data. If any
    // other errors happen, this function returns GetLastError().
    DWORD WriteBytes(LPCVOID pBase, DWORD dwLength);

    static LPCTSTR lpszPipeName;

private:
    DWORD EnablePipeListening();

    static void CALLBACK WriteCompleted(
        _In_    DWORD dwErrorCode,
        _In_    DWORD dwNumberOfBytesTransfered,
        _Inout_ LPOVERLAPPED lpOverlapped);

    LPOVERLAPPED m_pConnectOverlappedArg;
    LPOVERLAPPED m_pWriteOverlappedArg;

    HANDLE m_PipeHandle;

    // Used in multi-monitor situation to guarantee
    // exclusive write operation on m_PipeHandle.
    //
    // Use spin lock here since we assume that,
    // in the most time, there're no writing operations.
    SpinLock m_WriteLock;
};
