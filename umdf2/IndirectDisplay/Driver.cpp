/*++

Module Name:

	driver.c

Abstract:

	This file contains the driver entry points and callbacks.

Environment:

	User-mode Driver Framework 2

--*/

#include "pch.h"
#include "Driver.h"
#include "IoControl.h"
#include "IddCxCallbacks.h"

using namespace indirect_disp;

_Use_decl_annotations_
extern "C" NTSTATUS DriverEntry(
	PDRIVER_OBJECT  pDriverObject,
	PUNICODE_STRING pRegistryPath
)
{
	WDF_DRIVER_CONFIG Config;
	NTSTATUS Status;

	OutputDebugString(L"[IndirDisp] DriverEntry \n");

	WDF_OBJECT_ATTRIBUTES Attributes;
	WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);

	WDF_DRIVER_CONFIG_INIT(&Config,
		Evt_IddDeviceAdd
	);

	Status = WdfDriverCreate(pDriverObject, pRegistryPath, &Attributes, &Config, WDF_NO_HANDLE);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	return Status;
}

_Use_decl_annotations_
NTSTATUS Evt_IddDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT pDeviceInit)
{
	NTSTATUS Status = STATUS_SUCCESS;
	WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks;

	UNREFERENCED_PARAMETER(Driver);

	OutputDebugString(L"[IndirDisp] Evt_IddDeviceAdd\n");

	// Register for power callbacks - in this sample only power-on is needed
	//
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
	PnpPowerCallbacks.EvtDeviceD0Entry = Evt_IddDeviceD0Entry;
	WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &PnpPowerCallbacks);

	IDD_CX_CLIENT_CONFIG IddConfig;
	IDD_CX_CLIENT_CONFIG_INIT(&IddConfig);

	// If the driver wishes to handle custom IoDeviceControl requests, it's necessary to use this callback since IddCx
	// redirects IoDeviceControl requests to an internal queue. This sample does not need this.
	IddConfig.EvtIddCxDeviceIoControl = Evt_IddIoDeviceControl;

	IddConfig.EvtIddCxAdapterInitFinished = Evt_IddAdapterInitFinished;

	IddConfig.EvtIddCxParseMonitorDescription = Evt_IddParseMonitorDescription;
	IddConfig.EvtIddCxMonitorGetDefaultDescriptionModes = Evt_IddMonitorGetDefaultModes;
	IddConfig.EvtIddCxMonitorQueryTargetModes = Evt_IddMonitorQueryModes;
	IddConfig.EvtIddCxAdapterCommitModes = Evt_IddAdapterCommitModes;
	IddConfig.EvtIddCxMonitorAssignSwapChain = Evt_IddMonitorAssignSwapChain;
	IddConfig.EvtIddCxMonitorUnassignSwapChain = Evt_IddMonitorUnassignSwapChain;

	Status = IddCxDeviceInitConfig(pDeviceInit, &IddConfig);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	WDF_OBJECT_ATTRIBUTES Attr;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, IndirectAdapterContext);
    Attr.EvtCleanupCallback = [](WDFOBJECT Object)
    {
        // Automatically cleanup the context when the WDF object is about to be deleted
        auto* pContext = WdfObjectGet_IndirectAdapterContext(Object);
        if (pContext)
        {
            pContext->Cleanup();
        }
    };

	WDFDEVICE Device = nullptr;
	Status = WdfDeviceCreate(&pDeviceInit, &Attr, &Device);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	Status = WdfDeviceCreateDeviceInterface(
		Device,
		(LPGUID)&GUID_DEVINTERFACE_INDIRECT_DEVICE,
		NULL // ReferenceString
	);

	if (!NT_SUCCESS(Status)) {
		OutputDebugString(L"WdfDeviceCreateDeviceInterface failed.\n");
		return Status;
	}

	Status = IddCxDeviceInitialize(Device);

	OutputDebugString(L"[IndirDisp] Exit Evt_IddDeviceAdd.\n");

	return Status;
}

