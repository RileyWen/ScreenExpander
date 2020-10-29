#pragma once

#include "pch.h"
#include "IndirectAdapter.h"

namespace indirect_disp {
    
    // Avoid recursive header reference
    class SwapChainProcessor;

    struct IndirectMonitor
    {
        IndirectMonitor(_In_ DWORD MonitorIndex, _In_ IDDCX_MONITOR IddCxMonitorObj, _In_ IndirectAdapter::AdapterContext* pAdapterContext);
        ~IndirectMonitor();

        void AssignSwapChain(IDDCX_SWAPCHAIN SwapChain, LUID RenderAdapter, HANDLE NewFrameEvent);
        void UnassignSwapChain();

        struct MonitorContext {
            DWORD MonitorIndex;
            IDDCX_MONITOR IddCxMonitorObj;
            IndirectAdapter::AdapterContext* pAdapterContext;
        };

        struct MonitorContext m_MonitorContext;

        std::unique_ptr<SwapChainProcessor> m_ProcessingThread;
    };

}
