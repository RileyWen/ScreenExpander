#pragma once

// Suppress the redefinition of NTSTATUS
#include <ntstatus.h>
#define WIN32_NO_STATUS

#ifndef UMDF_MINIMUM_VERSION_REQUIRED
#define UMDF_MINIMUM_VERSION_REQUIRED 27
#endif

#ifndef UMDF_VERSION_MINOR
#define UMDF_VERSION_MINOR 27
#endif


// Important! The minimum Windows versions required for
// each IddCx Version are listed below. Note that This
// Driver is built on IddCx version 1.3.x
//
//    Windows Version    | IddCx Version
// ----------------------|---------------
// 1709 Redstone 3 16299 |    1.2.0
// 1803 Redstone 4 17134 |    1.3.0
// 1809 Redstone 5 17763 |    1.3.8
// 1903    19H1    18362 |    1.4.0
//
#ifndef IDDCX_VERSION_MAJOR
#define IDDCX_VERSION_MAJOR 1
#endif

#ifndef IDDCX_VERSION_MINOR
#define IDDCX_VERSION_MINOR 3
#endif

#include <unknwn.h>
#include <winrt/base.h>

#include <Windows.h>

#include <wdf.h>

#if   IDDCX_VERSION_MINOR == 3
#include <iddcx/1.3/IddCx.h>

#elif IDDCX_VERSION_MINOR >= 4
#include <iddcx/1.4/IddCx.h>

#else
#error At least IddCx 1.3 is required!

#endif

#include <cstdio>
#include <strsafe.h>

#include <dxgi1_6.h>
#include <d3d11_4.h>
#include <avrt.h>

#include <memory>
#include <vector>
#include <array>

#include "SharedDefs.h"

// If defined, new monitors will be reported without EDID.
#define MONITOR_NO_EDID

#pragma hdrstop
