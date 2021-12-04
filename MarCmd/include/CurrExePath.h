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
		uint64_t count = GetModuleFileName(NULL, buff, sizeof(buff));
		return std::string(buff, count);
	}

	#elif defined MARCMD_PLATFORM_UNIX

	std::string CurrExePath()
	{
		char buff[1024];
		uint64_t count = readlink("/proc/self/exe", buff, sizeof(buff));
		return std::string(buff, count);
	}

	#endif
}