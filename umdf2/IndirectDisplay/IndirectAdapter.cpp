#include "pch.h"

#include "Driver.h"
#include "IndirectAdapter.h"
#include "IndirectMonitor.h"

namespace indirect_disp {

    IndirectAdapter::IndirectAdapter(WDFDEVICE Device) : m_WdfDevice(Device), m_dwNumOfChildDisplay(0)
    {
        m_AdapterContext.IddCxAdapterObject = nullptr;

        ZeroMemory(m_pChildMonitors, MAX_MONITOR_CAPACITY * sizeof(struct IndirectMonitor*));

        // =========================== Comment of Original Forked Code =====================================================
        // TODO: Update the below diagnostic information in accordance with the target hardware. The strings and version
        // numbers are used for telemetry and may be displayed to the user in some situations.
        //
        // This is also where static per-adapter capabilities are determined.
        // =================================================================================================================

        IDDCX_ADAPTER_CAPS AdapterCaps = {};
        AdapterCaps.Size = sizeof(AdapterCaps);

        // Declare basic feature support for the adapter (required)
        AdapterCaps.MaxMonitorsSupported = MAX_MONITOR_CAPACITY;
        AdapterCaps.EndPointDiagnostics.Size = sizeof(AdapterCaps.EndPointDiagnostics);

        // Seems useless
        AdapterCaps.MaxDisplayPipelineRate = MAX_DISPLAY_WIDTH * MAX_DISPLAY_HEIGHT * MAX_DISPLAY_REFRESH_RATE_AT_MAX_RESOLUTION *
            32ULL * MAX_MONITOR_CAPACITY;

        AdapterCaps.EndPointDiagnostics.GammaSupport = IDDCX_FEATURE_IMPLEMENTATION_NONE;
        AdapterCaps.EndPointDiagnostics.TransmissionType = IDDCX_TRANSMISSION_TYPE_WIRED_OTHER;

        // Declare your device strings for telemetry (required)
        AdapterCaps.EndPointDiagnostics.pEndPointFriendlyName = L"Idd Device";
        AdapterCaps.EndPointDiagnostics.pEndPointManufacturerName = L"RileyW";
        AdapterCaps.EndPointDiagnostics.pEndPointModelName = L"Idd Model";

        // Declare your hardware and firmware versions (required)
        IDDCX_ENDPOINT_VERSION Version = {};
        Version.Size = sizeof(Version);
        Version.MajorVer = 1;
        AdapterCaps.EndPointDiagnostics.pFirmwareVersion = &Version;
        AdapterCaps.EndPointDiagnostics.pHardwareVersion = &Version;

        // Initialize a WDF context that can store a pointer to the device context object
        WDF_OBJECT_ATTRIBUTES Attr;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, AdapterWdfContext);
        // No need to specify cleanup callback here since we have already specified it when
        // declaring the WDF Object Attr of the underlying device (notice the underlying device
        // and the indirect adapter is a 1-to-1 mapping).

        IDARG_IN_ADAPTER_INIT AdapterInit = {};
        AdapterInit.WdfDevice = m_WdfDevice;
        AdapterInit.pCaps = &AdapterCaps;
        AdapterInit.ObjectAttributes = &Attr;

        // Start the initialization of the adapter, which will trigger the AdapterFinishInit callback later
        IDARG_OUT_ADAPTER_INIT AdapterInitOut;
        NTSTATUS Status = IddCxAdapterInitAsync(&AdapterInit, &AdapterInitOut);

        if (NT_SUCCESS(Status))
        {
            // Store a reference to the WDF adapter handle
            m_AdapterContext.IddCxAdapterObject = AdapterInitOut.AdapterObject;
            m_AdapterContext.pAdaterClass = this;

            auto* pContext = WdfObjectGet_AdapterWdfContext(m_AdapterContext.IddCxAdapterObject);
            pContext->pIndirectAdapter = this;

            PrintfDebugString("IddCxAdapterInitAsync succeeded!\n");
        }
        else {
            PrintfDebugString("IddCxAdapterInitAsync failed: 0x%x\n", Status);
        }

        DWORD dwAdvisoryBufferSize = sizeof(IMAGE_FRAME);
        m_PipeServer.InitConnectedPipe(dwAdvisoryBufferSize);
    }

    IndirectAdapter::~IndirectAdapter() {
        // No need to delete 'IndirectMonitor*'s in 'm_pChildMonitors'
        // since we have binded a callback on Wdf Object to clean them up.
    }

    //const BYTE IndirectAdapter::s_KnownMonitorEdid[] =
    //{
    //    0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x25,0xC4,0x33,0x23,0x1D,0x09,0x00,0x00,0x08,0x1E,0x01,0x03,0x80,0x34,
    //    0x1D,0x78,0x2E,0x4E,0xC0,0xA6,0x55,0x50,0x9C,0x26,0x11,0x50,0x54,0x21,0x08,0x00,0xD1,0xC0,0xB3,0x00,0x95,0x00,
    //    0x81,0x80,0x81,0x40,0x81,0x00,0x81,0xC0,0x71,0x40,0x02,0x3A,0x80,0x18,0x71,0x38,0x2D,0x40,0x58,0x2C,0x45,0x00,
    //    0x09,0x25,0x21,0x00,0x00,0x1E,0x00,0x00,0x00,0xFD,0x00,0x38,0x4B,0x1E,0x51,0x12,0x00,0x0A,0x20,0x20,0x20,0x20,
    //    0x20,0x20,0x00,0x00,0x00,0xFC,0x00,0x52,0x69,0x6C,0x65,0x79,0x49,0x6E,0x64,0x69,0x72,0x65,0x63,0x74,0x00,0x00,
    //    0x00,0xFF,0x00,0x42,0x41,0x4C,0x41,0x42,0x41,0x4C,0x41,0x42,0x41,0x4C,0x41,0x42,0x00,0x6A
    //};
    // This is a sample monitor EDID - FOR SAMPLE PURPOSES ONLY (EDID in the Original Forked Code)
    //{
    //    0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x79,0x5E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xA6,0x01,0x03,0x80,0x28,
    //    0x1E,0x78,0x0A,0xEE,0x91,0xA3,0x54,0x4C,0x99,0x26,0x0F,0x50,0x54,0x20,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,
    //    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0xA0,0x0F,0x20,0x00,0x31,0x58,0x1C,0x20,0x28,0x80,0x14,0x00,
    //    0x90,0x2C,0x11,0x00,0x00,0x1E,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //    0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //    0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6E
    //};

    bool IndirectAdapter::NewMonitorArrives(DWORD* dwNewMonitorIndex) {
        // =========================== Comment of Original Forked Code =====================================================
        // TODO: In a real driver, the EDID should be retrieved dynamically from a connected physical monitor. The EDID
        // provided here is purely for demonstration, as it describes only 640x480 @ 60 Hz and 800x600 @ 60 Hz. Monitor
        // manufacturers are required to correctly fill in physical monitor attributes in order to allow the OS to optimize
        // settings like viewing distance and scale factor. Manufacturers should also use a unique serial number every
        // single device to ensure the OS can tell the monitors apart.
        // =================================================================================================================

        *dwNewMonitorIndex = 0xFFFFFFFF;

        // Find an empty slot for newly arrived monitors.
        DWORD indexOfEmptySlot;
        for (indexOfEmptySlot = 0; indexOfEmptySlot < MAX_MONITOR_CAPACITY; indexOfEmptySlot++)
            if (m_pChildMonitors[indexOfEmptySlot] == nullptr)
                break;

        // If there's no empty slot, refuse to set up a new monitor and return false.
        if (indexOfEmptySlot >= MAX_MONITOR_CAPACITY)
            return false;

        m_dwNumOfChildDisplay++;

        // Start initializing the new monitor.

        IDDCX_MONITOR_INFO MonitorInfo = {};
        MonitorInfo.Size = sizeof(IDDCX_MONITOR_INFO);
        MonitorInfo.MonitorType = DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI;

        // This is a zero based unique identifier for this connector, it should be unique for this adapter and the value should
        // not change for this connector across system reboot or driver upgrade.
        // The value has to be between **0** and **(IDDCX_ADAPTER_CAPS.MaxMonitorsSupported-1)**
        MonitorInfo.ConnectorIndex = indexOfEmptySlot;

        MonitorInfo.MonitorDescription.Size = sizeof(IDDCX_MONITOR_INFO::MonitorDescription);
        MonitorInfo.MonitorDescription.Type = IDDCX_MONITOR_DESCRIPTION_TYPE_EDID;

        // The monitor description is EDID or no EDID description available
        // If the monitor has no description then IDDCX_MONITOR_DESCRIPTION_TYPE_EDID shall be used with zero description size
        // and null pointer for data
#ifndef MONITOR_NO_EDID
        MonitorInfo.MonitorDescription.DataSize = IndirectSampleMonitorInfo::szEdidBlock;
        MonitorInfo.MonitorDescription.pData = const_cast<BYTE*>(s_SampleMonitorInfo[MANUALLY_SPECIFIED_MONITOR_INFO_INDEX].pEdidBlock);
#else
        m_pIddCxMonitorInfo.MonitorDescription.DataSize = 0;
        m_pIddCxMonitorInfo.MonitorDescription.pData = nullptr;
#endif

        // =========================== Comment of Original Forked Code =====================================================
        // TODO: The monitor's container ID should be distinct from "this" device's container ID if the monitor is not
        // permanently attached to the display adapter device object. The container ID is typically made unique for each
        // monitor and can be used to associate the monitor with other devices, like audio or input devices. In this
        // sample we generate a random container ID GUID, but it's best practice to choose a stable container ID for a
        // unique monitor or to use "this" device's container ID for a permanent/integrated monitor.
        // =================================================================================================================

        // Create a container ID
        CoCreateGuid(&MonitorInfo.MonitorContainerId);


        // ============================= Create WDF Object Context for IddCx Monitor =================================
        WDF_OBJECT_ATTRIBUTES Attr;
        IDARG_IN_MONITORCREATE ArgInMonitorCreate = {};

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, MonitorWdfContext);
        Attr.EvtCleanupCallback = [](WDFOBJECT Object)
        {
            // Automatically cleanup the context when the WDF object is about to be deleted
            auto* pContext = WdfObjectGet_MonitorWdfContext(Object);
            if (pContext)
                pContext->Cleanup();
        };
        // ===========================================================================================================


        // ======= Create a monitor object with `MonitorInfo` and `WDF object context` decleared above ===============
        ArgInMonitorCreate.ObjectAttributes = &Attr;
        ArgInMonitorCreate.pMonitorInfo = &MonitorInfo;

        IDARG_OUT_MONITORCREATE ArgOutMonitorCreate = {};
        NTSTATUS Status = IddCxMonitorCreate(m_AdapterContext.IddCxAdapterObject, &ArgInMonitorCreate, &ArgOutMonitorCreate);
        if (!NT_SUCCESS(Status)) {
            PrintfDebugString("IddCxMonitorCreate Failed: 0x%x\n", Status);
            return false;
        }
        // ===========================================================================================================

        // ================== Tell the OS that the monitor has been plugged in =======================================
        IDARG_OUT_MONITORARRIVAL ArrivalOut = {};
        Status = IddCxMonitorArrival(ArgOutMonitorCreate.MonitorObject, &ArrivalOut);
        if (!NT_SUCCESS(Status)) {
            PrintfDebugString("IddCxMonitorArrival Failed: 0x%x\n", Status);
            
            WdfObjectDelete(ArgOutMonitorCreate.MonitorObject);
            return false;
        }
        // ===========================================================================================================

        // Store the IDDCX_MONITOR in corresponding indirect_disp::IndirectMonitor class
        m_pChildMonitors[indexOfEmptySlot] = new IndirectMonitor(
            indexOfEmptySlot,
            ArgOutMonitorCreate.MonitorObject,
            &this->m_AdapterContext);

        m_pChildMonitors[indexOfEmptySlot]->m_MonitorContext.IddCxMonitorObj = ArgOutMonitorCreate.MonitorObject;

        // Store the pointer in Monitor's WDF context
        auto* pMonitorWdfContext = WdfObjectGet_MonitorWdfContext(ArgOutMonitorCreate.MonitorObject);
        pMonitorWdfContext->pIndirectMonitor = m_pChildMonitors[indexOfEmptySlot];

        // Set the out function argument
        *dwNewMonitorIndex = indexOfEmptySlot;

        return true;
    }

    bool IndirectAdapter::MonitorDepart(DWORD MonitorIndex) {
        NTSTATUS Status = IddCxMonitorDeparture(m_pChildMonitors[MonitorIndex]->m_MonitorContext.IddCxMonitorObj);
        STATUS_MONITOR_INVALID_DESCRIPTOR_CHECKSUM;

        if (NT_SUCCESS(Status)) {
            PrintfDebugString("IddCxMonitorDeparture Succeeded. Monitor Index: %ud\n", MonitorIndex);

            // The class pointed by m_pChildMonitors[MonitorIndex] has been deleted by WDF Context callback.
            // We just set the dangling pointer here to nullptr.
            m_pChildMonitors[MonitorIndex] = nullptr;

            return true;
        }
        else {
            PrintfDebugString("IddCxMonitorDeparture Failed: 0x%x. Monitor Index: %ud.\n", Status, MonitorIndex);

            return false;
        }
    }
}