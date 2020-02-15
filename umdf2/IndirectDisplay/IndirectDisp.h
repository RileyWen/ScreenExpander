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
    class IndirectAdapter
    {
    public:
        IndirectAdapter() {
            m_pIddCxMonitorInfo = new IDDCX_MONITOR_INFO;
            ZeroMemory(m_pIddCxMonitorInfo, sizeof(IDDCX_MONITOR_INFO));
        }

        // Must be called explictly in the callback function specified
        // in WDF Object Attr since WDF is essentially a C environment!
        ~IndirectAdapter() {
            delete m_pIddCxMonitorInfo;
        }

        void IndirectAdapterInit();
        void IndirectAdapterFinishInit();
        bool NewMonitorArrives();

    protected:
        DWORD m_dwNumOfChildDisplay;

        // Why is a 'class' needed in front of a class type in MSVC????
        // FuckMSVC.jpg
        class IndirectMonitor* m_pChildMonitors[8];

        WDFDEVICE m_WdfDevice;
        IDDCX_ADAPTER m_ThisAdapter;

        // The specification on the monitors attached to this adapter.
        //
        // Since now we assume that every child monitor has the same
        // specification, we just store the specication here as a template
        // for each newly arrived monitor.
        //
        // It's initialized in IndirectAdapterFinishInit
        IDDCX_MONITOR_INFO* m_pIddCxMonitorInfo;

    public:
        static const DISPLAYCONFIG_VIDEO_SIGNAL_INFO s_KnownMonitorModes[];
        static const BYTE s_KnownMonitorEdid[];
    };


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
