#pragma once

#include "PublicHeader.h"

namespace std
{
#ifdef UNICODE
	extern wostream& _tcout;
	extern wistream& _tcin;
	using _tstring = wstring;
#else
	extern ostream& _tcout;
	extern istream& _tcin;
#endif // UNICODE
}

#define T(_info_) TEXT##(##_info_##)

#define INFO(_info_) _tprintf(TEXT(_info_))


void PrintCSBackupAPIErrorMessage(DWORD dwErr);

void GetStrLastError();
