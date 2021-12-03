#pragma once

#if defined MARCMD_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>
#elif defined MARCMD_PLATFORM_UNIX
#endif

namespace MarCmd
{
	#if defined MARCMD_PLATFORM_WINDOWS
	std::string CurrExePath()
	{
		char buff[1024];
		GetModuleFileName(NULL, buff, sizeof(buff));
		return buff;
	}

	#elif defined MARCMD_PLATFORM_UNIX

	std::string CurrExePath()
	{
		;
	}

	#endif
}