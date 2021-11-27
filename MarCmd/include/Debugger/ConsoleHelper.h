#pragma once

#if defined MARCMD_PLATFORM_WINDOWS
#include <Windows.h>
#include <conio.h>
#elif defined MARCMD_PLATFORM_UNIX
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <sys/ioctl.h>
#endif

namespace MarCmd
{
	struct ConsoleDimensions
	{
		uint64_t width = 0;
		uint64_t height = 0;
	};

	#if defined MARCMD_PLATFORM_WINDOWS
	inline char getChar()
	{
		return _getch();
	}
	inline ConsoleDimensions getConsoleDimensions()
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		ConsoleDimensions cd;
		cd.width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		cd.height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

		return cd;
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
	inline ConsoleDimensions getConsoleDimensions()
	{
		struct winsize ws;
		ioctl(0, TIOCGWINSZ, &ws);
		ConsoleDimensions cd;
		cd.width = ws.ws_col;
		cd.height = ws.ws_row;
		return cd;
	}
	#endif
}
