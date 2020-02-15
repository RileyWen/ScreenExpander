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


    class IndirectMonitor
    {
    public:
        IndirectMonitor(IDDCX_MONITOR IddCxMonitor);
        ~IndirectMonitor();

        void AssignSwapChain(IDDCX_SWAPCHAIN SwapChain, LUID RenderAdapter, HANDLE NewFrameEvent);
        void UnassignSwapChain();

        IDDCX_MONITOR m_ThisMonitor;

    protected:
        DWORD m_MonitorIndex;

        //IDDCX_ADAPTER m_ParentAdapter;

        std::unique_ptr<SwapChainProcessor> m_ProcessingThread;
    };

}
