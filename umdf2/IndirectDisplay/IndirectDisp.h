#pragma once

#include "pch.h"
#include "Swapchain.h"

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

    // Provides a sample implementation of an indirect display driver.
    class IndirectMonitor
    {
    public:
        IndirectMonitor(_In_ WDFDEVICE WdfDevice);
        virtual ~IndirectMonitor();

        void D0Entry_InitMonitor();
        void FinishInit();

        void AssignSwapChain(IDDCX_SWAPCHAIN SwapChain, LUID RenderAdapter, HANDLE NewFrameEvent);
        void UnassignSwapChain();

    protected:

        WDFDEVICE m_WdfDevice;
        IDDCX_ADAPTER m_ParentAdapter;
        IDDCX_MONITOR m_ThisMonitor;

        std::unique_ptr<SwapChainProcessor> m_ProcessingThread;

    public:
        static const DISPLAYCONFIG_VIDEO_SIGNAL_INFO s_KnownMonitorModes[];
        static const BYTE s_KnownMonitorEdid[];
    };

}
