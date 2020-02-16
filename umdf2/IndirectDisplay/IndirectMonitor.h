#pragma once

#include "pch.h"
#include "Swapchain.h"

namespace indirect_disp {


    struct IndirectMonitor
    {
        IndirectMonitor(_In_ IDDCX_MONITOR IddCxMonitor);
        ~IndirectMonitor();

        void AssignSwapChain(IDDCX_SWAPCHAIN SwapChain, LUID RenderAdapter, HANDLE NewFrameEvent);
        void UnassignSwapChain();

        IDDCX_MONITOR m_ThisMonitor;
        DWORD m_MonitorIndex;

        //IDDCX_ADAPTER m_ParentAdapter;

        std::unique_ptr<SwapChainProcessor> m_ProcessingThread;
    };

}
