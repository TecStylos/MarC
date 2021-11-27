#pragma once

#if defined MARCMD_PLATFORM_WINDOWS
#include <conio.h>
#elif defined MARCMD_PLATFORM_UNIX
#include <unistd.h>
#include <termios.h>
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
		char buf = 0;
		struct termios old = { 0 };
		fflush(stdout);
		if (tcgetattr(0, &old) < 0)
			perror("tcgetattr()");
		old.c_lflag &= ~ICANON;
		old.c_lflag &= ~ECHO;
		old.c_cc[VMIN] = 1;
		old.c_cc[VTIME] = 0;
		if (tcsetattr(0, TCSANOW, &old) < 0)
			perror("tcsetattr ICANON");
		if (read(0, &buf, 1) < 0)
			perror("read()");
		old.c_lflag |= ICANON;
		old.c_lflag |= ECHO;
		if (tcsetattr(0, TCSADRAIN, &old) < 0)
			perror("tcsetattr ~ICANON");
		return buf;
	}
	#endif
}
