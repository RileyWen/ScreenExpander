#include "PublicHeader.h"
#include "ErrorOutput.h"
#include "OpenUmdfInterfaceTest.h"

HANDLE OpenDeviceInterface(DWORD dwDesiredAccess);

HANDLE OpenDeviceInterfaceRW() {
    return OpenDeviceInterface(GENERIC_READ | GENERIC_WRITE);
}

HANDLE OpenDeviceInterfaceRO() {
    return OpenDeviceInterface(GENERIC_READ);
}

bool AdapterEchoTest() {
    bool result = false;

    WCHAR pwInputBuffer[] = L"Hello from User Mode App!\n";

    WCHAR pwOutputBuffer[256];
    DWORD bytesReturned = 0;

    HANDLE hDeviceInterface;

    hDeviceInterface = OpenDeviceInterfaceRW();
    if (hDeviceInterface == INVALID_HANDLE_VALUE)
        return false;

    if (!DeviceIoControl(
        hDeviceInterface,
        IOCTL_ADAPTER_ECHO,
        pwInputBuffer, sizeof(pwInputBuffer),
        pwOutputBuffer, sizeof(pwOutputBuffer),
        &bytesReturned, NULL)) {
        PrintCSBackupAPIErrorMessage(GetLastError());
        result = false;
        goto CleanUp0;
    }

    std::_tcout << "Message returned from driver:" << pwOutputBuffer << std::endl;

    result = true;

CleanUp0:
    CloseHandle(hDeviceInterface);

    return result;
}

bool NewMonitorTest()
{
    bool result = false;
    HANDLE hDeviceInterface;

    MONITOR_ARRIVE_ARG_OUT MonitorArriveArgOut;
    DWORD bytesReturned = 0;

    hDeviceInterface = OpenDeviceInterfaceRO();
    if (hDeviceInterface == INVALID_HANDLE_VALUE)
        return false;

    if (!DeviceIoControl(
        hDeviceInterface,
        IOCTL_MONITOR_ARRIVE,
        NULL, 0,
        &MonitorArriveArgOut, sizeof(MONITOR_ARRIVE_ARG_OUT),
        &bytesReturned, NULL)) {
        PrintCSBackupAPIErrorMessage(GetLastError());
        result = false;
        goto CleanUp0;
    }

    result = true;

    std::_tcout << "New Monitor Index: " << MonitorArriveArgOut.dwMonitorIndex << std::endl;

CleanUp0:
    CloseHandle(hDeviceInterface);

    return result;
}

HANDLE OpenDeviceInterface(DWORD dwDesiredAccess) {
    HDEVINFO							hardwareDevInfo;
    SP_DEVICE_INTERFACE_DATA			deviceInterfaceData;

    PSP_DEVICE_INTERFACE_DETAIL_DATA	pDeviceInterfaceDetailData = NULL;
    ULONG								predictedLen = 0;
    ULONG								requiredLen = 0;

    HANDLE								hDeviceInterface = INVALID_HANDLE_VALUE;

    hardwareDevInfo = SetupDiGetClassDevs(
        (LPGUID)&GUID_DEVINTERFACE_INDIRECT_DEVICE,
        NULL,
        NULL,
        (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)); // Only Devices present & Function class devices

    if (hardwareDevInfo == INVALID_HANDLE_VALUE) {
        PrintCSBackupAPIErrorMessage(GetLastError());
        return hDeviceInterface;
    }

    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (!SetupDiEnumDeviceInterfaces(hardwareDevInfo,
        0, // Can be NULL since the device is uniquely identified by GUID
        (LPGUID)&GUID_DEVINTERFACE_INDIRECT_DEVICE,
        0,
        &deviceInterfaceData)) {
        PrintCSBackupAPIErrorMessage(GetLastError());
        goto CleanUp0;
    }

    INFO("Enumerate Interface Data Succeed!\n");

    // Get the required buffer size for our SP_DEVICE_INTERFACE_DATA in which stores the detailed
    // info about the Indirect Display interface
    SetupDiGetDeviceInterfaceDetail(
        hardwareDevInfo,
        &deviceInterfaceData,
        NULL,
        0,
        &requiredLen,
        NULL);
    if (ERROR_INSUFFICIENT_BUFFER != GetLastError()) {
        PrintCSBackupAPIErrorMessage(GetLastError());
        goto CleanUp0;
    }
    predictedLen = requiredLen;

    INFO("Probe Interface Data Buffer Length Succeed!\n");

    // Allocate a buffer after getting the required size.
    pDeviceInterfaceDetailData =
        (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, predictedLen);
    if (pDeviceInterfaceDetailData)
        pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    else {
        PrintCSBackupAPIErrorMessage(GetLastError());
        goto CleanUp0;
    }

    INFO("Allocate Interface Data Buffer Length Succeed!\n");

    // Get the actual data for the Indirect Display interface
    if (!SetupDiGetDeviceInterfaceDetail(
        hardwareDevInfo,
        &deviceInterfaceData,
        pDeviceInterfaceDetailData,
        predictedLen,
        &requiredLen,
        NULL)) {
        PrintCSBackupAPIErrorMessage(GetLastError());
        goto CleanUp1;
    }


    std::_tcout << TEXT("Query Interface Data Succeed! Path: ")
        << pDeviceInterfaceDetailData->DevicePath
        << std::endl;

    hDeviceInterface = CreateFile(
        pDeviceInterfaceDetailData->DevicePath,
        dwDesiredAccess, // Important!!! Don't forget it!
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (hDeviceInterface == INVALID_HANDLE_VALUE) {
        PrintCSBackupAPIErrorMessage(GetLastError());
        goto CleanUp1;
    }

    INFO("Create Handle to Interface Data Succeed!\n");

    return hDeviceInterface;

CleanUp1:
    HeapFree(GetProcessHeap(), 0, pDeviceInterfaceDetailData);

CleanUp0:
    SetupDiDestroyDeviceInfoList(hardwareDevInfo);
    return INVALID_HANDLE_VALUE;
}

