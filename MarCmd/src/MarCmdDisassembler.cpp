#include "MarCmdDisassembler.h"

#include <fstream>
#include <MarCore.h>

namespace MarCmd
{
	int Disassembler::disassemble(const Settings& settings)
	{
		bool verbose = settings.flags.hasFlag(CmdFlags::Verbose);

		auto exeInfo = MarC::ExecutableInfo::create();
		{
			std::ifstream iStream(settings.inFile, std::ios::binary | std::ios::in);
			if (!iStream.is_open())
			{
				std::cout << "Unable to open input file!" << std::endl;
				return -1;
			}
			MarC::deserialize(*exeInfo, iStream);

			if (!iStream.good())
			{
				std::cout << "An error occured while reading from the input file!" << std::endl;
				return -1;
			}
		}

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

		for (uint64_t i = 0; i < exeInfo->modules.size(); ++i)
		{
			std::string source;

			auto& codeMem = *(exeInfo->modules[i]->codeMemory);

			uint64_t nDisassembled = 0;
			while (nDisassembled < codeMem.size())
			{
				auto daii = MarC::Disassembler::disassemble((char*)codeMem.getBaseAddress() + nDisassembled);
				
				source.append(MarC::DisAsmInsInfoToString(daii));

				nDisassembled += daii.rawData.size();
				
				if (nDisassembled < codeMem.size())
					source.push_back('\n');

			}

			std::string outFile = std::filesystem::path(outDir).append(exeInfo->modules[i]->moduleName).string() + ".mcd";
			std::ofstream oStream(outFile, std::ios::out | std::ios::trunc);
			if (!oStream.is_open())
			{
				std::cout << "Unable to open output file!" << std::endl;
				return -1;
			}
			oStream.write(source.c_str(), source.size());
		}

		
		return 0;
	}
}