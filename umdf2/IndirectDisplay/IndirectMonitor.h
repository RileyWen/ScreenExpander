#pragma once

#include "pch.h"
#include "Swapchain.h"
#include "IndirectAdapter.h"

namespace indirect_disp {


    struct IndirectMonitor
    {
        IndirectMonitor(_In_ IDDCX_MONITOR IddCxMonitorObj, _In_ IndirectAdapter* pParentAdapter);
        ~IndirectMonitor();

        void AssignSwapChain(IDDCX_SWAPCHAIN SwapChain, LUID RenderAdapter, HANDLE NewFrameEvent);
        void UnassignSwapChain();

        //DWORD m_MonitorIndex;

        IDDCX_MONITOR m_ThisMonitorIddCxObj;
        IndirectAdapter* m_pParentAdapter;

        std::unique_ptr<SwapChainProcessor> m_ProcessingThread;
    };

}
