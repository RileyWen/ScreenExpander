#include "pch.h"
#include "IoControl.h"
#include "Driver.h"

_Use_decl_annotations_
VOID Evt_IddIoDeviceControl(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
) {
    //UNREFERENCED_PARAMETER(Device);
    //UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    //UNREFERENCED_PARAMETER(IoControlCode);

    NTSTATUS status;

    auto* pContext = WdfObjectGet_AdapterWdfContext(Device);
    auto* pIndirectAdapter = pContext->pIndirectAdapter;

    // Since adapter is null, we cannot attach new monitors to it.
    if (pIndirectAdapter->IsAdapterNull()) {
        PrintfDebugString("pIndirectAdapter->IddCxAdapterObject is null! Ignoring this I/O request.\n");
        WdfRequestComplete(Request, STATUS_IO_DEVICE_ERROR);
        return;
    }

    switch (IoControlCode)
    {
    case IOCTL_MONITOR_ARRIVE: {
        PVOID pUserInputBuf;
        size_t cbInputBuf;

        PVOID pUserOutputBuf;
        size_t cbOutputBuf;

        bool bMonitorCreateSuccess;

        MONITOR_ARRIVE_ARG_IN MonitorArriveArgIn;
        MONITOR_ARRIVE_ARG_OUT MonitorArriveArgOut;

        PrintfDebugString("[IOCTL_MONITOR_ARRIVE] Trying to add a new monitor...\n");

        pUserInputBuf = NULL;
        pUserOutputBuf = NULL;
        cbInputBuf = 0;
        cbOutputBuf = 0;

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(MONITOR_ARRIVE_ARG_IN),
            (PVOID*)&pUserInputBuf, &cbInputBuf);
        if (!NT_SUCCESS(status)) {
            PrintfDebugString("[IOCTL_MONITOR_ARRIVE] WdfRequestRetrieveInputBuffer Failed!\n");
            WdfRequestComplete(Request, status);
            return;
        }

        _Analysis_assume_(cbInputBuf >= sizeof(MONITOR_ARRIVE_ARG_IN));
        CopyMemory(&MonitorArriveArgIn, pUserInputBuf, sizeof(MONITOR_ARRIVE_ARG_IN));

        bMonitorCreateSuccess = pIndirectAdapter->NewMonitorArrives(MonitorArriveArgIn, &MonitorArriveArgOut);
        if (!bMonitorCreateSuccess) {
            PrintfDebugString("[IOCTL_MONITOR_ARRIVE] pIndirectAdapter->NewMonitorArrives Failed!\n");
            WdfRequestComplete(Request, STATUS_IO_DEVICE_ERROR);
            return;
        }
        
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(MONITOR_ARRIVE_ARG_OUT),
            (PVOID*)&pUserOutputBuf, &cbOutputBuf);
        if (!NT_SUCCESS(status)) {
            PrintfDebugString("[IOCTL_MONITOR_ARRIVE] WdfRequestRetrieveOutputBuffer Failed!\n");
            WdfRequestComplete(Request, status);
            return;
        }

        _Analysis_assume_(cbOutputBuf >= sizeof(MONITOR_ARRIVE_ARG_OUT));
        CopyMemory(pUserOutputBuf, &MonitorArriveArgOut, sizeof(MONITOR_ARRIVE_ARG_OUT));

        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, sizeof(MONITOR_ARRIVE_ARG_OUT));
        break;
    }

    case IOCTL_MONITOR_DEPART: {
        PVOID pUserInputBuf;
        size_t cbInputBuf;

        bool bMonitorDepartSuccess;

        MONITOR_DEPART_ARG_IN MonitorDepartArgIn;

        PrintfDebugString("[IOCTL_MONITOR_DEPART] Trying to add a new monitor...\n");

        pUserInputBuf = NULL;
        cbInputBuf = 0;

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(MONITOR_DEPART_ARG_IN),
            (PVOID*)&pUserInputBuf, &cbInputBuf);
        if (!NT_SUCCESS(status)) {
            PrintfDebugString("[IOCTL_MONITOR_DEPART] WdfRequestRetrieveInputBuffer Failed!\n");
            WdfRequestComplete(Request, status);
            return;
        }

        _Analysis_assume_(cbInputBuf >= sizeof(MONITOR_DEPART_ARG_IN));
        CopyMemory(&MonitorDepartArgIn, pUserInputBuf, sizeof(MONITOR_DEPART_ARG_IN));

        bMonitorDepartSuccess = pIndirectAdapter->MonitorDepart(MonitorDepartArgIn);
        if (!bMonitorDepartSuccess) {
            PrintfDebugString("[IOCTL_MONITOR_DEPART] pIndirectAdapter->MonitorDepart Failed!\n");
            WdfRequestComplete(Request, STATUS_IO_DEVICE_ERROR);
            return;
        }

        WdfRequestComplete(Request, STATUS_SUCCESS);
        break;
    }

    case IOCTL_ADAPTER_ECHO: {
        LPWSTR pwUserInputBuf;
        size_t inputLength;

        LPWSTR pwUserOutputBuf;
        size_t outputLength;

        pwUserInputBuf = NULL;
        pwUserOutputBuf = NULL;
        inputLength = 0;
        outputLength = 0;

        status = WdfRequestRetrieveInputBuffer(Request, 0, (PVOID*)&pwUserInputBuf, &inputLength);
        if (!NT_SUCCESS(status)) {
            PrintfDebugString("InWdfRequestRetrieveInputBuffer Failed!\n");
            WdfRequestComplete(Request, status);
        }

        status = WdfRequestRetrieveOutputBuffer(Request, 256, (PVOID*)&pwUserOutputBuf, &outputLength);
        if (!NT_SUCCESS(status)) {
            PrintfDebugString("WdfRequestRetrieveOutputBuffer Failed!\n");
            WdfRequestComplete(Request, status);
        }

        PrintfDebugString("IO Request: OutputBuf = 0x%p, OutputBufLength = %zu\n", pwUserOutputBuf, outputLength);

        StringCbPrintf(pwUserOutputBuf, outputLength, TEXT("Response from UMDF Driver!"));

        PrintfDebugString("Now pUserOutputBuf: %ws\n", pwUserOutputBuf);


        // Must use WdfRequestCompleteWithInformation to set the 'ByteReturn' field!!
        // If you just use WdfRequestComplete, OS thinks you're returning 0 bytes
        // and nothing will be returned to the buffer sent by DeviceIoControl!
        // Why the hell MSDN Documentation doesn't mention that?
        WdfRequestCompleteWithInformation(Request, status, sizeof(TEXT("Response from UMDF Driver!")));

        break;
    }

    default:
        WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
        break;
    }
}

//NTSTATUS
//IndirectDisplayQueueInitialize(
//	_In_ WDFDEVICE Device
//)
///*++
//
//Routine Description:
//
//	 The I/O dispatch callbacks for the frameworks device object
//	 are configured in this function.
//
//	 A single default I/O Queue is configured for parallel request
//	 processing, and a driver context memory allocation is created
//	 to hold our structure QUEUE_CONTEXT.
//
//Arguments:
//
//	Device - Handle to a framework device object.
//
//Return Value:
//
//	VOID
//
//--*/
//{
//	WDFQUEUE queue;
//	NTSTATUS status;
//	WDF_IO_QUEUE_CONFIG queueConfig;
//
//	//
//	// Configure a default queue so that requests that are not
//	// configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
//	// other queues get dispatched here.
//	//
//	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
//		&queueConfig,
//		WdfIoQueueDispatchParallel
//	);
//
//	queueConfig.EvtIoDeviceControl = IndirectDisplayEvtIoDeviceControl;
//	queueConfig.EvtIoStop = IndirectDisplayEvtIoStop;
//
//	status = WdfIoQueueCreate(
//		Device,
//		&queueConfig,
//		WDF_NO_OBJECT_ATTRIBUTES,
//		&queue
//	);
//
//	if (!NT_SUCCESS(status)) {
//		return status;
//	}
//
//	return status;
//}

//VOID
//IndirectDisplayEvtIoDeviceControl(
//	_In_ WDFQUEUE Queue,
//	_In_ WDFREQUEST Request,
//	_In_ size_t OutputBufferLength,
//	_In_ size_t InputBufferLength,
//	_In_ ULONG IoControlCode
//)
///*++
//
//Routine Description:
//
//	This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.
//
//Arguments:
//
//	Queue -  Handle to the framework queue object that is associated with the
//			 I/O request.
//
//	Request - Handle to a framework request object.
//
//	OutputBufferLength - Size of the output buffer in bytes
//
//	InputBufferLength - Size of the input buffer in bytes
//
//	IoControlCode - I/O control code.
//
//Return Value:
//
//	VOID
//
//--*/
//{
//	UNREFERENCED_PARAMETER(Queue);
//	UNREFERENCED_PARAMETER(Request);
//	UNREFERENCED_PARAMETER(OutputBufferLength);
//	UNREFERENCED_PARAMETER(InputBufferLength);
//	UNREFERENCED_PARAMETER(IoControlCode);
//
//	WdfRequestComplete(Request, STATUS_SUCCESS);
//
//	return;
//}
//
//VOID
//IndirectDisplayEvtIoStop(
//	_In_ WDFQUEUE Queue,
//	_In_ WDFREQUEST Request,
//	_In_ ULONG ActionFlags
//)
///*++
//
//Routine Description:
//
//	This event is invoked for a power-managed queue before the device leaves the working state (D0).
//
//Arguments:
//
//	Queue -  Handle to the framework queue object that is associated with the
//			 I/O request.
//
//	Request - Handle to a framework request object.
//
//	ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
//				  that identify the reason that the callback function is being called
//				  and whether the request is cancelable.
//
//Return Value:
//
//	VOID
//
//--*/
//{
//	UNREFERENCED_PARAMETER(Queue);
//	UNREFERENCED_PARAMETER(Request);
//	UNREFERENCED_PARAMETER(ActionFlags);
//
//	//
//	// In most cases, the EvtIoStop callback function completes, cancels, or postpones
//	// further processing of the I/O request.
//	//
//	// Typically, the driver uses the following rules:
//	//
//	// - If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
//	//   (if the request is cancelable) and either calls WdfRequestStopAcknowledge
//	//   with a Requeue value of TRUE, or it calls WdfRequestComplete with a
//	//   completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
//	//
//	//   Before it can call these methods safely, the driver must make sure that
//	//   its implementation of EvtIoStop has exclusive access to the request.
//	//
//	//   In order to do that, the driver must synchronize access to the request
//	//   to prevent other threads from manipulating the request concurrently.
//	//   The synchronization method you choose will depend on your driver's design.
//	//
//	//   For example, if the request is held in a shared context, the EvtIoStop callback
//	//   might acquire an internal driver lock, take the request from the shared context,
//	//   and then release the lock. At this point, the EvtIoStop callback owns the request
//	//   and can safely complete or requeue the request.
//	//
//	// - If the driver has forwarded the I/O request to an I/O target, it either calls
//	//   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
//	//   further processing of the request and calls WdfRequestStopAcknowledge with
//	//   a Requeue value of FALSE.
//	//
//	// A driver might choose to take no action in EvtIoStop for requests that are
//	// guaranteed to complete in a small amount of time.
//	//
//	// In this case, the framework waits until the specified request is complete
//	// before moving the device (or system) to a lower power state or removing the device.
//	// Potentially, this inaction can prevent a system from entering its hibernation state
//	// or another low system power state. In extreme cases, it can cause the system
//	// to crash with bugcheck code 9F.
//	//
//
//	return;
//}
