#pragma once

#include <string>

namespace MarCmd
{
	enum class Mode
	{
		None,
		Help,
		LiveCode,        // Live interpret MarC code from commandline
		LiveAsm,         // Live interpret MarCembly code from commandline
		Build,           // *.mcc/*.mca -> *.mce
		Disassemble,     // *.mcc -> *.mcd
		Debug,           // Debug a *.mcc/*.mca/*.mcc file
		Interpret        // Run ([compile,] assemble, link) any file asscociated with the MarC/MarCembly languages
	};
}