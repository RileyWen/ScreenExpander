/*++

Module Name:

    queue.h

Abstract:

    This file contains the queue definitions.

Environment:

    User-mode Driver Framework 2

--*/
#pragma once

#include "pch.h"

EVT_IDD_CX_DEVICE_IO_CONTROL Evt_IddIoDeviceControl;

//EXTERN_C_START
//
////
//// This is the context that can be placed per queue
//// and would contain per queue information.
////
//typedef struct _QUEUE_CONTEXT {
//
//    ULONG PrivateDeviceData;  // just a placeholder
//
//} QUEUE_CONTEXT, *PQUEUE_CONTEXT;
//
//WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)
//
//NTSTATUS
//IndirectDisplayQueueInitialize(
//    _In_ WDFDEVICE Device
//    );
//
////
//// Events from the IoQueue object
////
//EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL IndirectDisplayEvtIoDeviceControl;
//EVT_WDF_IO_QUEUE_IO_STOP IndirectDisplayEvtIoStop;
//
//EXTERN_C_END


