#include "pch.h"
#include "AsyncPipeServer.h"

LPCTSTR AsyncPipeServer::lpszPipeName = TEXT("\\\\.\\pipe\\RileyNamedPipe");

AsyncPipeServer::AsyncPipeServer()
    : m_PipeHandle(INVALID_HANDLE_VALUE), m_pWriteOverlappedArg(nullptr),
    m_pConnectOverlappedArg(nullptr)
{
}

AsyncPipeServer::~AsyncPipeServer()
{
    if (m_PipeHandle != INVALID_HANDLE_VALUE)
        CloseHandle(m_PipeHandle);
    m_PipeHandle = INVALID_HANDLE_VALUE;

    if (m_pWriteOverlappedArg) {
        HeapFree(GetProcessHeap(), 0, m_pWriteOverlappedArg);
        m_pWriteOverlappedArg = nullptr;
    }

    if (m_pConnectOverlappedArg) {
        HeapFree(GetProcessHeap(), 0, m_pConnectOverlappedArg);
        m_pConnectOverlappedArg = nullptr;
    }
}

bool AsyncPipeServer::InitConnectedPipe(_In_ DWORD dwAdvisoryBufSize)
{
    m_PipeHandle = CreateNamedPipe(
        lpszPipeName,             // pipe name 
        PIPE_ACCESS_OUTBOUND |    // server's write access
        FILE_FLAG_OVERLAPPED,     // Async Listening
        PIPE_TYPE_MESSAGE |       // message type pipe 
        PIPE_READMODE_MESSAGE |   // message-read mode 
        PIPE_WAIT,                // blocking mode 
        1,                        // only 1 instances  
        dwAdvisoryBufSize,        // output buffer size 
        dwAdvisoryBufSize,        // input buffer size 
        0,                        // client time-out 
        NULL);                    // default security attribute

    if (m_PipeHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    m_pWriteOverlappedArg = (LPOVERLAPPED)HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(OVERLAPPED));

    if (m_pWriteOverlappedArg == NULL) {
        goto Clean0;
    }

    m_pConnectOverlappedArg = (LPOVERLAPPED)HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(OVERLAPPED));

    if (m_pConnectOverlappedArg == NULL) {
        goto Clean0;
    }

    return EnablePipeListening() == S_OK;

Clean0:
    CloseHandle(m_PipeHandle);

    return false;
}

DWORD AsyncPipeServer::WriteBytes(LPCVOID pBase, DWORD dwLength)
{
    BOOL fWrite = FALSE;
    DWORD dwByteTransferred;
    DWORD dwErr;

    if (GetOverlappedResult(
        m_PipeHandle,
        (LPOVERLAPPED)m_pWriteOverlappedArg,
        &dwByteTransferred,
        TRUE) == 0)
        return GetLastError();

    // TODO: The WriteFileEx function may fail if there are too many
    // outstanding asynchronous I/O requests. In the event of such a failure, 
    // GetLastError can return ERROR_INVALID_USER_BUFFER or ERROR_NOT_ENOUGH_MEMORY.


    //m_WriteLock.Lock();

    fWrite = WriteFileEx(
        m_PipeHandle,
        pBase,
        dwLength,
        m_pWriteOverlappedArg,
        WriteCompleted);

    //m_WriteLock.Unlock();

    if (fWrite != 0) {
        PrintfDebugString("[Async Pipe] Frame is written succeessfully.\n");
        return S_OK;
    }
    else {
        dwErr = GetLastError();

        if (dwErr == ERROR_NO_DATA) {
            PrintfDebugString("[Async Pipe] Reinitlizing the pipe server.\n");
            DisconnectNamedPipe(m_PipeHandle);
            ConnectNamedPipe(m_PipeHandle, m_pConnectOverlappedArg);
            return S_OK;
        }
        else if (dwErr == ERROR_PIPE_LISTENING) {
            PrintfDebugString("[Async Pipe] Wait for pipe client connecting...\n");
            return S_OK;
        }
        else {
            PrintfDebugString("[Async Pipe] Unexpected Error: 0x%x\n", dwErr);
            return dwErr;
        }
    }
}

DWORD AsyncPipeServer::EnablePipeListening()
{
    BOOL fConnected;
    DWORD dwErr;

    fConnected = ConnectNamedPipe(m_PipeHandle, m_pConnectOverlappedArg);

    if (fConnected)
        return S_OK;
    else {
        dwErr = GetLastError();
        return (dwErr == ERROR_IO_PENDING || dwErr == ERROR_PIPE_CONNECTED)
            ? S_OK : dwErr;
    }
}

void CALLBACK AsyncPipeServer::WriteCompleted(
    _In_    DWORD dwErrorCode,
    _In_    DWORD dwNumberOfBytesTransfered,
    _Inout_ LPOVERLAPPED lpOverlapped)
{
    UNREFERENCED_PARAMETER(dwErrorCode);
    UNREFERENCED_PARAMETER(dwNumberOfBytesTransfered);
    UNREFERENCED_PARAMETER(lpOverlapped);

    // This function will never be called since we acquire the async IO
    // result by polling.
}
