#include "pch.h"
#include "IndirectDisp.h"

using namespace std;
using namespace indirect_disp;

_Use_decl_annotations_
NTSTATUS Evt_IddDeviceD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState)
{
    UNREFERENCED_PARAMETER(PreviousState);

    // This function is called by WDF to start the device in the fully-on power state.

    auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(Device);
    pContext->pIndirectDeviceContext->D0Entry_InitAdapter();

    return STATUS_SUCCESS;
}


// Called by IddCx after the driver calls 'IddCxMonitorArrival'
// since the IDDCX_SWAPCHAIN is a child of the IDDCX_MONITOR.
// There will only be one assigned swapchain to a given monitor at any time.
//
_Use_decl_annotations_
NTSTATUS Evt_IddMonitorAssignSwapChain(IDDCX_MONITOR MonitorObject, const IDARG_IN_SETSWAPCHAIN* pInArgs)
{
    auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(MonitorObject);
    pContext->pIndirectDeviceContext->AssignSwapChain(pInArgs->hSwapChain, pInArgs->RenderAdapterLuid, pInArgs->hNextSurfaceAvailable);
    return STATUS_SUCCESS;
}

// Called by IddCx after the driver calls 'IddCxMonitorDeparture'
//
_Use_decl_annotations_
NTSTATUS Evt_IddMonitorUnassignSwapChain(IDDCX_MONITOR MonitorObject)
{
    auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(MonitorObject);
    pContext->pIndirectDeviceContext->UnassignSwapChain();
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS Evt_IddAdapterInitFinished(IDDCX_ADAPTER AdapterObject, const IDARG_IN_ADAPTER_INIT_FINISHED* pInArgs)
{
    // This is called when the OS has finished setting up the adapter for use by the IddCx driver. It's now possible
    // to report attached monitors.

    auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(AdapterObject);
    if (NT_SUCCESS(pInArgs->AdapterInitStatus))
    {
        pContext->pIndirectDeviceContext->FinishInit();
    }

    return STATUS_SUCCESS;
}


// Callbacks that actually do something.

_Use_decl_annotations_
NTSTATUS Evt_IddAdapterCommitModes(IDDCX_ADAPTER AdapterObject, const IDARG_IN_COMMITMODES* pInArgs)
{
    UNREFERENCED_PARAMETER(AdapterObject);
    UNREFERENCED_PARAMETER(pInArgs);

    // For the sample, do nothing when modes are picked - the swap-chain is taken care of by IddCx

    // ==============================
    // TODO: In a real driver, this function would be used to reconfigure the device to commit the new modes. Loop
    // through pInArgs->pPaths and look for IDDCX_PATH_FLAGS_ACTIVE. Any path not active is inactive (e.g. the monitor
    // should be turned off).
    // ==============================

    return STATUS_SUCCESS;
}

// Just Manually COUNT the # of modes!
const DWORD MonitorModesCount = 2;

_Use_decl_annotations_
NTSTATUS Evt_IddParseMonitorDescription(const IDARG_IN_PARSEMONITORDESCRIPTION* pInArgs, IDARG_OUT_PARSEMONITORDESCRIPTION* pOutArgs)
{
    // ==============================
    // TODO: In a real driver, this function would be called to generate monitor modes for an EDID by parsing it. In
    // this sample driver, we hard-code the EDID, so this function can generate known modes.
    // ==============================

    pOutArgs->MonitorModeBufferOutputCount = MonitorModesCount;

    if (pInArgs->MonitorModeBufferInputCount < MonitorModesCount)
    {
        // Return success if there was no buffer, since the caller was only asking for a count of modes
        return (pInArgs->MonitorModeBufferInputCount > 0) ? STATUS_BUFFER_TOO_SMALL : STATUS_SUCCESS;
    }
    else
    {
        // Copy the known modes to the output buffer
        for (DWORD ModeIndex = 0; ModeIndex <  MonitorModesCount; ModeIndex++)
        {
            pInArgs->pMonitorModes[ModeIndex].Size = sizeof(IDDCX_MONITOR_MODE);
            pInArgs->pMonitorModes[ModeIndex].Origin = IDDCX_MONITOR_MODE_ORIGIN_MONITORDESCRIPTOR;
            pInArgs->pMonitorModes[ModeIndex].MonitorVideoSignalInfo = IndirectDeviceContext::s_KnownMonitorModes[ModeIndex];
        }

        // Set the preferred mode as represented in the EDID
        pOutArgs->PreferredMonitorModeIdx = 0;

        return STATUS_SUCCESS;
    }
}

_Use_decl_annotations_
NTSTATUS Evt_IddMonitorGetDefaultModes(IDDCX_MONITOR MonitorObject, const IDARG_IN_GETDEFAULTDESCRIPTIONMODES* pInArgs, IDARG_OUT_GETDEFAULTDESCRIPTIONMODES* pOutArgs)
{
    UNREFERENCED_PARAMETER(MonitorObject);
    UNREFERENCED_PARAMETER(pInArgs);
    UNREFERENCED_PARAMETER(pOutArgs);

    // Should never be called since we create a single monitor with a known EDID in this sample driver.

    // ==============================
    // TODO: In a real driver, this function would be called to generate monitor modes for a monitor with no EDID.
    // Drivers should report modes that are guaranteed to be supported by the transport protocol and by nearly all
    // monitors (such 640x480, 800x600, or 1024x768). If the driver has access to monitor modes from a descriptor other
    // than an EDID, those modes would also be reported here.
    // ==============================

    return STATUS_NOT_IMPLEMENTED;
}

/// <summary>
/// Creates a target mode from the fundamental mode attributes.
/// </summary>
void CreateTargetMode(DISPLAYCONFIG_VIDEO_SIGNAL_INFO& Mode, UINT Width, UINT Height, UINT VSync)
{
    Mode.totalSize.cx = Mode.activeSize.cx = Width;
    Mode.totalSize.cy = Mode.activeSize.cy = Height;
    Mode.AdditionalSignalInfo.vSyncFreqDivider = 1;
    Mode.AdditionalSignalInfo.videoStandard = 255;
    Mode.vSyncFreq.Numerator = VSync;
    Mode.vSyncFreq.Denominator = Mode.hSyncFreq.Denominator = 1;
    Mode.hSyncFreq.Numerator = VSync * Height;
    Mode.scanLineOrdering = DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE;
    Mode.pixelRate = VSync * Width * Height;
}

void CreateTargetMode(IDDCX_TARGET_MODE& Mode, UINT Width, UINT Height, UINT VSync)
{
    Mode.Size = sizeof(Mode);
    CreateTargetMode(Mode.TargetVideoSignalInfo.targetVideoSignalInfo, Width, Height, VSync);
}

_Use_decl_annotations_
NTSTATUS Evt_IddMonitorQueryModes(IDDCX_MONITOR MonitorObject, const IDARG_IN_QUERYTARGETMODES* pInArgs, IDARG_OUT_QUERYTARGETMODES* pOutArgs)
{
    UNREFERENCED_PARAMETER(MonitorObject);

    std::vector<IDDCX_TARGET_MODE> TargetModes(4);

    // Create a set of modes supported for frame processing and scan-out. These are typically not based on the
    // monitor's descriptor and instead are based on the static processing capability of the device. The OS will
    // report the available set of modes for a given output as the intersection of monitor modes with target modes.

    CreateTargetMode(TargetModes[0], 1920, 1080, 60);
    CreateTargetMode(TargetModes[1], 1024, 768, 60);
    CreateTargetMode(TargetModes[2], 800, 600, 60);
    CreateTargetMode(TargetModes[3], 640, 480, 60);

    pOutArgs->TargetModeBufferOutputCount = (UINT)TargetModes.size();

    if (pInArgs->TargetModeBufferInputCount >= TargetModes.size())
    {
        std::copy(TargetModes.begin(), TargetModes.end(), pInArgs->pTargetModes);
    }

    return STATUS_SUCCESS;
}

