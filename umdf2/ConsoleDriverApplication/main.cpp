#include "PublicHeader.h"
#include "ErrorOutput.h"
#include "OpenUmdfInterfaceTest.h"
#include "NamedPipeTest.h"

using namespace std;

int main() {
    _tsetlocale(LC_ALL, TEXT("chs"));

    _tstring cmd;

    while (true) {
        _tcin >> cmd;

        if (cmd == T("o")) {
            if (OpenBusInterface())
                INFO("Call IOCTL Succeeded!\n");
            else
                INFO("Call IOCTL Failed!\n");
        }
        else if (cmd == T("p")) {
            PipeTest1_ConnectAfterCreated_RecvByPacket();
        }
        else if (cmd == T("q")) {
            break;
        }
    }

    return 0;
}