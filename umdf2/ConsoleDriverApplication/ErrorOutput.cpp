#include "ErrorOutput.h"

namespace std
{
#ifdef UNICODE
    wostream& _tcout = wcout;
    wistream& _tcin = wcin;
#else
    ostream& _tcout = cout;
    isteram& _tcin = cin;
    using _tstring = string;
#endif // UNICODE
}

void PrintCSBackupAPIErrorMessage(DWORD dwErr)
{
    WCHAR   wszMsgBuff[512];  // Buffer for text.

    DWORD   dwChars;  // Number of chars returned.

    // Try to get the message from the system errors.
    dwChars = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwErr,
        0,
        wszMsgBuff,
        512,
        NULL);

    if (0 == dwChars)
    {
        // The error code did not exist in the system errors.
        // Try Ntdsbmsg.dll for the error code.

        HINSTANCE hInst;

        // Load the library.
        hInst = LoadLibrary(TEXT("Ntdsbmsg.dll"));
        if (NULL == hInst)
        {
            _tprintf(TEXT("cannot load Ntdsbmsg.dll\n"));
            exit(1);  // Could 'return' instead of 'exit'.
        }

        // Try getting message text from ntdsbmsg.
        dwChars = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            hInst,
            dwErr,
            0,
            wszMsgBuff,
            512,
            NULL);

        // Free the library.
        FreeLibrary(hInst);

    }

    // Display the error message, or generic text if not found.
    _tprintf(TEXT("Error value: %d Message: %ws\n"),
        dwErr,
        dwChars ? wszMsgBuff : TEXT("Error message not found.\n"));

}

void GetStrLastError() {
    PrintCSBackupAPIErrorMessage(GetLastError());
}

