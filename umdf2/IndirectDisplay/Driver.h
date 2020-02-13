/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    User-mode Driver Framework 2

--*/
#pragma once

#include "pch.h"

#include "Device.h"
#include "IoControl.h"

// GUID of the interface that allow user application
// to notify the indirect display driver that a monitor
// arrives or departs. 
//
// {6b06164c-8a07-4820-9139-19758f29e43e}
//
DEFINE_GUID (GUID_DEVINTERFACE_IndirectDisplay,
    0x6b06164c,0x8a07,0x4820,0x91,0x39,0x19,0x75,0x8f,0x29,0xe4,0x3e);


EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;

EXTERN_C_END

//
////
//// WDFDRIVER Events
////
//EVT_WDF_DRIVER_DEVICE_ADD IndirectDisplayEvtDeviceAdd;
//EVT_WDF_OBJECT_CONTEXT_CLEANUP IndirectDisplayEvtDriverContextCleanup;
//
