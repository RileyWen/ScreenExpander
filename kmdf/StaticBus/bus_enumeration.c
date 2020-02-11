#include "pch.h"
#include "bus_def.h"

// Make all code pageable
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, Bus_EvtDeviceAdd)
#endif

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT  DriverObject,
	IN PUNICODE_STRING RegistryPath
) {
	WDF_DRIVER_CONFIG   config;
	NTSTATUS            status;
	WDFDRIVER driver;

	KdPrint((KDPFX "Enter Func DriverEntry\n"));

	WDF_DRIVER_CONFIG_INIT(
		&config,
		Bus_EvtDeviceAdd
	);

	// Create a framework driver object to represent the PDO of
	// this bus device.
	//
	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		WDF_NO_OBJECT_ATTRIBUTES,
		&config,
		&driver);

	if (!NT_SUCCESS(status)) {
		KdPrint((KDPFX "WdfDriverCreate failed with status 0x%x\n", status));
	}

	return status;
}

NTSTATUS
Bus_EvtDeviceAdd(
	IN WDFDRIVER        Driver,
	IN PWDFDEVICE_INIT  DeviceInit
) {
	NTSTATUS                   status;
	WDFDEVICE                  device;

	UNREFERENCED_PARAMETER(Driver);
	PAGED_CODE();

	KdPrint((KDPFX "Enter Func Bus_EvtDeviceAdd with Driver Handle: 0x%p\n", Driver));

	//
	// Initialize all the properties specific to the device.
	// Framework has default values for the one that are not
	// set explicitly here. So please read the doc and make sure
	// you are okay with the defaults.
	//
	WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);
	WdfDeviceInitSetExclusive(DeviceInit, TRUE);

	//
	// Create a framework device object. In response to this call, framework
	// creates a WDM deviceobject.
	//
	status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = Bus_StaticEnum(device);

	return status;
}

NTSTATUS
Bus_StaticEnum(
	_In_ WDFDEVICE Device
) {
	NTSTATUS		status = STATUS_FAILED_DRIVER_ENTRY;
	PWDFDEVICE_INIT pDeviceInit = NULL;
	WDFDEVICE hChild = NULL;

	DECLARE_CONST_UNICODE_STRING(deviceId, INDIRECT_DISPLAY_HARDWARE_IDS);
	DECLARE_CONST_UNICODE_STRING(compatId, INDIRECT_DISPLAY_COMPATIBLE_IDS);
	DECLARE_CONST_UNICODE_STRING(instanceID, L"Indirect_Disp_Adapter_0");

	KdPrint((KDPFX "Enter Func Bus_StaticEnum\n"));

	PAGED_CODE();

	// Initialize child PDO
	//
	pDeviceInit = WdfPdoInitAllocate(Device);
	if (pDeviceInit == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto CleanUp;
	}

	WdfDeviceInitSetDeviceType(pDeviceInit, FILE_DEVICE_BUS_EXTENDER);
	WdfDeviceInitSetExclusive(pDeviceInit, TRUE);

	status = WdfPdoInitAssignDeviceID(pDeviceInit, &deviceId);
	if (!NT_SUCCESS(status))
		goto CleanUp;

	status = WdfPdoInitAddHardwareID(pDeviceInit, &deviceId);
	if (!NT_SUCCESS(status))
		goto CleanUp;
	
	status = WdfPdoInitAddCompatibleID(pDeviceInit, &compatId);
	if (!NT_SUCCESS(status))
		goto CleanUp;

	status = WdfPdoInitAssignInstanceID(pDeviceInit, &instanceID);
	if (!NT_SUCCESS(status))
		goto CleanUp;

	status = WdfDeviceCreate(&pDeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &hChild);
	if (!NT_SUCCESS(status))
		goto CleanUp;

	status = WdfFdoAddStaticChild(Device, hChild);
	if (!NT_SUCCESS(status)) {
		goto CleanUp;
	}

	return status;

CleanUp:
	KdPrint((KDPFX "Bus_StaticEnum failed with %x\n", status));

	if (pDeviceInit)
		WdfDeviceInitFree(pDeviceInit);

	if (hChild)
		WdfObjectDelete(hChild);

	return status;
}

