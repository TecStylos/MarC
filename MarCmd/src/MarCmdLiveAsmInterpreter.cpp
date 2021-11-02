#include "MarCmdLiveAsmInterpreter.h"

#include <iostream>

namespace MarCmd
{
	int LiveAsmInterpreter::run(const std::set<std::string>& modDirs, Flags<CmdFlags> flags)
	{
		LiveAsmInterpreter lai(modDirs, flags);
		return lai.run();
	}

	LiveAsmInterpreter::LiveAsmInterpreter(const std::set<std::string>& modDirs, Flags<CmdFlags> flags)
		: m_modDirs(modDirs), m_flags(flags)
	{
		m_codeStr = "";
		m_pTokenizer = std::make_shared<MarC::AsmTokenizer>(m_codeStr);
		m_pCompiler = std::make_shared<MarC::Compiler>(m_pTokenizer->getTokenList(), "<cin>");
		m_pLinker = std::make_shared<MarC::Linker>();
		m_pInterpreter = std::make_shared<MarC::Interpreter>(m_pLinker->getExeInfo(), 4096);
	
		m_pLinker->addModule(m_pCompiler->getModuleInfo());
	}

	int LiveAsmInterpreter::run()
	{
		std::cout << "MarCmd Live MarCembly Interpreter" << std::endl;

		while (!m_pInterpreter->lastError())
		{
			std::cout << " >>> ";
			m_backupCodeStrSize = m_codeStr.size();
			m_codeStr.append(readCodeFromConsole());

			if (!m_pTokenizer->tokenize())
			{
				recover(RecoverBegin::Tokenizer);
				std::cout << "An error occured while running the tokenizer!" << std::endl
					<< "  " << m_pTokenizer->lastError().getMessage() << std::endl;
				continue;
			}

			if (!m_pCompiler->compile())
			{
				recover(RecoverBegin::Compiler);
				std::cout << "An error occured while running the compiler!:" << std::endl
					<< "  " << m_pCompiler->lastError().getMessage() << std::endl;
				continue;
			}

			// TODO: Load/add required modules

			if (!m_pLinker->link())
			{
				recover(RecoverBegin::Linker);
				std::cout << "An error occured while running the linker!" << std::endl
					<< "  " << m_pLinker->lastError().getMessage() << std::endl;
				continue;
			}

			if (!m_pInterpreter->interpret())
			{
				if (m_pInterpreter->lastError().getCode() == MarC::IntErrCode::AbortViaEndOfCode)
				{
					m_pInterpreter->resetError();
				}
				if (!m_pInterpreter->lastError().isOK())
				{
					recover(RecoverBegin::Interpreter);
					std::cout << std::endl << "An error occured while interpreting the code!" << std::endl
						<< "    " << m_pInterpreter->lastError().getMessage() << std::endl;
				}
			}
		}

		int64_t exitCode = m_pInterpreter->getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_64;

		std::cout << "Module '<cin>' exited with code " << exitCode << "." << std::endl;

		if (m_flags.hasFlag(CmdFlags::Verbose))
			std::cout << "  Reason: '" << m_pInterpreter->lastError().getCodeStr() << "'" << std::endl;

		return m_pInterpreter->getRegister(MarC::BC_MEM_REG_EXIT_CODE).as_I_32;
	}

	void LiveAsmInterpreter::recover(RecoverBegin rs)
	{
		switch (rs)
		{
		case RecoverBegin::Interpreter:
			std::cout << "CANNOT RECOVER FROM INTERPRETER ERRORS! IT IS RECOMMENDED RESTARTING THE PROGRAM." << std::endl;
			//__fallthrough
		case RecoverBegin::Linker:
			std::cout << "CANNOT RECOVER FROM LINKER ERRORS! IT IS RECOMMENDED RESTARTING THE PROGRAM." << std::endl;
			m_pCompiler->recover();
			//__fallthrough
		case RecoverBegin::Compiler:
			m_pTokenizer->recover();
			//__fallthrough
		case RecoverBegin::Tokenizer: 
			m_codeStr.resize(m_backupCodeStrSize);
			//__fallthrough
		case RecoverBegin::None:
			break;
		}
	}

	std::string LiveAsmInterpreter::readCodeFromConsole()
	{
		std::string line;
		std::getline(std::cin, line);
		if (line != "%")
			return line + '\n';


		std::cout << "     ";
		std::getline(std::cin, line);
		std::string code;
		do
		{
			code.append(line);
			code.push_back('\n');
			std::cout << "     ";
			std::getline(std::cin, line);
		} while (line != "%");

		return code;
	}
}