#include "MarCmdLiveAsmInterpreter.h"

#include <iostream>

#include "MarCore.h"

namespace MarCmd
{
	int LiveAsmInterpreter::run(const std::set<std::string>& modDirs, Flags<CmdFlags> flags)
	{
		std::string codeStr;

		MarC::AsmTokenizer tokenizer(codeStr);
		MarC::Compiler compiler(tokenizer.getTokenList(), "<cin>");

		MarC::Linker linker;
		linker.addModule(compiler.getModuleInfo());
		MarC::Interpreter interpreter(linker.getExeInfo(), 4096);

		std::cout << "MarCmd Live MarCembly Interpreter" << std::endl;

		while (!interpreter.lastError())
		{
			std::cout << " >>> ";
			codeStr.append(readCodeFromConsole());

			// TODO: Check for errors

			tokenizer.tokenize();
			compiler.compile();

			// TODO: Load/add required modules

			linker.link();
			if (!interpreter.interpret() && interpreter.lastError().getCode() == MarC::IntErrCode::AbortViaEndOfCode)
				interpreter.resetError();
		}

		int64_t exitCode = interpreter.getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64;

		std::cout << "Module '<cin>' exited with code " << exitCode << "." << std::endl;

		if (flags.hasFlag(CmdFlags::Verbose))
			std::cout << "  Reason: '" << interpreter.lastError().getCodeStr() << "'" << std::endl;

		return interpreter.getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_32;
	}

	std::string LiveAsmInterpreter::readCodeFromConsole()
	{
		std::string line;
		std::getline(std::cin, line);
		line.push_back('\n');
		return line;
	}
}