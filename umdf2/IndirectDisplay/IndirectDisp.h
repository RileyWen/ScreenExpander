#pragma once

#include <unknwn.h>
#include <winrt/base.h>
#include <Windows.h>
#include <wdf.h>

#ifndef IDDCX_VERSION_MAJOR
#define IDDCX_VERSION_MAJOR 1
#endif

#ifndef IDDCX_VERSION_MINOR
#define IDDCX_VERSION_MINOR 2
#endif


#include <iddcx/1.2/IddCx.h>


#include <dxgi1_6.h>
#include <d3d11_4.h>
#include <avrt.h>

#include <memory>
#include <vector>

EVT_WDF_DRIVER_DEVICE_ADD IddDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY IddDeviceD0Entry;

EVT_IDD_CX_ADAPTER_INIT_FINISHED IddAdapterInitFinished;
EVT_IDD_CX_ADAPTER_COMMIT_MODES IddAdapterCommitModes;

EVT_IDD_CX_PARSE_MONITOR_DESCRIPTION IddParseMonitorDescription;
EVT_IDD_CX_MONITOR_GET_DEFAULT_DESCRIPTION_MODES IddMonitorGetDefaultModes;
EVT_IDD_CX_MONITOR_QUERY_TARGET_MODES IddMonitorQueryModes;

EVT_IDD_CX_MONITOR_ASSIGN_SWAPCHAIN IddMonitorAssignSwapChain;
EVT_IDD_CX_MONITOR_UNASSIGN_SWAPCHAIN IddMonitorUnassignSwapChain;

namespace indirect_disp {
    struct Direct3DDevice
    {
        Direct3DDevice(LUID AdapterLuid);
        Direct3DDevice();
        HRESULT Init();

        LUID AdapterLuid;
        winrt::com_ptr<IDXGIFactory5> DxgiFactory;
        winrt::com_ptr<IDXGIAdapter1> Adapter;
        winrt::com_ptr<ID3D11Device> Device;
        winrt::com_ptr<ID3D11DeviceContext> DeviceContext;
    };

    class SwapChainProcessor
    {
    public:
        SwapChainProcessor(IDDCX_SWAPCHAIN hSwapChain, std::shared_ptr<Direct3DDevice> Device, HANDLE NewFrameEvent);
        ~SwapChainProcessor();

    private:
        static DWORD CALLBACK RunThread(LPVOID Argument);

        void Run();
        void RunCore();

    public:
        IDDCX_SWAPCHAIN m_hSwapChain;
        std::shared_ptr<Direct3DDevice> m_Device;
        HANDLE m_hAvailableBufferEvent;
        winrt::handle m_hThread;
        winrt::handle m_hTerminateEvent;
    };

    // Provides a sample implementation of an indirect display driver.
    class IndirectDeviceContext
    {
    public:
        IndirectDeviceContext(_In_ WDFDEVICE WdfDevice);
        virtual ~IndirectDeviceContext();

        void InitAdapter();
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
}
