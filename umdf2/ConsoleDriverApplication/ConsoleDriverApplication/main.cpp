#include <iostream>
#include <stdio.h>
#include <string>

#include <Windows.h>
#include <SetupAPI.h>
#include <winioctl.h>

#include <initguid.h>
// GUID of the interface that allow user application to notify the indirect display driver that a
// monitor arrives or departs.
//
// {6b06164c-8a07-4820-9139-19758f29e43e}
DEFINE_GUID(GUID_DEVINTERFACE_IndirectDisplay,
	0x6b06164c, 0x8a07, 0x4820, 0x91, 0x39, 0x19, 0x75, 0x8f, 0x29, 0xe4, 0x3e);

// Also used as DeviceId
#define INDIRECT_DISPLAY_HARDWARE_IDS L"{6C2DAA24-08C9-4BDF-8791-68DA023EDA11}\\Indirect_Disp\0"

using namespace std;

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
		hInst = LoadLibrary(L"Ntdsbmsg.dll");
		if (NULL == hInst)
		{
			printf("cannot load Ntdsbmsg.dll\n");
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
	printf("Error value: %d Message: %ws\n",
		dwErr,
		dwChars ? wszMsgBuff : L"Error message not found.");

}

bool OpenBusInterface() {
	HDEVINFO							hardwareDevInfo;
	SP_DEVICE_INTERFACE_DATA			deviceInterfaceData;
	
	PSP_DEVICE_INTERFACE_DETAIL_DATA	pDeviceInterfaceDetailData = NULL;
	ULONG								predictedLen = 0;
	ULONG								requiredLen = 0;

	HANDLE								hDeviceInterface = INVALID_HANDLE_VALUE;

	hardwareDevInfo = SetupDiGetClassDevs(
		(LPGUID)&GUID_DEVINTERFACE_IndirectDisplay,
		NULL,
		NULL,
		(DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)); // Only Devices present & Function class devices

	if (hardwareDevInfo == INVALID_HANDLE_VALUE) {
		PrintCSBackupAPIErrorMessage(GetLastError());
		return false;
	}

	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	if (!SetupDiEnumDeviceInterfaces(hardwareDevInfo,
		0, // Can be NULL since the device is uniquely identified by GUID
		(LPGUID)&GUID_DEVINTERFACE_IndirectDisplay,
		0,
		&deviceInterfaceData)) {
		PrintCSBackupAPIErrorMessage(GetLastError());
		goto CleanUp;
	}

	std::cout << "Enumerate Interface Data Succeed!\n";

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
		goto CleanUp;
	}
	predictedLen = requiredLen;

	std::cout << "Probe Interface Data Buffer Length Succeed!\n";

	// Allocate a buffer after getting the required size.
	pDeviceInterfaceDetailData =
		(PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, predictedLen);
	if (pDeviceInterfaceDetailData)
		pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	else {
		PrintCSBackupAPIErrorMessage(GetLastError());
		goto CleanUp;
	}

	std::cout << "Allocate Interface Data Buffer Length Succeed!\n";

	// Get the actual data for the Indirect Display interface
	if (!SetupDiGetDeviceInterfaceDetail(
		hardwareDevInfo,
		&deviceInterfaceData,
		pDeviceInterfaceDetailData,
		predictedLen,
		&requiredLen,
		NULL)) {
		PrintCSBackupAPIErrorMessage(GetLastError());
		goto CleanUp;
	}
	std::wcout << L"Query Interface Data Succeed! Path: "
		<< pDeviceInterfaceDetailData->DevicePath
		<< std::endl;

	hDeviceInterface = CreateFile(
		pDeviceInterfaceDetailData->DevicePath,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (hDeviceInterface == INVALID_HANDLE_VALUE) {
		PrintCSBackupAPIErrorMessage(GetLastError());
		goto CleanUp;
	}

	std::cout << "Create Handle to Interface Data Succeed!\n";

#define INDIRECT_DISP_IOCTL(_index_) \
	CTL_CODE (FILE_DEVICE_BUS_EXTENDER, _index_, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SND_MSG INDIRECT_DISP_IOCTL(0x0)

	WCHAR msg[] = L"Hello from User Mode App!\n";
	DWORD bytesReturned = 0;

	if (DeviceIoControl(
		hDeviceInterface,
		IOCTL_SND_MSG,
		msg, sizeof(msg),
		NULL, 0,
		&bytesReturned, NULL)) {
		return true;
	}
	else {
		PrintCSBackupAPIErrorMessage(GetLastError());
		return false;
	}
	

CleanUp:
	SetupDiDestroyDeviceInfoList(hardwareDevInfo);
	return false;
}

int main() {
	string cmd;

	while (true) {
		cin >> cmd;

		if (cmd == "o") {
			if (OpenBusInterface())
				std::cout << "Call IOCTL Succeeded!\n";
			else
				std::cout << "Call IOCTL Failed!\n";
		}
		else if (cmd == "c") {
		}
		else if (cmd == "q") {
			break;
		}
	}

	return 0;
}