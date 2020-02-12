#pragma once

#include "pch.h"

EVT_WDF_DRIVER_DEVICE_ADD Evt_IddDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY Evt_IddDeviceD0Entry;

EVT_IDD_CX_ADAPTER_INIT_FINISHED Evt_IddAdapterInitFinished;
EVT_IDD_CX_ADAPTER_COMMIT_MODES Evt_IddAdapterCommitModes;

EVT_IDD_CX_PARSE_MONITOR_DESCRIPTION Evt_IddParseMonitorDescription;
EVT_IDD_CX_MONITOR_GET_DEFAULT_DESCRIPTION_MODES Evt_IddMonitorGetDefaultModes;
EVT_IDD_CX_MONITOR_QUERY_TARGET_MODES Evt_IddMonitorQueryModes;

EVT_IDD_CX_MONITOR_ASSIGN_SWAPCHAIN Evt_IddMonitorAssignSwapChain;
EVT_IDD_CX_MONITOR_UNASSIGN_SWAPCHAIN Evt_IddMonitorUnassignSwapChain;

namespace indirect_disp {
    struct Direct3DDevice
    {
        Direct3DDevice() = delete;
        Direct3DDevice(LUID AdapterLuid) ;

        HRESULT Init();

		LUID AdapterLuid;
        winrt::com_ptr<ID3D11Device> Device;
        winrt::com_ptr<ID3D11DeviceContext> DeviceContext;
    };

    class SwapChainProcessor
    {
    public:
        SwapChainProcessor(IDDCX_SWAPCHAIN hSwapChain, std::shared_ptr<Direct3DDevice> Device, HANDLE NewFrameEvent);
        ~SwapChainProcessor();

    private:
        // A new thread that executes Run()
        static DWORD CALLBACK RunThread(LPVOID Argument);
        
        // Do preparation and cleanup job for RunCore()
        void Run();

        // Core Processing on Image Data
        void RunCore();

    public:
        IDDCX_SWAPCHAIN m_hSwapChain;
        std::shared_ptr<Direct3DDevice> m_Device;
        
        // 
        winrt::handle m_hThread;

        // Imformed by IddCx framework that there's a new image available
        HANDLE m_hAvailableBufferEvent;

        // Informed by user or our driver to stop processing images
        winrt::handle m_hTerminateEvent;

    };

    // Provides a sample implementation of an indirect display driver.
    class IndirectDeviceContext
    {
    public:
        IndirectDeviceContext(_In_ WDFDEVICE WdfDevice);
        virtual ~IndirectDeviceContext();

        void D0Entry_InitAdapter();
        void FinishInit();

        void AssignSwapChain(IDDCX_SWAPCHAIN SwapChain, LUID RenderAdapter, HANDLE NewFrameEvent);
        void UnassignSwapChain();

    protected:

        WDFDEVICE m_WdfDevice;
        IDDCX_ADAPTER m_Adapter;
        IDDCX_MONITOR m_Monitor;

        std::unique_ptr<SwapChainProcessor> m_ProcessingThread;

    public:
        static const DISPLAYCONFIG_VIDEO_SIGNAL_INFO s_KnownMonitorModes[];
        static const BYTE s_KnownMonitorEdid[];
    };

	struct IndirectDeviceContextWrapper
	{
		IndirectDeviceContext* pIndirectDeviceContext;

		void Cleanup()
		{
			delete pIndirectDeviceContext;
			pIndirectDeviceContext = nullptr;
		}
	};

	// This macro creates the methods for accessing an IndirectDeviceContextWrapper as a context for a WDF object
	WDF_DECLARE_CONTEXT_TYPE(IndirectDeviceContextWrapper);
}
