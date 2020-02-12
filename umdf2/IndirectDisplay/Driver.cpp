/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    User-mode Driver Framework 2

--*/

#include "driver.h"
#include "IndirectDisp.h"

_Use_decl_annotations_
extern "C" NTSTATUS DriverEntry(
    PDRIVER_OBJECT  pDriverObject,
    PUNICODE_STRING pRegistryPath
)
{
    WDF_DRIVER_CONFIG Config;
    NTSTATUS Status;

    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);

    WDF_DRIVER_CONFIG_INIT(&Config,
        IddSampleDeviceAdd
    );

    Status = WdfDriverCreate(pDriverObject, pRegistryPath, &Attributes, &Config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return Status;
}
//_Use_decl_annotations_
//NTSTATUS DriverEntry(
//    _In_ PDRIVER_OBJECT  DriverObject,
//    _In_ PUNICODE_STRING RegistryPath
//    )
///*++
//
//Routine Description:
//    DriverEntry initializes the driver and is the first routine called by the
//    system after the driver is loaded. DriverEntry specifies the other entry
//    points in the function driver, such as EvtDevice and DriverUnload.
//
//Parameters Description:
//
//    DriverObject - represents the instance of the function driver that is loaded
//    into memory. DriverEntry must initialize members of DriverObject before it
//    returns to the caller. DriverObject is allocated by the system before the
//    driver is loaded, and it is released by the system after the system unloads
//    the function driver from memory.
//
//    RegistryPath - represents the driver specific path in the Registry.
//    The function driver can use the path to store driver related data between
//    reboots. The path does not store hardware instance specific data.
//
//Return Value:
//
//    STATUS_SUCCESS if successful,
//    STATUS_UNSUCCESSFUL otherwise.
//
//--*/
//{
//    WDF_DRIVER_CONFIG config;
//    NTSTATUS status;
//    WDF_OBJECT_ATTRIBUTES attributes;
//
//    //
//    // Register a cleanup callback so that we can call WPP_CLEANUP when
//    // the framework driver object is deleted during driver unload.
//    //
//    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
//    attributes.EvtCleanupCallback = IndirectDisplayEvtDriverContextCleanup;
//
//    WDF_DRIVER_CONFIG_INIT(&config,
//                           IndirectDisplayEvtDeviceAdd
//                           );
//
//    status = WdfDriverCreate(DriverObject,
//                             RegistryPath,
//                             &attributes,
//                             &config,
//                             WDF_NO_HANDLE
//                             );
//
//    if (!NT_SUCCESS(status)) {
//        return status;
//    }
//
//    return status;
//}

_Use_decl_annotations_
NTSTATUS IndirectDisplayEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);

    status = IndirectDisplayCreateDevice(DeviceInit);

    return status;
}

VOID
IndirectDisplayEvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
    )
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);
}
