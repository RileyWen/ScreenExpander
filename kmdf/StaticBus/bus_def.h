#ifndef BUS_DEF_H
#define BUS_DEF_H

#define KDPFX "[Riley StatBus] "

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD Bus_EvtDeviceAdd;

NTSTATUS
Bus_StaticEnum(
	_In_ WDFDEVICE Device
);

#endif // !BUS_DEF_H

