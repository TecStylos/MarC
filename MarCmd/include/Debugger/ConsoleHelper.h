#pragma once

#if defined MARCMD_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>
#include <conio.h>
#elif defined MARCMD_PLATFORM_UNIX
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <sys/ioctl.h>
#endif

#include <ostream>

namespace MarCmd
{
	namespace Console
	{
		struct Dimensions
		{
			uint64_t width = 0;
			uint64_t height = 0;
		};

		#if defined MARCMD_PLATFORM_WINDOWS
		inline char getChar()
		{
			return _getch();
		}
		inline bool charWaiting()
		{
			return _kbhit() != 0;
		}
		inline Dimensions getDimensions()
		{
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
			Dimensions cd;
			cd.width = uint64_t(csbi.srWindow.Right - csbi.srWindow.Left) + 1;
			cd.height = uint64_t(csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;

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
		inline bool charWaiting()
		{
			struct termios term = { 0 };
			tcgetattr(0, &term);
			termios term2 = term;
			term2.c_lflag &= ~ICANON;
			tcsetattr(0, TCSANOW, &term2);

			int byteswaiting;
			ioctl(0, FIONREAD, &byteswaiting);

			tcsetattr(0, TCSANOW, &term);

			return byteswaiting > 0;
		}
		inline Dimensions getDimensions()
		{
			struct winsize ws;
			ioctl(0, TIOCGWINSZ, &ws);
			Dimensions cd;
			cd.width = ws.ws_col;
			cd.height = ws.ws_row;
			return cd;
		}
		#endif

		typedef enum class CursorVisibility
		{
			EnableBlink,
			DisableBlink,
			Show,
			Hide
		} CurVis;

		struct CursorPos
		{
			uint64_t x, y;
			CursorPos(uint64_t x, uint64_t y) : x(x), y(y) {}
		};


		typedef enum class TextFormatCode
		{
			F_Default = 0,
			F_Bright = 1,
			F_NoBright = 22,
			F_Underline = 4,
			F_NoUnderline = 24,

			F_Negative = 7,
			F_Positive = 27,

			FG_Black = 30,
			FG_Red = 31,
			FG_Green = 32,
			FG_Yellow = 33,
			FG_Blue = 34,
			FG_Magenta = 35,
			FG_Cyan = 36,
			FG_White = 37,
			FG_Default = 39,

			BG_Black = 40,
			BG_Red = 41,
			BG_Green = 42,
			BG_Yellow = 43,
			BG_Blue = 44,
			BG_Magenta = 45,
			BG_Cyan = 46,
			BG_White = 47,
			BG_Default = 49,

			FG_Bright_Black = 90,
			FG_Bright_Red = 91,
			FG_Bright_Green = 92,
			FG_Bright_Yellow = 93,
			FG_Bright_Blue = 94,
			FG_Bright_Magenta = 95,
			FG_Bright_Cyan = 96,
			FG_Bright_White = 97,

			BG_Bright_Black = 100,
			BG_Bright_Red = 101,
			BG_Bright_Green = 102,
			BG_Bright_Yellow = 103,
			BG_Bright_Blue = 104,
			BG_Bright_Magenta = 105,
			BG_Bright_Cyan = 106,
			BG_Bright_White = 107,
		} TFC;

		typedef enum class TextFormatGround
		{
			Foreground = 38,
			Background = 48,
		} TFG;

		struct TextFormat
		{
			std::string subsequence;
		public:
			TextFormat() = default;
			TextFormat(TextFormatCode tfc);
			TextFormat(uint8_t r, uint8_t g, uint8_t b, TextFormatGround ground);
		public:
			static TextFormat ColorFG(uint8_t r, uint8_t g, uint8_t b);
			static TextFormat ColorBG(uint8_t r, uint8_t g, uint8_t b);
		private:
			static TextFormat Color(uint8_t r, uint8_t g, uint8_t b, uint8_t ground);
		};

		std::ostream& operator<<(std::ostream& oStream, CursorVisibility curVis);
		std::ostream& operator<<(std::ostream& oStream, const CursorPos& cp);
		std::ostream& operator<<(std::ostream& oStream, TextFormat tfc);
	}
}
