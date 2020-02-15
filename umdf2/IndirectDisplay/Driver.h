/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    User-mode Driver Framework 2

--*/
#pragma once

#include "pch.h"
#include "IndirectAdapter.h"
#include "IndirectMonitor.h"
#include "IoControl.h"

struct IndirectAdapterContext {
	indirect_disp::IndirectAdapter* pIndirectAdapter;

	void Cleanup()
	{
		delete pIndirectAdapter;
		pIndirectAdapter = nullptr;
	}
};
WDF_DECLARE_CONTEXT_TYPE(IndirectAdapterContext);

struct IndirectMonitorContext
{
	indirect_disp::IndirectMonitor* pIndirectMonitor;

	void Cleanup()
	{
		delete pIndirectMonitor;
		pIndirectMonitor = nullptr;
	}
};
WDF_DECLARE_CONTEXT_TYPE(IndirectMonitorContext);

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;

EXTERN_C_END

