#include "pch.h"
#include "IndirectDisp.h"

namespace indirect_disp {

#pragma region IndirectDeviceContext

	const UINT64 MHZ = 1000000;
	const UINT64 KHZ = 1000;

	// A list of modes exposed by the sample monitor EDID - FOR SAMPLE PURPOSES ONLY
	const DISPLAYCONFIG_VIDEO_SIGNAL_INFO IndirectDeviceContext::s_KnownMonitorModes[] =
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

	// This is a sample monitor EDID - FOR SAMPLE PURPOSES ONLY
	const BYTE IndirectDeviceContext::s_KnownMonitorEdid[] =
	{
		0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x79,0x5E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xA6,0x01,0x03,0x80,0x28,
		0x1E,0x78,0x0A,0xEE,0x91,0xA3,0x54,0x4C,0x99,0x26,0x0F,0x50,0x54,0x20,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0xA0,0x0F,0x20,0x00,0x31,0x58,0x1C,0x20,0x28,0x80,0x14,0x00,
		0x90,0x2C,0x11,0x00,0x00,0x1E,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6E
	};

	IndirectDeviceContext::IndirectDeviceContext(_In_ WDFDEVICE WdfDevice) :
		m_WdfDevice(WdfDevice)
	{
	}

	IndirectDeviceContext::~IndirectDeviceContext()
	{
		m_ProcessingThread.reset();
	}

	void IndirectDeviceContext::D0Entry_InitAdapter()
	{
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
		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, IndirectDeviceContextWrapper);

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
			m_Adapter = AdapterInitOut.AdapterObject;

			// Store the device context object into the WDF object context
			auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(AdapterInitOut.AdapterObject);
			pContext->pIndirectDeviceContext = this;
		}
	}

	void IndirectDeviceContext::FinishInit()
	{
		// ==============================
		// TODO: In a real driver, the EDID should be retrieved dynamically from a connected physical monitor. The EDID
		// provided here is purely for demonstration, as it describes only 640x480 @ 60 Hz and 800x600 @ 60 Hz. Monitor
		// manufacturers are required to correctly fill in physical monitor attributes in order to allow the OS to optimize
		// settings like viewing distance and scale factor. Manufacturers should also use a unique serial number every
		// single device to ensure the OS can tell the monitors apart.
		// ==============================

		WDF_OBJECT_ATTRIBUTES Attr;
		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, IndirectDeviceContextWrapper);

		IDDCX_MONITOR_INFO MonitorInfo = {};
		MonitorInfo.Size = sizeof(MonitorInfo);
		MonitorInfo.MonitorType = DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI;
		MonitorInfo.ConnectorIndex = 0;
		MonitorInfo.MonitorDescription.Size = sizeof(MonitorInfo.MonitorDescription);
		MonitorInfo.MonitorDescription.Type = IDDCX_MONITOR_DESCRIPTION_TYPE_EDID;
		MonitorInfo.MonitorDescription.DataSize = sizeof(s_KnownMonitorEdid);
		MonitorInfo.MonitorDescription.pData = const_cast<BYTE*>(s_KnownMonitorEdid);

		// ==============================
		// TODO: The monitor's container ID should be distinct from "this" device's container ID if the monitor is not
		// permanently attached to the display adapter device object. The container ID is typically made unique for each
		// monitor and can be used to associate the monitor with other devices, like audio or input devices. In this
		// sample we generate a random container ID GUID, but it's best practice to choose a stable container ID for a
		// unique monitor or to use "this" device's container ID for a permanent/integrated monitor.
		// ==============================

		// Create a container ID
		CoCreateGuid(&MonitorInfo.MonitorContainerId);

		IDARG_IN_MONITORCREATE MonitorCreate = {};
		MonitorCreate.ObjectAttributes = &Attr;
		MonitorCreate.pMonitorInfo = &MonitorInfo;

		// Create a monitor object with the specified monitor descriptor
		IDARG_OUT_MONITORCREATE MonitorCreateOut;
		NTSTATUS Status = IddCxMonitorCreate(m_Adapter, &MonitorCreate, &MonitorCreateOut);
		if (NT_SUCCESS(Status))
		{
			m_Monitor = MonitorCreateOut.MonitorObject;

			// Associate the monitor with this device context
			auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(MonitorCreateOut.MonitorObject);
			pContext->pIndirectDeviceContext = this;

			// Tell the OS that the monitor has been plugged in
			IDARG_OUT_MONITORARRIVAL ArrivalOut;
			Status = IddCxMonitorArrival(m_Monitor, &ArrivalOut);
		}
	}

	void IndirectDeviceContext::AssignSwapChain(IDDCX_SWAPCHAIN SwapChain, LUID RenderAdapter, HANDLE NewFrameEvent)
	{
		m_ProcessingThread.reset();

		auto Device = std::make_shared<Direct3DDevice>(RenderAdapter);
		if (FAILED(Device->Init()))
		{
			// It's important to delete the swap-chain if D3D initialization fails, so that the OS knows to generate a new
			// swap-chain and try again.
			WdfObjectDelete(SwapChain);
		}
		else
		{
			// Create a new swap-chain processing thread
			m_ProcessingThread.reset(new SwapChainProcessor(SwapChain, Device, NewFrameEvent));
		}
	}

	void IndirectDeviceContext::UnassignSwapChain()
	{
		// Stop processing the last swap-chain
		m_ProcessingThread.reset();
	}

#pragma endregion

}
