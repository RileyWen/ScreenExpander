#include <iostream>
#include <string>
#include <thread>

#include "InterProcessQueue.h"
#include "SharedDefs.h"

struct ElemType {
    ElemType() = default;

    ElemType(int a, int b) : a{ a }, b{ b } {}

    int a{};
    int b{};
};

void InterProcessQueueProducer() {
    InterProcessQueue<IMAGE_FRAME> queue{ INTERPROCESS_FILE_MAPPING_NAME };
    auto pFrame = std::make_unique<IMAGE_FRAME>();

    try {
        queue.Create();
        DWORD dwWidth, dwHeight;

        while (true) {
            std::cin >> dwWidth >> dwHeight;

            pFrame->dwWidth = dwWidth;
            pFrame->dwHeight = dwHeight;

            if (!queue.TryPushBack(*pFrame))
                std::cout << "Emplace Failed!\n";

            if (dwWidth == 0 && dwHeight == 0)
                break;
        }
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        system("pause");
    }


}

void InterProcessQueueConsumer() {
    InterProcessQueue<IMAGE_FRAME> queue{ INTERPROCESS_FILE_MAPPING_NAME };
    auto pFrame = std::make_unique<IMAGE_FRAME>();

    try {
        queue.OpenExisting();

        while (true) {
            DWORD dwResult;

            try {
                dwResult = WaitForSingleObject(queue.GetQueueSemaphoreHandle(), 1000 * 5);

                switch (dwResult)
                {
                case WAIT_OBJECT_0:
                    queue.TryPopFront(pFrame.get());
                    if (pFrame->dwHeight == 0 && pFrame->dwWidth == 0)
                        break;

                    std::cout << pFrame->dwWidth << " " << pFrame->dwHeight << std::endl;

                    system("pause");
                    break;

                case WAIT_TIMEOUT:
                    throw WaitTimeoutException();
                    break;
                case WAIT_FAILED:
                    [[fallthrough]];
                default:
                    throw WinApiException("[Semaphore] WaitOne");
                    break;
                }
            }
            catch (std::exception& e) {
                std::cout << e.what() << std::endl;
                system("pause");
            }
        }
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        system("pause");
    }

}
