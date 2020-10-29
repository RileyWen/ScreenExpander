#pragma once

#include "pch.h"
#include "AsyncPipeServer.h"

namespace indirect_disp {
    class IndirectAdapter
    {
        friend struct IndirectMonitor;

    public:
        static constexpr DWORD MAX_MONITOR_CAPACITY = 8;

        // Only for test purpose
        static constexpr DWORD MANUALLY_SPECIFIED_MONITOR_INFO_INDEX = 1;

        static constexpr DWORD MAX_DISPLAY_WIDTH = MAX_IMAGE_WIDTH;

        static constexpr DWORD MAX_DISPLAY_HEIGHT = MAX_IMAGE_HEIGHT;

        static constexpr DWORD MAX_DISPLAY_REFRESH_RATE_AT_MAX_RESOLUTION = 60;

        IndirectAdapter(WDFDEVICE Device);

        // Must be called explictly in the callback function specified
        // in WDF Object Attr since WDF is essentially a C environment!
        ~IndirectAdapter();

        // Called after the constructor. See the constructor.
        void IndirectAdapterFinishInit() {}

        bool NewMonitorArrives(DWORD* NewMonitorIndex);

        bool MonitorDepart(DWORD MonitorIndex);

        // In case of failed IddCxAdapterInitAsync
        bool IsAdapterNull() { return nullptr == m_AdapterContext.IddCxAdapterObject; }

    protected:
        struct AdapterContext {
            IDDCX_ADAPTER IddCxAdapterObject;
            IndirectAdapter* pAdaterClass;
        };

        AdapterContext m_AdapterContext;

        DWORD m_dwNumOfChildDisplay;

        // Why is a 'class' needed in front of a class type in MSVC????
        // FuckMSVC.jpg
        struct IndirectMonitor* m_pChildMonitors[MAX_MONITOR_CAPACITY];

        WDFDEVICE m_WdfDevice;

    public:
        AsyncPipeServer m_PipeServer;

        struct IndirectSampleMonitorInfo
        {
            static constexpr size_t szEdidBlock = 128;
            static constexpr size_t szModeList = 3;

            const BYTE pEdidBlock[szEdidBlock];
            const struct SampleMonitorMode {
                DWORD Width;
                DWORD Height;
                DWORD VSync;
            } pModeList[szModeList];
            const DWORD ulPreferredModeIdx;
        };

        static const struct IndirectSampleMonitorInfo s_SampleMonitorInfo[];
    };
}
