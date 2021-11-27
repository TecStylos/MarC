#pragma once

#if defined MARCMD_PLATFORM_WINDOWS
#include <conio.h>
#elif defined MARCMD_PLATFORM_UNIX
#endif

namespace MarCmd
{
	#if defined MARCMD_PLATFORM_WINDOWS
	inline char getChar()
	{
		return _getch();
	}
	#elif defined MARCMD_PLATFORM_UNIX
	inline char getChar()
	{
		;
	}
	#endif
}