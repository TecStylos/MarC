#include "Debugger/ConsoleHelper.h"

#include <string>

namespace MarCmd
{
	namespace Console
	{
		TextFormat::TextFormat(TextFormatCode tfc)
		{
			subsequence.append(std::to_string((int)tfc));
		}

		TextFormat::TextFormat(uint8_t r, uint8_t g, uint8_t b, TextFormatGround ground)
		{
			subsequence.append(std::to_string((int)ground));
			subsequence.append(";2;");
			subsequence.append(std::to_string((int)r));
			subsequence.append(";");
			subsequence.append(std::to_string((int)g));
			subsequence.append(";");
			subsequence.append(std::to_string((int)b));
		}

		TextFormat TextFormat::ColorFG(uint8_t r, uint8_t g, uint8_t b)
		{
			return TextFormat(r, g, b, TFG::Foreground);
		}

		TextFormat TextFormat::ColorBG(uint8_t r, uint8_t g, uint8_t b)
		{
			return TextFormat(r, g, b, TFG::Background);
		}

		std::ostream& operator<<(std::ostream& oStream, CursorVisibility curVis)
		{
			switch (curVis)
			{
			case CurVis::EnableBlink:
				return oStream << "\033[?12h";
			case CurVis::DisableBlink:
				return oStream << "\033[?12l";
			case CurVis::Show:
				return oStream << "\033[?25h";
			case CurVis::Hide:
				return oStream << "\033[?25l";
			}
		}
		std::ostream& operator<<(std::ostream& oStream, const CursorPos& cp)
		{
			return oStream << "\033[" << (1 + cp.x) << ";" << (1 + cp.y) << "H";
		}
		std::ostream& operator<<(std::ostream& oStream, TextFormat tf)
		{
			return oStream << "\033[" << tf.subsequence << "m";
		}
	}
}