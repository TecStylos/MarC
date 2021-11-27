#pragma once

namespace MarCmd
{
	const char* HelpText =
		"Options:\n"
		"    --help            View this help page.\n"
		"    --verbose         View more runtime information.\n"
		"  Modes (If none selected, pass a single *.mcc/*.mca/*.mco/*.mce file to interpret):\n"
		"    --livecode        Live interpret MarC code from the console.\n"
		"    --liveasm         Live interpret MarCembly code from the console.\n"
		"    --build           Comile/Assemble/Link a *.mcc/*.mca file.\n"
		"    --disasm          Disassemble a *.mce file and store the modules disassemblies in the output directory.\n"
		"    --debug           Debug a *.mcc/*.mca/*.mce file.\n"
		"    --interpret       Interpret a *.mcc/*.mca/*.mce file.\n"
		"  File IO:\n"
		"    -o [filepath]     Output file.\n"
		"    -m [directory]    Directory to search for modules in (Can be used multiple times).\n"
		"    -e [directory]    Directory to search for extensions in (Can be used multiple times).\n"
		"  Exit behavior: (Default: Keeps MarCmd open when the exit code is non-zero.)\n"
		"    --keeponexit      Keep MarCmd open after the execution has finished.\n"
		"    --closeonexit     Close MarCmd after the execution has finished.\n"
		"    --noexitinfo      Don't view the \"Module '...' exited with code x.\" message.\n"
		"  Debugging:\n"
		"    --profile         Profile the interpreter (func/funx/instruction call counts/timings, execution time, ...)\n"
		"    --dbginfo         With 'build' switch: Generate debug information for the application.\n"
		"    --verbose         Show more details when building/running code.\n"
		"  Miscellaneous:\n"
		"    [file]            Any unknown argument gets interpreted as the input file.\n"
		;
}
