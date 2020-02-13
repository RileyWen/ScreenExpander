/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    User-mode Driver Framework 2

--*/

#include "pch.h"

#include "Device.h"
#include "IoControl.h"

DEFINE_GUID (GUID_DEVINTERFACE_IndirectDisplay,
    0x6b06164c,0x8a07,0x4820,0x91,0x39,0x19,0x75,0x8f,0x29,0xe4,0x3e);
// {6b06164c-8a07-4820-9139-19758f29e43e}

//EXTERN_C_START
//
////
//// WDFDRIVER Events
////
//DRIVER_INITIALIZE DriverEntry;
//EVT_WDF_DRIVER_DEVICE_ADD IndirectDisplayEvtDeviceAdd;
//EVT_WDF_OBJECT_CONTEXT_CLEANUP IndirectDisplayEvtDriverContextCleanup;
//
//EXTERN_C_END
