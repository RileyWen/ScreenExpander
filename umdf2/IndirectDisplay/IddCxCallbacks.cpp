#include "pch.h"

#include "IddCxCallbacks.h"
#include "IndirectAdapter.h"
#include "IndirectMonitor.h"
#include "Driver.h"

using namespace std;

void CreateTargetMode(DISPLAYCONFIG_VIDEO_SIGNAL_INFO& Mode, UINT Width, UINT Height, UINT VSync);

#pragma region Indirect Adapter Callback Forwarding

// Original comment: This function is called by WDF to start the 
//                   device (or Adapter) in the fully-on power state.
//
// Called when the device underlying the virtual indirect adatper is ready.
//
_Use_decl_annotations_
NTSTATUS Evt_IddDeviceD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState)
{
    UNREFERENCED_PARAMETER(PreviousState);

    OutputDebugString(TEXT("[IndirDisp] Enter Evt_IddDeviceD0Entry\n"));

    // Initialize the indirect adapter since the underlying device is ready.
    auto* pAdapterContext = WdfObjectGet_IndirectAdapterContext(Device);
    pAdapterContext->pIndirectAdapter = new indirect_disp::IndirectAdapter(Device);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS Evt_IddAdapterInitFinished(IDDCX_ADAPTER AdapterObject, const IDARG_IN_ADAPTER_INIT_FINISHED* pInArgs)
{
    OutputDebugString(TEXT("[IndirDisp] Enter Evt_IddAdapterInitFinished\n"));

    // This is called when the OS has finished setting up the adapter for use by the IddCx driver. It's now possible
    // to report attached monitors.

    auto* pContext = WdfObjectGet_IndirectAdapterContext(AdapterObject);
    if (NT_SUCCESS(pInArgs->AdapterInitStatus))
    {
        pContext->pIndirectAdapter->IndirectAdapterFinishInit();
    }

    return STATUS_SUCCESS;
}

#pragma endregion

#pragma region Indirect Monitor Callback Forwarding

// Called by IddCx after the driver calls 'IddCxMonitorArrival'
// since the IDDCX_SWAPCHAIN is a child of the IDDCX_MONITOR.
// There will only be one assigned swapchain to a given monitor at any time.
//
_Use_decl_annotations_
NTSTATUS Evt_IddMonitorAssignSwapChain(IDDCX_MONITOR MonitorObject, const IDARG_IN_SETSWAPCHAIN* pInArgs)
{
    OutputDebugString(TEXT("[IndirDisp] Enter Evt_IddMonitorAssignSwapChain\n"));

    auto* pContext = WdfObjectGet_IndirectMonitorContext(MonitorObject);
    pContext->pIndirectMonitor->AssignSwapChain(pInArgs->hSwapChain, pInArgs->RenderAdapterLuid, pInArgs->hNextSurfaceAvailable);
    return STATUS_SUCCESS;
}

// Called by IddCx after the driver calls 'IddCxMonitorDeparture'
//
_Use_decl_annotations_
NTSTATUS Evt_IddMonitorUnassignSwapChain(IDDCX_MONITOR MonitorObject)
{
    OutputDebugString(TEXT("[IndirDisp] Enter Evt_IddMonitorUnassignSwapChain\n"));

    auto* pContext = WdfObjectGet_IndirectMonitorContext(MonitorObject);
    pContext->pIndirectMonitor->UnassignSwapChain();
    return STATUS_SUCCESS;
}

#pragma endregion

#pragma region Monitor Callbacks Only When No EDID Is Provided

// A driver DDI function that OS calls to request the driver provide the default monitor mode list for the
// specified monitor. This DDI is called when a monitor without a description (EDID in this driver) is connected.
// It should never be called when we create a single monitor with a known EDID.
//
// This function should be called twice. In the first call OS asks for the buffer size (in the # of modes) required.
// It's in the second call that OS provides actual buffer.
_Use_decl_annotations_
NTSTATUS Evt_IddMonitorGetDefaultModes(IDDCX_MONITOR MonitorObject, const IDARG_IN_GETDEFAULTDESCRIPTIONMODES* pInArgs, IDARG_OUT_GETDEFAULTDESCRIPTIONMODES* pOutArgs)
{
    UNREFERENCED_PARAMETER(MonitorObject);
    //UNREFERENCED_PARAMETER(pInArgs);
    //UNREFERENCED_PARAMETER(pOutArgs);

    OutputDebugString(TEXT("[IndirDisp] Enter Evt_IddMonitorGetDefaultModes\n"));

    const DWORD MonitorModesCount = 4;

    pOutArgs->DefaultMonitorModeBufferOutputCount = 4;      // The # of monitor modes that OS should provides buffer for

    if (pInArgs->DefaultMonitorModeBufferInputCount < MonitorModesCount)
    {
        pOutArgs->PreferredMonitorModeIdx = NO_PREFERRED_MODE;

        // Return success if there was no buffer, since the caller was only asking for a count of modes
        return (pInArgs->DefaultMonitorModeBufferInputCount > 0) ? STATUS_BUFFER_TOO_SMALL : STATUS_SUCCESS;
    }
    else {  // In the second call by OS. Copy the known modes to the output buffer
        std::array<std::pair<UINT, UINT>, MonitorModesCount> resolutions = {
            std::make_pair(1920, 1080), std::make_pair(1600, 900),
            std::make_pair(1024, 768),  std::make_pair(800, 600),
        };

        for (DWORD ModeIndex = 0; ModeIndex < MonitorModesCount; ModeIndex++)
        {
            pInArgs->pDefaultMonitorModes[ModeIndex].Size = sizeof(IDDCX_MONITOR_MODE);
            pInArgs->pDefaultMonitorModes[ModeIndex].Origin = IDDCX_MONITOR_MODE_ORIGIN_DRIVER;
            CreateTargetMode(pInArgs->pDefaultMonitorModes[ModeIndex].MonitorVideoSignalInfo,
                resolutions[ModeIndex].first,resolutions[ModeIndex].second, 60);
        }

        pOutArgs->PreferredMonitorModeIdx = 0;
    }

    return STATUS_SUCCESS;

    // =========================== Comment of Original Forked Code =====================================================
    // TODO: In a real driver, this function would be called to generate monitor modes for a monitor with no EDID.
    // Drivers should report modes that are guaranteed to be supported by the transport protocol and by nearly all
    // monitors (such 640x480, 800x600, or 1024x768). If the driver has access to monitor modes from a descriptor other
    // than an EDID, those modes would also be reported here.
    // =================================================================================================================

    //return STATUS_NOT_IMPLEMENTED;
}

#if IDDCX_VERSION_MINOR >= 4
// A driver DDI function that OS calls to query the physical size of a remote Edid-less monitor
//
// If the driver knows the physical size of the monitor is should fill in the pOutArgs and return 
// STATUS_SUCCESS, in the event the driver does not know the physical size it should 
// return STATUS_NO_DATA_DETECTED.
//
// This DDI is only called for remote drivers that do not provide a monitor descriptions for the 
// specified monitor.  If a remote driver does specify a monitor description for a given monitor 
// then the physical width and height will be taken from the description and this DDI will not be 
// called for that monitor.
//
_Use_decl_annotations_
NTSTATUS Evt_IddMonitorGetPhysicalSize(_In_ IDDCX_MONITOR MonitorObject, _Out_ IDARG_OUT_MONITORGETPHYSICALSIZE* pOutArgs)
{
    UNREFERENCED_PARAMETER(MonitorObject);

    OutputDebugString(TEXT("[IndirDisp] Enter Evt_IddMonitorGetPhysicalSize\n"));

    const UINT cm = 100;
    pOutArgs->PhysicalWidth = 48 * cm;
    pOutArgs->PhysicalHeight = 27 * cm;

    return STATUS_SUCCESS;
}
#endif

#pragma endregion

#pragma region Indirect Monitor Callbacks Applied Only When EDID Is Provided

// The FIRST Function to call after reporting a new monitor arrives. 
//
_Use_decl_annotations_
NTSTATUS Evt_IddParseMonitorDescription(const IDARG_IN_PARSEMONITORDESCRIPTION* pInArgs, IDARG_OUT_PARSEMONITORDESCRIPTION* pOutArgs)
{
    OutputDebugString(TEXT("[IndirDisp] Enter Evt_IddParseMonitorDescription\n"));

    // ==============================
    // TODO: In a real driver, this function would be called to generate monitor modes for an EDID by parsing it. In
    // this sample driver, we hard-code the EDID, so this function can generate known modes.
    // ==============================

    // Just Manually COUNT the # of modes!
    const DWORD MonitorModesCount = 2;

    pOutArgs->MonitorModeBufferOutputCount = MonitorModesCount;

    if (pInArgs->MonitorModeBufferInputCount < MonitorModesCount)
    {
        // Return success if there was no buffer, since the caller was only asking for a count of modes
        return (pInArgs->MonitorModeBufferInputCount > 0) ? STATUS_BUFFER_TOO_SMALL : STATUS_SUCCESS;
    }
    else
    {
        // Copy the known modes to the output buffer
        for (DWORD ModeIndex = 0; ModeIndex < MonitorModesCount; ModeIndex++)
        {
            pInArgs->pMonitorModes[ModeIndex].Size = sizeof(IDDCX_MONITOR_MODE);
            pInArgs->pMonitorModes[ModeIndex].Origin = IDDCX_MONITOR_MODE_ORIGIN_MONITORDESCRIPTOR;
            pInArgs->pMonitorModes[ModeIndex].MonitorVideoSignalInfo = indirect_disp::IndirectAdapter::s_KnownMonitorModes[ModeIndex];
        }

        // Set the preferred mode as represented in the EDID
        pOutArgs->PreferredMonitorModeIdx = 0;

        return STATUS_SUCCESS;
    }
}


/// <summary>
/// Creates a target mode from the fundamental mode attributes.
/// </summary>
void CreateTargetMode(DISPLAYCONFIG_VIDEO_SIGNAL_INFO& Mode, UINT Width, UINT Height, UINT VSync)
{
    // ----------- Documentation in IddCx.h --------------
    // This is the details of the target mode. Note that AdditionalSignalInfo.vSyncFreqDivider has to have a zero value
    // NOTE : DISPLAYCONFIG_VIDEO_SIGNAL_INFO.vSyncFreq is the Vsync rate between the Indirect Display device and the
    // connected monitor.  DISPLAYCONFIG_VIDEO_SIGNAL_INFO.AdditionalSignalInfo.vSyncFreqDivider is used to
    // calculate the rate at which the OS will update the desktop image.
    //
    // The desktop update rate will calculate be :
    // DISPLAYCONFIG_VIDEO_SIGNAL_INFO.vSyncFreq / DISPLAYCONFIG_VIDEO_SIGNAL_INFO.AdditionalSignalInfo.vSyncFreqDivider
    //
    // DISPLAYCONFIG_VIDEO_SIGNAL_INFO.AdditionalSignalInfo.vSyncFreqDivider cannot be zero
    Mode.totalSize.cx = Mode.activeSize.cx = Width;
    Mode.totalSize.cy = Mode.activeSize.cy = Height;

    Mode.AdditionalSignalInfo.vSyncFreqDivider = 1; // OS updates desktop at the same rate as the Indirect Display device.
                                                    // See comments above.

    Mode.AdditionalSignalInfo.videoStandard = 255;  // D3DKMDT_VSS_OTHER
    Mode.vSyncFreq.Numerator = VSync;
    Mode.vSyncFreq.Denominator = Mode.hSyncFreq.Denominator = 1;
    Mode.hSyncFreq.Numerator = VSync * Height;
    Mode.scanLineOrdering = DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE;
    Mode.pixelRate = UINT64(VSync) * UINT64(Width) * UINT64(Height);
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

    OutputDebugString(TEXT("[IndirDisp] Enter Evt_IddMonitorQueryModes\n"));

    std::vector<IDDCX_TARGET_MODE> TargetModes(4);

    // Create a set of modes supported for frame processing and scan-out. These are typically not based on the
    // monitor's descriptor and instead are based on the static processing capability of the device. The OS will
    // report the available set of modes for a given output as the **INTERSECTION** of monitor modes with target modes.

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

#pragma endregion


#pragma region Do-nothing Functions

_Use_decl_annotations_
NTSTATUS Evt_IddAdapterCommitModes(IDDCX_ADAPTER AdapterObject, const IDARG_IN_COMMITMODES* pInArgs)
{
    UNREFERENCED_PARAMETER(AdapterObject);
    UNREFERENCED_PARAMETER(pInArgs);

    OutputDebugString(TEXT("[IndirDisp] Enter Evt_IddAdapterCommitModes\n"));

    // For the sample, do nothing when modes are picked - the swap-chain is taken care of by IddCx

    // ==============================
    // TODO: In a real driver, this function would be used to reconfigure the device to commit the new modes. Loop
    // through pInArgs->pPaths and look for IDDCX_PATH_FLAGS_ACTIVE. Any path not active is inactive (e.g. the monitor
    // should be turned off).
    // ==============================

    return STATUS_SUCCESS;
}

#pragma endregion

