#include "MarCmdDisassembler.h"

#include <fstream>
#include <MarCore.h>

#include "AutoExecutableLoader.h"

namespace MarCmd
{
	int Disassembler::run(const Settings& settings)
	{
		bool verbose = settings.flags.hasFlag(CmdFlags::Verbose);

		auto exeInfo = autoLoadExecutable(settings.inFile, settings.modDirs);

		if (verbose)
			std::cout << "Creating the output directory..." << std::endl;
		std::string outDir;
		if (!settings.outFile.empty())
			outDir = settings.outFile;
		else
			outDir = std::filesystem::path(settings.inFile).replace_extension(".mcd").string();

		{
			std::error_code ec;
			std::filesystem::create_directories(outDir, ec);
			if (ec)
			{
				std::cout << "Unable to create the output directory!" << std::endl;
				return -1;
			}
		}

		{
			if (verbose)
				std::cout << "Disassembling executable..." << std::endl;

			std::string source;

			auto& codeMem = exeInfo->codeMemory;

			uint64_t nDisassembled = 0;
			while (nDisassembled < codeMem.size())
			{
				auto daii = MarC::Disassembler::disassemble((char*)codeMem.getBaseAddress() + nDisassembled);

				source.append(MarC::DisAsmInsInfoToString(daii, exeInfo->symbols));

				nDisassembled += daii.rawData.size();
				
				if (nDisassembled < codeMem.size())
					source.push_back('\n');

			}

			std::string outFile = std::filesystem::path(outDir).append(exeInfo->name).string() + ".mcd";
			std::ofstream oStream(outFile, std::ios::out | std::ios::trunc);
			if (!oStream.is_open())
			{
				std::cout << "Unable to open output file!" << std::endl;
				return -1;
			}
			oStream.write(source.c_str(), source.size());
		}

		std::cout << "Successfully disassembled the executable!" << std::endl;
		
		return 0;
	}
}