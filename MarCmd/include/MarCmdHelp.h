#pragma once

namespace MarCmd
{
	constexpr char* HelpText =
		"Options:\n"
		"    -help            View this help page.\n"
		"    -verbose         View more runtime information.\n"
		"  Modes (If none selected, pass a single *.mca/*.mcc file to interpret):\n"
		"    -livecode        Live interpret MarC code from the console.\n"
		"    -liveasm         Live interpret MarCembly code from the console.\n"
		"    -execute         Execute a *.mce file.\n"
		"    -assemble        Assemble a *.mcc file.\n"
		"    -compile         Compile a *.mca file.\n"
		"    -link            Link a *.mco file.\n"
		"  File IO:\n"
		"    -i [filepath]    Input file.\n"
		"    -o [filepath]    Output file.\n"
		"    -m [directory]   Directory to search for modules in (Can be used multiple times).\n"
		"  Exit behavior: (Default: Keeps MarCmd open when the exit code is non-zero.)\n"
		"    -keeponexit      Keep MarCmd open after the execution has finished.\n"
		"    -closeonexit     Close MarCmd after the execution has finished.\n"
		;
}