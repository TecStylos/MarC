#include "AutoExecutableLoader.h"

namespace MarCmd
{
	MarC::ExecutableInfoRef autoLoadExecutable(const std::string& inFile, const std::set<std::string>& modDirs)
	{
		MarC::ExecutableInfoRef exeInfo = nullptr;
		auto extension = MarC::modExtFromPath(inFile);
		if (extension == ".mcc")
		{
			throw MarC::MarCoreError("FileFormatError", "MarC code files are not supported yet!");
		}
		else if (extension == ".mce")
		{
			exeInfo = MarC::ExecutableLoader::load(inFile);
		}
		else if (extension == ".mca")
		{
			auto mod = MarC::ModuleLoader::load(inFile, modDirs);

			MarC::Assembler assembler(mod);
			if (!assembler.assemble())
				throw assembler.lastError();

			MarC::Linker linker(assembler.getModuleInfo());
			if (!linker.link())
				throw linker.lastError();

			exeInfo = linker.getExeInfo();
		}
		else
		{
			throw MarC::MarCoreError("FileFormatError", "Unknown input file format!");
		}
		return exeInfo;
	}
}