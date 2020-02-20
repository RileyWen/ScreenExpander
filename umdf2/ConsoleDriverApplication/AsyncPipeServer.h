#pragma once

#include "PublicHeader.h"

class AsyncPipeServer {
public:
    AsyncPipeServer();
    ~AsyncPipeServer();

    bool InitConnectedPipe(_In_ DWORD dwAdvisoryBufSize);

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
};
