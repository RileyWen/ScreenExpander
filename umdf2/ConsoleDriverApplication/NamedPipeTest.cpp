#include "NamedPipeTest.h"
#include "AsyncPipeServer.h"

TCHAR PIPE_NAME[] = TEXT("\\\\.\\pipe\\RileyTestPipe");

using namespace std;

const DWORD BUF_SIZE = 1024;

HANDLE ClientConnectToPipe(LPCTSTR lpszPipeName) {
    HANDLE hPipe;
    DWORD dwMode;
    BOOL bResult;

    do {
        hPipe = CreateFile(
            lpszPipeName,
            GENERIC_READ | FILE_WRITE_ATTRIBUTES,	// It's required by SetNamedPipeHandleState
            0,										// No sharing
            NULL,									// Default security attributes
            OPEN_EXISTING,
            0,
            NULL);
        if (hPipe == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_PIPE_BUSY) {
                WaitNamedPipe(lpszPipeName, NMPWAIT_WAIT_FOREVER);
                continue;
            }
            else
                return INVALID_HANDLE_VALUE;
        }
        break;
    } while (1);

    dwMode = PIPE_READMODE_MESSAGE;
    bResult = SetNamedPipeHandleState(
        hPipe,
        &dwMode,
        NULL,
        NULL);
    if (!bResult) {
        CloseHandle(hPipe);
        return INVALID_HANDLE_VALUE;
    }

    return hPipe;
}

void PipeTest3_AsyncPipeServer()
{
    mutex ClientConnectedMux, ReconnectMux;
    condition_variable ClientConnectCv;

    auto pipe_server = [&]() {
        _tstring cmd;
        DWORD dwResult;
        AsyncPipeServer pipe;
        unique_lock<mutex> ReconnectLock(ReconnectMux);

        if (!pipe.InitConnectedPipe(BUF_SIZE)) {
            INFO("[Server] Failed to Create a pipe.\n");
            GetStrLastError();
            return;
        }

        INFO("[Server] The pipe is created successfully.\n");

        INFO("[Server] Start reading stdin and send to Client\n");

        while (1) {
            _tcin >> cmd;
            if (cmd == T("q"))
                break;
            else if (cmd == T("g"))
                ClientConnectCv.notify_one();	// Let client go
            else if (cmd == T("r")) {
                ReconnectLock.unlock();
            }

            DWORD bytesToWrite = static_cast<DWORD>(cmd.size() + 1) * sizeof(TCHAR);

            dwResult = pipe.WriteBytes(cmd.c_str(), bytesToWrite);

            if (dwResult) {
                INFO("[Server] Failed to write to pipe:");
                GetStrLastError();
                break;
            }

            if (cmd == T("r")) {
                this_thread::sleep_for(chrono::seconds(3));
                ReconnectLock.lock();
            }
        }

        INFO("[Server] Closing the pipe...\n");
    };

    auto pipe_client = [&]() {
        HANDLE hPipe = INVALID_HANDLE_VALUE;
        BOOL bResult;
        TCHAR Buf[BUF_SIZE] = {};
        DWORD byteRead;

        unique_lock<mutex> ConnectLock(ClientConnectedMux, defer_lock);
        unique_lock<mutex> ReconnectLock(ReconnectMux, defer_lock);

    Start_:
        ConnectLock.lock();

        hPipe = INVALID_HANDLE_VALUE;
        INFO("[Client] Start connecting the pipe.\n");


        INFO("[Client] Start receiving messages from Server.\n");

        //{
        //    INFO("[Client] Sleep some seconds...\n");
        //    // Sleep some seconds to test whether the data are
        //    // received by stream or by packet.
        //    this_thread::sleep_for(chrono::seconds(5));
        //}

        hPipe = ClientConnectToPipe(AsyncPipeServer::lpszPipeName);
        if (hPipe == INVALID_HANDLE_VALUE) {
            INFO("[Client] Failed to connect to the pipe: ");
            GetStrLastError();
        }
        int sum = 0;

        INFO("[Client] Connected.\n");

        ClientConnectCv.wait(ConnectLock);

        INFO("[Client] Wait server for unlocking.\n");

        do {
            bResult = ReadFile(
                hPipe,
                Buf,
                BUF_SIZE,
                &byteRead,
                NULL);

            if (!bResult &&
                GetLastError() != ERROR_MORE_DATA) { // Our buffer is not suffient for the whole data
                INFO("[Client] Read pipe error: ");
                GetStrLastError();
                break;
            }

            sum += byteRead;

            _tprintf(T("[Client] %d Receive from server: %s\n"), sum, Buf);

            if (ReconnectLock.try_lock()) {
                INFO("[Client] Reconnecting... \n");
                ReconnectLock.unlock();
                ConnectLock.unlock();
                CloseHandle(hPipe);
                hPipe = INVALID_HANDLE_VALUE;
                goto Start_;
            }
        } while (1);

        _tprintf(T("[Client] Closing...\n"));
        CloseHandle(hPipe);
    };

    thread t1(pipe_server), t2(pipe_client);
    t1.join();
    t2.join();
}

void PipeTest2_OptimisiticLock() {
    SpinLock lock, OutLock;
    OutLock.Lock();

    auto snd_thread = [&] {
        _tstring cmd;

        while (1) {
            _tcin >> cmd;

            if (cmd == T("l"))
                lock.Lock();
            else if (cmd == T("u"))
                lock.Unlock();
            else if (cmd == T("q"))
                break;
        }

        OutLock.Unlock();
    };

    auto recv_thread = [&] {
        while (!OutLock.TryLock()) {
            lock.Lock();
            _tprintf(T("."));
            lock.Unlock();

            fflush(stdout);
            this_thread::sleep_for(chrono::seconds(1));
        }
    };

    thread t1(snd_thread), t2(recv_thread);
    t1.join();
    t2.join();
}

void PipeTest1_ConnectAfterCreated_RecvByPacket() {
    mutex mux;
    condition_variable cv;

    auto pipe_server = [&]() {
        HANDLE hPipe;
        _tstring cmd;
        DWORD bytesWritten;
        BOOL bResult;

        hPipe = CreateNamedPipe(
            PIPE_NAME,
            PIPE_ACCESS_OUTBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
            1,			// Only 1 instance
            BUF_SIZE,
            BUF_SIZE,
            0,			// Default timeout
            NULL		// Default security descriptor
        );
        if (hPipe == INVALID_HANDLE_VALUE) {
            cv.notify_one();	// Let client go
            INFO("[Server] Failed to Create a pipe.\n");
            GetStrLastError();
            return;
        }

        INFO("[Server] The pipe is created successfully.\n");
        cv.notify_one();

        INFO("[Server] Start reading stdin and send to Client\n");

        while (1) {
            _tcin >> cmd;
            if (cmd == T("q"))
                break;

            DWORD bytesToWrite = static_cast<DWORD>(cmd.size() + sizeof(TCHAR));

            bResult = WriteFile(
                hPipe,
                cmd.c_str(),
                bytesToWrite,
                &bytesWritten,
                NULL);

            if (!bResult || bytesWritten != bytesToWrite) {
                INFO("[Server] Failed to write to pipe:");
                GetStrLastError();
                break;
            }
        }

        INFO("[Server] Closing the pipe...\n");

        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    };

    auto pipe_client = [&]() {
        HANDLE hPipe;
        DWORD dwMode;
        BOOL bResult;
        TCHAR Buf[BUF_SIZE] = {};
        DWORD byteRead;

        unique_lock<mutex> lck(mux);
        cv.wait(lck);

        INFO("[Client] Start connecting the pipe.\n");

        hPipe = CreateFile(
            PIPE_NAME,
            GENERIC_READ | FILE_WRITE_ATTRIBUTES,	// It's required by SetNamedPipeHandleState
            0,										// No sharing
            NULL,									// Default security attributes
            OPEN_EXISTING,
            0,
            NULL);
        if (hPipe == INVALID_HANDLE_VALUE) {
            INFO("[Client] Failed to connect to the pipe: ");
            GetStrLastError();
            return;
        }

        dwMode = PIPE_READMODE_MESSAGE;
        bResult = SetNamedPipeHandleState(
            hPipe,
            &dwMode,
            NULL,
            NULL);
        if (!bResult) {
            INFO("[Client] Failed to set pipe mode: ");
            GetStrLastError();
            CloseHandle(hPipe);
            return;
        }

        INFO("[Client] Start receiving messages from Server.\n");

        {
            INFO("[Client] Sleep some seconds...\n");
            // Sleep some seconds to test whether the data are
            // received by stream or by packet.
            this_thread::sleep_for(chrono::seconds(5));
        }

        do {
            bResult = ReadFile(
                hPipe,
                Buf,
                BUF_SIZE,
                &byteRead,
                NULL);
            if (!bResult &&
                GetLastError() != ERROR_MORE_DATA) { // Our buffer is not suffient for the whole data
                INFO("[Client] Read pipe error: ");
                GetStrLastError();
                break;
            }

            _tprintf(T("[Client] Receive from server: %s\n"), Buf);
        } while (bResult);

        CloseHandle(hPipe);
    };

    thread t1(pipe_server), t2(pipe_client);
    t1.join();
    t2.join();
}
