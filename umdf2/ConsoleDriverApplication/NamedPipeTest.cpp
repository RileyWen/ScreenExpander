#include "NamedPipeTest.h"

TCHAR PIPE_NAME[] = TEXT("\\\\.\\pipe\\RileyTestPipe");

using namespace std;

void PipeTest3_AsyncPipeServer()
{
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

#define BUF_SIZE 128


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
