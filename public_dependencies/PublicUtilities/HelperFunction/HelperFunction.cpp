// HelperFunction.cpp : Defines the functions for the static library.
//

#include "pch.h"

#include <winsock.h>

#pragma (lib, "Ws2_32.lib")

std::string LastErrorCodeToStringA(DWORD dwErrorCode) {
    if (dwErrorCode == 0) {
        // No error message has been recorded
        return {};
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dwErrorCode, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

// Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsStringA()
{
    return LastErrorCodeToStringA(GetLastError());
}

std::string WSAGetLastErrorAsStringA()
{
    return LastErrorCodeToStringA(WSAGetLastError());
}
