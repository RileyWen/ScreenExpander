#pragma once

#include "pch.h"

namespace indirect_disp {
    class IndirectAdapter
    {
    public:
        IndirectAdapter(WDFDEVICE Device);

        // Must be called explictly in the callback function specified
        // in WDF Object Attr since WDF is essentially a C environment!
        ~IndirectAdapter();

        // Called after the constructor. See the constructor.
        void IndirectAdapterFinishInit();
        
        bool NewMonitorArrives();

        // In case of failed IddCxAdapterInitAsync
        bool IsAdapterNull() { return nullptr == m_ThisAdapter; }

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
}
