#include <iostream>
#include <string>
#include <thread>

#include "InterProcessQueue.h"
#include "SharedDefs.h"

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return std::string(); //No error message has been recorded

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

struct ElemType {
    ElemType() = default;

    ElemType(int a, int b) : a{ a }, b{ b } {}

    int a{};
    int b{};
};

void producer() {
    InterProcessQueue<ElemType> queue{ INTERPROCESS_FILE_MAPPING_NAME };

    try {
        queue.Create();
        int a, b;

        while (true) {
            std::cin >> a >> b;

            if (!queue.TryEmplaceBack(a, b))
                std::cout << "Emplace Failed!\n";

            if (a == 0 && b == 0)
                break;
        }
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        system("pause");
    }


}

void consumer() {
    InterProcessQueue<ElemType> queue{ INTERPROCESS_FILE_MAPPING_NAME };

    try {
        queue.OpenExisting();

        ElemType elem;

        while (true) {
            queue.PopFront(&elem);
            if (elem.a == 0 && elem.b == 0)
                break;

            std::cout << elem.a << " " << elem.b << std::endl;

            system("pause");

        }
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        system("pause");
    }

}

int main()
{
    std::string cmd;

    std::cin >> cmd;
    if (cmd == "P") {
        producer();
    }
    else {
        consumer();
    }

    return 0;
}
