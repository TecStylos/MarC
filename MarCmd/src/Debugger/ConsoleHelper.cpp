#include "Debugger/ConsoleHelper.h"

#include <string>

namespace MarCmd
{
	namespace Console
	{
		std::ostream& operator<<(std::ostream& oStream, const CursorPos& cp)
		{
			return oStream << "\033[" << (1 + cp.x) << ";" << (1 + cp.y) << "H";
		}
		std::ostream& operator<<(std::ostream& oStream, TextFormat tf)
		{
			return oStream << "\033[" << (int)tf << "m";
		}
	}
}