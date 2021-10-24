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
		Execute,         // Execute a *.mce file
		Assemble,        // Assemble a *.mcc file
		Compile,         // Compile a *.mca file
		Link,            // Link a *.mco file
		Interpret        // Run ([assemble,] compile, link) a *.mcc/*.mca file
	};
}