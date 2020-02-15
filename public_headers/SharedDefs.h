#pragma once

#include <initguid.h>
// GUID of the interface that allow user application
// to notify the indirect display driver that a monitor
// arrives or departs. 
//
// {6b06164c-8a07-4820-9139-19758f29e43e}
//
DEFINE_GUID (GUID_DEVINTERFACE_INDIRECT_DEVICE,
    0x6b06164c,0x8a07,0x4820,0x91,0x39,0x19,0x75,0x8f,0x29,0xe4,0x3e);

// {4d36e968-e325-11ce-bfc1-08002be10318}
// This is a system-preserved device class GUID for display adapters
DEFINE_GUID(GUID_DEV_CLASS_DISPLAY_ADAPTER,
	0x4d36e968, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);

// GUID for Indirect Display Device
// {6C2DAA24-08C9-4BDF-8791-68DA023EDA11}
DEFINE_GUID(GUID_DEV_INDIRECT_DISP ,
	0x6c2daa24, 0x8c9, 0x4bdf, 0x87, 0x91, 0x68, 0xda, 0x2, 0x3e, 0xda, 0x11);

#define INDIRECT_DISPLAY_HARDWARE_IDS L"{6C2DAA24-08C9-4BDF-8791-68DA023EDA11}\\Indirect_Disp\0"
#define INDIRECT_DISPLAY_HARDWARE_IDS_LEN sizeof (INDIRECT_DISPLAY_HARDWARE_IDS)

#define INDIRECT_DISPLAY_COMPATIBLE_IDS L"{6C2DAA24-08C9-4BDF-8791-68DA023EDA11}\\Indirect_Disp_Compatible\0"
#define INDIRECT_DISPLAY_COMPATIBLE_IDS_LEN sizeof (INDIRECT_DISPLAY_COMPATIBLE_IDS)

