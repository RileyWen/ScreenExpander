#include "pch.h"

#include "Driver.h"
#include "IndirectAdapter.h"
#include "IndirectMonitor.h"

namespace indirect_disp {

    IndirectAdapter::IndirectAdapter(WDFDEVICE Device) : m_WdfDevice(Device), m_dwNumOfChildDisplay(0)
    {
        m_ThisAdapter = nullptr;

        m_pIddCxMonitorInfo = new IDDCX_MONITOR_INFO;
        ZeroMemory(m_pIddCxMonitorInfo, sizeof(IDDCX_MONITOR_INFO));

        ZeroMemory(m_pChildMonitors, 8 * sizeof(struct IndirectMonitor*));

        // ==============================
        // TODO: Update the below diagnostic information in accordance with the target hardware. The strings and version
        // numbers are used for telemetry and may be displayed to the user in some situations.
        //
        // This is also where static per-adapter capabilities are determined.
        // ==============================

        IDDCX_ADAPTER_CAPS AdapterCaps = {};
        AdapterCaps.Size = sizeof(AdapterCaps);

        // Declare basic feature support for the adapter (required)
        AdapterCaps.MaxMonitorsSupported = 1;
        AdapterCaps.EndPointDiagnostics.Size = sizeof(AdapterCaps.EndPointDiagnostics);
        AdapterCaps.EndPointDiagnostics.GammaSupport = IDDCX_FEATURE_IMPLEMENTATION_NONE;
        AdapterCaps.EndPointDiagnostics.TransmissionType = IDDCX_TRANSMISSION_TYPE_WIRED_OTHER;

        // Declare your device strings for telemetry (required)
        AdapterCaps.EndPointDiagnostics.pEndPointFriendlyName = L"Idd Device";
        AdapterCaps.EndPointDiagnostics.pEndPointManufacturerName = L"Microsoft";
        AdapterCaps.EndPointDiagnostics.pEndPointModelName = L"Idd Model";

        // Declare your hardware and firmware versions (required)
        IDDCX_ENDPOINT_VERSION Version = {};
        Version.Size = sizeof(Version);
        Version.MajorVer = 1;
        AdapterCaps.EndPointDiagnostics.pFirmwareVersion = &Version;
        AdapterCaps.EndPointDiagnostics.pHardwareVersion = &Version;

        // Initialize a WDF context that can store a pointer to the device context object
        WDF_OBJECT_ATTRIBUTES Attr;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, IndirectAdapterContext);
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
            m_ThisAdapter = AdapterInitOut.AdapterObject;

            auto* pContext = WdfObjectGet_IndirectAdapterContext(m_ThisAdapter);
            pContext->pIndirectAdapter = this;

            OutputDebugString(TEXT("[IndirDisp] IddCxAdapterInitAsync succeeded!\n"));
        }
        else {
            TCHAR DebugBuffer[128];

            StringCbPrintf(DebugBuffer, sizeof(DebugBuffer),
                TEXT("[IndirDisp] IddCxAdapterInitAsync failed: 0x%x\n"),
                Status);
            OutputDebugString(DebugBuffer);
        }
    }

    IndirectAdapter::~IndirectAdapter() {
        delete m_pIddCxMonitorInfo;

        // No need to delete 'IndirectMonitor*'s in 'm_pChildMonitors'
        // since we have binded a callback on Wdf Object to clean them up.
    }

    const BYTE IndirectAdapter::s_KnownMonitorEdid[] =
    {
        0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x25,0xC4,0x33,0x23,0x1D,0x09,0x00,0x00,0x08,0x1E,0x01,0x03,0x80,0x34,
        0x1D,0x78,0x2E,0x4E,0xC0,0xA6,0x55,0x50,0x9C,0x26,0x11,0x50,0x54,0x21,0x08,0x00,0xD1,0xC0,0xB3,0x00,0x95,0x00,
        0x81,0x80,0x81,0x40,0x81,0x00,0x81,0xC0,0x71,0x40,0x02,0x3A,0x80,0x18,0x71,0x38,0x2D,0x40,0x58,0x2C,0x45,0x00,
        0x09,0x25,0x21,0x00,0x00,0x1E,0x00,0x00,0x00,0xFD,0x00,0x38,0x4B,0x1E,0x51,0x12,0x00,0x0A,0x20,0x20,0x20,0x20,
        0x20,0x20,0x00,0x00,0x00,0xFC,0x00,0x52,0x69,0x6C,0x65,0x79,0x49,0x6E,0x64,0x69,0x72,0x65,0x63,0x74,0x00,0x00,
        0x00,0xFF,0x00,0x42,0x41,0x4C,0x41,0x42,0x41,0x4C,0x41,0x42,0x41,0x4C,0x41,0x42,0x00,0x6A
    };
    // This is a sample monitor EDID - FOR SAMPLE PURPOSES ONLY
    //{
    //    0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x79,0x5E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xA6,0x01,0x03,0x80,0x28,
    //    0x1E,0x78,0x0A,0xEE,0x91,0xA3,0x54,0x4C,0x99,0x26,0x0F,0x50,0x54,0x20,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,
    //    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0xA0,0x0F,0x20,0x00,0x31,0x58,0x1C,0x20,0x28,0x80,0x14,0x00,
    //    0x90,0x2C,0x11,0x00,0x00,0x1E,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //    0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //    0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6E
    //};


    void IndirectAdapter::IndirectAdapterFinishInit()
    {
        // ==============================
        // TODO: In a real driver, the EDID should be retrieved dynamically from a connected physical monitor. The EDID
        // provided here is purely for demonstration, as it describes only 640x480 @ 60 Hz and 800x600 @ 60 Hz. Monitor
        // manufacturers are required to correctly fill in physical monitor attributes in order to allow the OS to optimize
        // settings like viewing distance and scale factor. Manufacturers should also use a unique serial number every
        // single device to ensure the OS can tell the monitors apart.
        // ==============================

        m_pIddCxMonitorInfo->Size = sizeof(IDDCX_MONITOR_INFO);
        m_pIddCxMonitorInfo->MonitorType = DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI;
        m_pIddCxMonitorInfo->ConnectorIndex = 0;
        m_pIddCxMonitorInfo->MonitorDescription.Size = sizeof(IDDCX_MONITOR_INFO::MonitorDescription);
        m_pIddCxMonitorInfo->MonitorDescription.Type = IDDCX_MONITOR_DESCRIPTION_TYPE_EDID;
        m_pIddCxMonitorInfo->MonitorDescription.DataSize = sizeof(s_KnownMonitorEdid);
        m_pIddCxMonitorInfo->MonitorDescription.pData = const_cast<BYTE*>(s_KnownMonitorEdid);

        // ==============================
        // TODO: The monitor's container ID should be distinct from "this" device's container ID if the monitor is not
        // permanently attached to the display adapter device object. The container ID is typically made unique for each
        // monitor and can be used to associate the monitor with other devices, like audio or input devices. In this
        // sample we generate a random container ID GUID, but it's best practice to choose a stable container ID for a
        // unique monitor or to use "this" device's container ID for a permanent/integrated monitor.
        // ==============================

        // Create a container ID
        CoCreateGuid(&m_pIddCxMonitorInfo->MonitorContainerId);
    }

    bool IndirectAdapter::NewMonitorArrives(_Out_ DWORD& NewMonitorIndex) {
        NewMonitorIndex = 0xFFFFFFFF;

        WDF_OBJECT_ATTRIBUTES Attr;
        IDARG_IN_MONITORCREATE MonitorCreate = {};

        if (m_dwNumOfChildDisplay > 8)
            return false;

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, IndirectMonitorContext);
        Attr.EvtCleanupCallback = [](WDFOBJECT Object)
        {
            // Automatically cleanup the context when the WDF object is about to be deleted
            auto* pContext = WdfObjectGet_IndirectMonitorContext(Object);
            if (pContext)
            {
                pContext->Cleanup();
            }
        };

        MonitorCreate.ObjectAttributes = &Attr;
        MonitorCreate.pMonitorInfo = m_pIddCxMonitorInfo;

        // Create a monitor object with the specified monitor descriptor
        IDARG_OUT_MONITORCREATE MonitorCreateOut;
        NTSTATUS Status = IddCxMonitorCreate(m_ThisAdapter, &MonitorCreate, &MonitorCreateOut);
        if (NT_SUCCESS(Status))
        {
            int index;
            for (index = 0; index < 8; index++)
                if (m_pChildMonitors[index] == NULL)
                    break;
            
            NewMonitorIndex = index;
            m_pChildMonitors[index] = new IndirectMonitor(MonitorCreateOut.MonitorObject);

            // Associate the monitor with this device context
            auto* pContext = WdfObjectGet_IndirectMonitorContext(MonitorCreateOut.MonitorObject);
            pContext->pIndirectMonitor = m_pChildMonitors[index];

            // Tell the OS that the monitor has been plugged in
            IDARG_OUT_MONITORARRIVAL ArrivalOut;
            Status = IddCxMonitorArrival(m_pChildMonitors[index]->m_ThisMonitor, &ArrivalOut);
        }
        return true;
    }

    const UINT64 MHZ = 1000000;
    const UINT64 KHZ = 1000;

    // A list of modes exposed by the sample monitor EDID - FOR SAMPLE PURPOSES ONLY
    const DISPLAYCONFIG_VIDEO_SIGNAL_INFO IndirectAdapter::s_KnownMonitorModes[] =
    {
        // 800 x 600 @ 60Hz
        {
              40 * MHZ,                                      // pixel clock rate [Hz]
            { 40 * MHZ, 800 + 256 },                         // fractional horizontal refresh rate [Hz]
            { 40 * MHZ, (800 + 256) * (600 + 28) },          // fractional vertical refresh rate [Hz]
            { 800, 600 },                                    // (horizontal, vertical) active pixel resolution
            { 800 + 256, 600 + 28 },                         // (horizontal, vertical) total pixel resolution
            { { 255, 0 }},                                   // video standard and vsync divider
            DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE
        },
        // 640 x 480 @ 60Hz
        {
              25175 * KHZ,                                   // pixel clock rate [Hz]
            { 25175 * KHZ, 640 + 160 },                      // fractional horizontal refresh rate [Hz]
            { 25175 * KHZ, (640 + 160) * (480 + 46) },       // fractional vertical refresh rate [Hz]
            { 640, 480 },                                    // (horizontal, vertical) active pixel resolution
            { 640 + 160, 480 + 46 },                         // (horizontal, vertical) blanking pixel resolution
            { { 255, 0 } },                                  // video standard and vsync divider
            DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE
        },
    };

}