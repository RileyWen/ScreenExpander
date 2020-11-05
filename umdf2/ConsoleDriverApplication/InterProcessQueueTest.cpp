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

void InterProcessQueueConsumer() {
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
