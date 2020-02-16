#include "pch.h"
#include "IndirectMonitor.h"

namespace indirect_disp {

#pragma region IndirectMonitor
    IndirectMonitor::IndirectMonitor(_In_ IDDCX_MONITOR IddCxMonitor) : m_ThisMonitor(IddCxMonitor)
    {
    }

    IndirectMonitor::~IndirectMonitor()
    {
        m_ProcessingThread.reset();
    }

    void IndirectMonitor::AssignSwapChain(IDDCX_SWAPCHAIN SwapChain, LUID RenderAdapter, HANDLE NewFrameEvent)
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

    void IndirectMonitor::UnassignSwapChain()
    {
        // Stop processing the last swap-chain
        m_ProcessingThread.reset();
    }

#pragma endregion

}
