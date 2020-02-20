#pragma once

#include "pch.h"

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
        SwapChainProcessor(IDDCX_SWAPCHAIN hSwapChain, std::shared_ptr<Direct3DDevice> Device,
                           HANDLE NewFrameEvent, struct IndirectMonitor* pParentMonitor);
        ~SwapChainProcessor();

    private:
        // A new thread that executes Run()
        static DWORD CALLBACK RunThread(LPVOID Argument);
        
        // Do preparation and cleanup job for RunCore()
        void Run();

        // Core Processing on Image Data
        void RunCore();

    public:
        struct IndirectMonitor* m_pParentMonitor;

        IDDCX_SWAPCHAIN m_hThisSwapChainIddCxObj;
        std::shared_ptr<Direct3DDevice> m_Device;
        
        // A handle that points to 'RunThread' 
        winrt::handle m_hThread;

        // Imformed by IddCx framework that there's a new image available
        HANDLE m_hAvailableBufferEvent;

        // Informed by user or our driver to stop processing images
        winrt::handle m_hTerminateEvent;
    };
}
