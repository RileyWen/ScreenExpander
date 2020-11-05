#ifndef BUS_DEF_H
#define BUS_DEF_H

#define KDPFX "[Riley StatBus] "

EXTERN_C
DRIVER_INITIALIZE DriverEntry;

EXTERN_C
EVT_WDF_DRIVER_DEVICE_ADD Bus_EvtDeviceAdd;

EXTERN_C
NTSTATUS Bus_StaticEnum(_In_ WDFDEVICE Device);

#endif // !BUS_DEF_H

