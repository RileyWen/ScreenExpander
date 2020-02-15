#pragma once

#include <iostream>
#include <string>

#include <mutex>
#include <condition_variable>

#include <thread>
#include <chrono>

#include <stdio.h>
#include <tchar.h>

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
#define INDIRECT_DISPLAY_HARDWARE_IDS TEXT("{6C2DAA24-08C9-4BDF-8791-68DA023EDA11}\\Indirect_Disp\0")
