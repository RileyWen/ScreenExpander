#include "pch.h"
#include "Swapchain.h"
#include "IndirectMonitor.h"

namespace indirect_disp {

#pragma region Direct3DDevice

    Direct3DDevice::Direct3DDevice(LUID AdapterLuid)
        : AdapterLuid(AdapterLuid) { }

    HRESULT Direct3DDevice::Init()
    {
        winrt::com_ptr<IDXGIFactory5> DxgiFactory;
        winrt::com_ptr<IDXGIAdapter1> Adapter;

        // The DXGI factory could be cached, but if a new render adapter appears on the system, a new factory needs to be
        // created. If caching is desired, check DxgiFactory->IsCurrent() each time and recreate the factory if !IsCurrent.
        HRESULT hr = CreateDXGIFactory2(0, __uuidof(DxgiFactory), DxgiFactory.put_void());
        if (FAILED(hr))
        {
            return hr;
        }

        // Find the specified render adapter
        hr = DxgiFactory->EnumAdapterByLuid(AdapterLuid, __uuidof(Adapter), Adapter.put_void());
        if (FAILED(hr))
        {
            return hr;
        }

        // Create a D3D device using the render adapter. BGRA support is required by the WHQL test suite.
        hr = D3D11CreateDevice(Adapter.get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr, 0, D3D11_SDK_VERSION, Device.put(), nullptr, DeviceContext.put());
        if (FAILED(hr))
        {
            // If creating the D3D device failed, it's possible the render GPU was lost (e.g. detachable GPU) or else the
            // system is in a transient state.
            return hr;
        }

        return S_OK;
    }

#pragma endregion

#pragma region SwapChainProcessor

    SwapChainProcessor::SwapChainProcessor(
        IDDCX_SWAPCHAIN hSwapChain, std::shared_ptr<Direct3DDevice> Device,
        HANDLE NewFrameEvent, IndirectMonitor* pParentMonitor)
        : m_hThisSwapChainIddCxObj(hSwapChain), m_Device(Device),
        m_hAvailableBufferEvent(NewFrameEvent), m_pParentMonitor(pParentMonitor)
    {
        m_hTerminateEvent.attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));

        // Immediately create and run the swap-chain processing thread, passing 'this' as the thread parameter
        m_hThread.attach(CreateThread(nullptr, 0, RunThread, this, 0, nullptr));

        m_pImageBuf = std::make_unique<IMAGE_FRAME>();
    }

    SwapChainProcessor::~SwapChainProcessor()
    {
        // Alert the swap-chain processing thread to terminate
        SetEvent(m_hTerminateEvent.get());

        if (m_hThread.get())
        {
            // Wait for the thread to terminate
            WaitForSingleObject(m_hThread.get(), INFINITE);
        }
    }

    DWORD CALLBACK SwapChainProcessor::RunThread(LPVOID Argument)
    {
        reinterpret_cast<SwapChainProcessor*>(Argument)->Run();
        return 0;
    }

    void SwapChainProcessor::Run()
    {
        // For improved performance, make use of the Multimedia Class Scheduler Service, which will intelligently
        // prioritize this thread for improved throughput in high CPU-load scenarios.
        DWORD AvTask = 0;
        HANDLE AvTaskHandle = AvSetMmThreadCharacteristics(L"Distribution", &AvTask);

        RunCore();

        // Always delete the swap-chain object when swap-chain processing loop terminates in order to kick the system to
        // provide a new swap-chain if necessary.
        WdfObjectDelete((WDFOBJECT)m_hThisSwapChainIddCxObj);
        m_hThisSwapChainIddCxObj = nullptr;

        AvRevertMmThreadCharacteristics(AvTaskHandle);
    }

    void SwapChainProcessor::RunCore()
    {
        // Get the DXGI device interface
        winrt::com_ptr<IDXGIDevice> DxgiDevice;
        bool result = m_Device->Device.try_as(DxgiDevice);
        if (!result)
        {
            return;
        }

        IDARG_IN_SWAPCHAINSETDEVICE SetDevice = {};
        SetDevice.pDevice = DxgiDevice.get();

        HRESULT hr;
        hr = IddCxSwapChainSetDevice(m_hThisSwapChainIddCxObj, &SetDevice);
        if (FAILED(hr))
        {
            return;
        }

        // Acquire and release buffers in a loop
        for (;;)
        {
            winrt::com_ptr<IDXGIResource> AcquiredBuffer;
            winrt::com_ptr<ID3D11Texture2D> AcquiredTexture;
            winrt::com_ptr<ID3D11Texture2D> CopiedTexture;


            // Ask for the next buffer from the producer
            IDARG_OUT_RELEASEANDACQUIREBUFFER Buffer = {};
            hr = IddCxSwapChainReleaseAndAcquireBuffer(m_hThisSwapChainIddCxObj, &Buffer);

            // AcquireBuffer immediately returns STATUS_PENDING if no buffer is yet available
            if (hr == E_PENDING)
            {
                // We must wait for a new buffer
                HANDLE WaitHandles[] =
                {
                    m_hAvailableBufferEvent,
                    m_hTerminateEvent.get()
                };
                DWORD WaitResult = WaitForMultipleObjects(ARRAYSIZE(WaitHandles), WaitHandles, FALSE, 16);
                if (WaitResult == WAIT_OBJECT_0 || WaitResult == WAIT_TIMEOUT)
                {
                    // We have a new buffer, so try the AcquireBuffer again
                    continue;
                }
                else if (WaitResult == WAIT_OBJECT_0 + 1)
                {
                    // We need to terminate
                    break;
                }
                else
                {
                    // The wait was cancelled or something unexpected happened
                    hr = HRESULT_FROM_WIN32(WaitResult);
                    break;
                }
            }
            else if (SUCCEEDED(hr))
            {
                AcquiredBuffer.attach(Buffer.MetaData.pSurface);


                if (!AcquiredBuffer.try_as(AcquiredTexture)) {
                    PrintfDebugString("Cannot Convert Acquired Buffer to Texture Failed\n");
                    goto EndOneSurfaceProcessing;
                }

                D3D11_TEXTURE2D_DESC TextureDesc;
                AcquiredTexture->GetDesc(&TextureDesc);

                //PrintfDebugString("Current Surface Width: Format: %u, Width: %u, Height: %u\n",
                //    SurDesc.Format, SurDesc.Width, SurDesc.Height);

                D3D11_MAPPED_SUBRESOURCE MappedSubResc;
                hr = m_Device->DeviceContext->Map(AcquiredTexture.get(), 0, D3D11_MAP_READ, 0, &MappedSubResc);
                if (FAILED(hr)) {
                    if (hr == E_INVALIDARG) {
                        D3D11_TEXTURE2D_DESC StagingTextureDesc;
                        StagingTextureDesc = TextureDesc;
                        StagingTextureDesc.Usage = D3D11_USAGE_STAGING;
                        StagingTextureDesc.BindFlags = 0;
                        StagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                        StagingTextureDesc.MiscFlags = 0;

                        hr = m_Device->Device->CreateTexture2D(&StagingTextureDesc, nullptr, CopiedTexture.put());
                        if (FAILED(hr)) {
                            PrintfDebugString("Create Staging Texture failed: 0x%x\n", hr);
                            goto EndOneSurfaceProcessing;
                        }

                        m_Device->DeviceContext->CopyResource(CopiedTexture.get(), AcquiredTexture.get());

                        hr = m_Device->DeviceContext->Map(CopiedTexture.get(), 0, D3D11_MAP_READ, 0, &MappedSubResc);
                        if (FAILED(hr)) {
                            PrintfDebugString("Mapping Staging Texture failed: 0x%x\n", hr);
                            goto EndOneSurfaceProcessing;
                        }

                        AcquiredTexture = std::move(CopiedTexture);
                    }
                    else {
                        PrintfDebugString("Mapping GPU Texture failed: 0x%x\n", hr);
                        goto EndOneSurfaceProcessing;
                    }

                }

                // The image format is always DXGI_FORMAT_B8G8R8A8_UNORM,
                // so the size of a frame is Height*Width bytes.
                m_pImageBuf->dwWidth = TextureDesc.Width;
                m_pImageBuf->dwHeight = TextureDesc.Height;
                CopyMemory(m_pImageBuf->pData, MappedSubResc.pData, TextureDesc.Width * TextureDesc.Height);

                m_pParentMonitor->m_pParentAdapter->m_PipeServer.WriteBytes(
                    m_pImageBuf.get(),
                    sizeof(DWORD) * 2 + TextureDesc.Width * TextureDesc.Height);

                //m_pParentMonitor->m_pParentAdapter->m_PipeServer.WriteBytes(
                //    MappedSubResc.pData,
                //    TextureDesc.Width * TextureDesc.Height);
                //
                //m_Device->DeviceContext->Unmap(AcquiredTexture.get(), 0);

                // ==============================
                // TODO: Process the frame here
                //
                // This is the most performance-critical section of code in an IddCx driver. It's important that whatever
                // is done with the acquired surface be finished as quickly as possible. This operation could be:
                //  * a GPU copy to another buffer surface for later processing (such as a staging surface for mapping to CPU memory)
                //  * a GPU encode operation
                //  * a GPU VPBlt to another surface
                //  * a GPU custom compute shader encode operation
                // ==============================


            EndOneSurfaceProcessing:

                AcquiredBuffer = nullptr;
                hr = IddCxSwapChainFinishedProcessingFrame(m_hThisSwapChainIddCxObj);
                if (FAILED(hr))
                {
                    break;
                }

                // ==============================
                // TODO: Report frame statistics once the asynchronous encode/send work is completed
                //
                // Drivers should report information about sub-frame timings, like encode time, send time, etc.
                // ==============================
                // IddCxSwapChainReportFrameStatistics(m_hThisSwapChainIddCxObj, ...);
            }
            else
            {
                // The swap-chain was likely abandoned (e.g. DXGI_ERROR_ACCESS_LOST), so exit the processing loop
                break;
            }
        }
    }

#pragma endregion
}