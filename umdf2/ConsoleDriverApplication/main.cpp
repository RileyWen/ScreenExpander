#include "PublicHeader.h"
#include "ErrorOutput.h"
#include "OpenUmdfInterfaceTest.h"
#include "NamedPipeTest.h"
#include "InterProcessQueueTest.h"

using namespace std;

int main() {
    _tsetlocale(LC_ALL, TEXT("chs"));

    _tstring cmd;

    while (true) {
        _tcin >> cmd;

        if (cmd == T("e")) {
            if (AdapterEchoTest())
                INFO("Call IOCTL Succeeded!\n");
            else
                INFO("Call IOCTL Failed!\n");
        }

        
        else if (cmd == T("n")) {
            if (NewMonitorTest())
                INFO("NewMonitorTest Succeeded!\n");
            else
                INFO("NewMonitorTest Failed!\n");
        }

        // AsyncPipeServer
        else if (cmd == T("p1")) {
            PipeTest1_ConnectAfterCreated_RecvByPacket();
        }
        else if (cmd == T("p2"))
            PipeTest2_OptimisiticLock();
        else if (cmd == T("p3"))
            PipeTest3_AsyncPipeServer();
        else if (cmd == T("p4"))
            PipeTest4_ReceiveImages();
        else if (cmd == T("p5"))
            PipeTest5_OnlyServer();

        // InterProcessQueue
        else if (cmd == T("P")) {
            InterProcessQueueProducer();
        }
        else if (cmd == T("C")) {
            InterProcessQueueConsumer();
        }
        else if (cmd == T("q")) {
            break;
        }
    }

    return 0;
}