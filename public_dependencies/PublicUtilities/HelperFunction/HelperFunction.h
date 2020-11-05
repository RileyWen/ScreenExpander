#pragma once

#include "pch.h"

std::string LastErrorCodeToStringA(DWORD dwErrorCode);

std::string GetLastErrorAsStringA();

std::string WSAGetLastErrorAsStringA();
