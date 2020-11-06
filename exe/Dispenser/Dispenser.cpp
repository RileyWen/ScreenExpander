#include "pch.h"

#include <iostream>
#include <string>
#include <thread>
#include <memory>

#include "InterProcessQueue.h"
#include "SharedDefs.h"
#include "HelperFunction.h"


#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#define DEFAULT_LISTEN_ADDR "0.0.0.0"
#define DEFAULT_PORT "23232"

HANDLE hSomeEvent;
HANDLE hSocketEvent;

SOCKET CreateListenSocket(PCSTR pNodeName, PCSTR pServiceName) {
    int iResult;
    struct addrinfo* result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(pNodeName, pServiceName, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        return INVALID_SOCKET;
    }

    SOCKET ListenSocket = INVALID_SOCKET;

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cout << "TempSocket failed with error: " << WSAGetLastErrorAsStringA() << std::endl;
        freeaddrinfo(result);
        return INVALID_SOCKET;
    }

    // Setup the TCP listening TempSocket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cout << "bind failed with error: " << WSAGetLastErrorAsStringA() << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        return INVALID_SOCKET;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cout << "listen failed with error: " << WSAGetLastErrorAsStringA() << std::endl;
        closesocket(ListenSocket);
        return INVALID_SOCKET;
    }

    return ListenSocket;
}

/*
Suppress arithmetic overflow check
*/
#pragma warning( push )
#pragma warning( disable : 26451 )

int RealMain() {
    int iResult;
    DWORD dwResult;
    bool bResult;

    SOCKET TempSocket = INVALID_SOCKET;
    HANDLE hTempEvent = INVALID_HANDLE_VALUE;

    struct sockaddr_storage client_addr;
    struct sockaddr_in* pSockAddrIn;
    int iAddrLen;

    WSANETWORKEVENTS NetworkEvents;

    std::vector<SOCKET> SocketList;
    std::vector<HANDLE> EventHandleList;

    DWORD dwClientCount = 0;
    DWORD dwClientToReadIndex;

    constexpr DWORD LISTEN_SOCK_INDEX = 0;
    constexpr DWORD CLIENT_SOCK_INDEX_0 = 1;

    constexpr DWORD QUEUE_EVENT_INDEX = 0;
    constexpr DWORD LISTEN_EVENT_INDEX = 1;
    constexpr DWORD CLIENT_EVENT_INDEX_0 = 2;

    auto BufPtr = std::make_unique<char>(1024 * 8);
    WSABUF WsaBuf{ 1024 * 8, BufPtr.get() };

    DWORD dwByteRecv;
    DWORD dwZero = 0;

    InterProcessQueue<IMAGE_FRAME> ImageFrameQueue{ INTERPROCESS_FILE_MAPPING_NAME };
    auto FrameBufPtr = std::make_unique<IMAGE_FRAME>();

    try {
        ImageFrameQueue.OpenExisting();
    }
    catch (WinApiException& e) {
        std::cout << "WinApiException: " << e.what() << std::endl;
        goto FailAndCleanUp;
    }
    catch (std::exception& e) {
        std::cout << "Unexpected exception occured: " << e.what() << std::endl;
        goto FailAndCleanUp;
    }
    EventHandleList.push_back(ImageFrameQueue.GetQueueSemaphoreHandle());

    TempSocket = CreateListenSocket(DEFAULT_LISTEN_ADDR, DEFAULT_PORT);
    if (TempSocket == INVALID_SOCKET) {
        goto FailAndCleanUp;
    }
    SocketList.push_back(TempSocket);

    hTempEvent = WSACreateEvent();
    if (hTempEvent == NULL) {
        std::cout << "WSACreateEvent <ListenEvent> failed with error: " << WSAGetLastErrorAsStringA() << std::endl;
        goto FailAndCleanUp;
    }
    EventHandleList.push_back(hTempEvent);

    iResult = WSAEventSelect(SocketList[LISTEN_SOCK_INDEX], EventHandleList[LISTEN_EVENT_INDEX], FD_ACCEPT | FD_CLOSE);
    if (iResult != 0) {
        std::cout << "WSAEventSelect <Listen> failed with error: " << WSAGetLastErrorAsStringA() << std::endl;
        goto FailAndCleanUp;
    }

    std::cout << "Start Listening...\n";

    while (true)
    {
        dwResult = WaitForMultipleObjects(EventHandleList.size(), EventHandleList.data(), FALSE, 10000);
        switch (dwResult)
        {
        case WAIT_OBJECT_0 + QUEUE_EVENT_INDEX:
            bResult = ImageFrameQueue.TryPopFront(FrameBufPtr.get());
            if (bResult) {
                printf("Receive a frame: Width: %lu, Height: %lu\n", FrameBufPtr->dwWidth, FrameBufPtr->dwHeight);
            }
            else {
                printf("Queue has been notified wrongly.\n");
            }

            break;

        case WAIT_OBJECT_0 + LISTEN_EVENT_INDEX:
            std::cout << "Listen Event Triggered!\n";

            iResult = WSAEnumNetworkEvents(SocketList[LISTEN_SOCK_INDEX], EventHandleList[LISTEN_EVENT_INDEX], &NetworkEvents);
            if (iResult != 0) {
                std::cout << "WSAEnumNetworkEvents <Listen> failed with error: " << LastErrorCodeToStringA(iResult) << std::endl;
                goto FailAndCleanUp;
            }

            if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
            {
                if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
                    printf("FD_ACCEPT failed with error %d\n", NetworkEvents.iErrorCode[FD_ACCEPT_BIT]);
                    goto FailAndCleanUp;
                }

                iAddrLen = sizeof(client_addr);
                TempSocket = accept(SocketList[LISTEN_SOCK_INDEX], (struct sockaddr*)&client_addr, &iAddrLen);
                if (TempSocket == INVALID_SOCKET) {
                    std::cout << "accept failed with error: " << WSAGetLastErrorAsStringA() << std::endl;
                    goto FailAndCleanUp;
                }
                SocketList.push_back(TempSocket);

                pSockAddrIn = reinterpret_cast<struct sockaddr_in*>(&client_addr);
                printf("Accpeted new client from: %d.%d.%d.%d : %hu.\n",
                    reinterpret_cast<uint8_t*>(&pSockAddrIn->sin_addr)[0],
                    reinterpret_cast<uint8_t*>(&pSockAddrIn->sin_addr)[1],
                    reinterpret_cast<uint8_t*>(&pSockAddrIn->sin_addr)[2],
                    reinterpret_cast<uint8_t*>(&pSockAddrIn->sin_addr)[3],
                    ntohs(pSockAddrIn->sin_port)
                );

                hTempEvent = WSACreateEvent();
                if (hTempEvent == NULL) {
                    std::cout << "WSACreateEvent <NewClient> failed with error: " << WSAGetLastErrorAsStringA() << std::endl;
                    goto FailAndCleanUp;
                }
                EventHandleList.push_back(hTempEvent);

                dwClientCount++;

                iResult = WSAEventSelect(SocketList[CLIENT_SOCK_INDEX_0 + dwClientCount - 1],
                    EventHandleList[CLIENT_EVENT_INDEX_0 + dwClientCount - 1], FD_READ | FD_CLOSE);
                if (iResult != 0) {
                    std::cout << "WSAEventSelect <NewClient> failed with error: " << WSAGetLastErrorAsStringA() << std::endl;
                    goto FailAndCleanUp;
                }
            }

            break;

        case WAIT_TIMEOUT:
            printf("Timeout\n");
            break;

        case WAIT_FAILED:
            std::cout << "Error occurred while waiting for events: "
                << GetLastErrorAsStringA() << std::endl;
            goto FailAndCleanUp;

        default:
            if (dwResult > WAIT_OBJECT_0 + CLIENT_EVENT_INDEX_0 + dwClientCount - 1) {
                std::cout << "Unexpected WAIT_OBJECT Index: " << dwResult << std::endl;
                goto FailAndCleanUp;
            }

            dwClientToReadIndex = (dwResult - WAIT_OBJECT_0) - CLIENT_EVENT_INDEX_0;

            iResult = WSAEnumNetworkEvents(SocketList[CLIENT_SOCK_INDEX_0 + dwClientToReadIndex],
                EventHandleList[CLIENT_EVENT_INDEX_0 + dwClientToReadIndex], &NetworkEvents);
            if (iResult != 0) {
                std::cout << "WSAEnumNetworkEvents <Client " << dwClientToReadIndex << "> failed with error: "
                    << LastErrorCodeToStringA(iResult) << std::endl;
                goto FailAndCleanUp;
            }

            // Handle READ Event
            if (NetworkEvents.lNetworkEvents & FD_READ)
            {
                if (NetworkEvents.iErrorCode[FD_READ_BIT] != 0) {
                    printf("FD_READ failed with error %d\n", NetworkEvents.iErrorCode[FD_READ_BIT]);
                    goto FailAndCleanUp;
                }

                iResult = WSARecv(
                    SocketList[CLIENT_SOCK_INDEX_0 + dwClientToReadIndex],
                    &WsaBuf, 1,     // WSABUF and # of WSABUF
                    &dwByteRecv,
                    &dwZero,        // Flag
                    NULL, NULL);

                if (iResult != 0) {
                    std::cout << "Client #" << dwClientToReadIndex << "'s WSARecv failed: "
                        << WSAGetLastErrorAsStringA() << " . Exiting...\n";
                    goto FailAndCleanUp;
                }

                printf("Client #%u received %u bytes: ", dwClientToReadIndex, dwByteRecv);
                std::cout << std::string_view(WsaBuf.buf, dwByteRecv) << std::endl;
            }

            // Handle Close Event
            else if (NetworkEvents.lNetworkEvents & FD_CLOSE) {
                if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
                    switch (NetworkEvents.iErrorCode[FD_CLOSE_BIT])
                    {
                    case WSAECONNRESET:
                        std::cout << "Client #" << dwClientToReadIndex << " encountered connection reset. Removing it...\n";
                        break;
                    case WSAECONNABORTED:
                        std::cout << "Client #" << dwClientToReadIndex << " encountered connection abort. Removing it...\n";
                        break;
                    case WSAENETDOWN:
                        std::cout << "Unexpected WSAENETDOWN. Exiting...\n";
                        [[fallthrough]];
                    default:
                        goto FailAndCleanUp;
                    }
                }
                else {
                    std::cout << "Client #" << dwClientToReadIndex << "'s connection close normally. Removing it...\n";
                }

                WSACloseEvent(EventHandleList[CLIENT_EVENT_INDEX_0 + dwClientToReadIndex]);
                closesocket(SocketList[CLIENT_SOCK_INDEX_0 + dwClientToReadIndex]);

                EventHandleList.erase(EventHandleList.begin() + CLIENT_EVENT_INDEX_0 + dwClientToReadIndex);
                SocketList.erase(SocketList.begin() + CLIENT_SOCK_INDEX_0 + dwClientToReadIndex);

                dwClientCount--;
            }

            break;
        }
    }

    return 0;

FailAndCleanUp:
    for (auto& event : EventHandleList)
        WSACloseEvent(event);

    for (auto& sock : SocketList)
        closesocket(sock);

    return 1;
}
#pragma warning( pop )

int main()
{
    int iResult;
    WSADATA wsaData;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    iResult = RealMain();

    WSACleanup();

    return iResult;
}
