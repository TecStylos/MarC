#pragma once

#include <set>
#include <string>

#include "MarCore.h"

#include "MarCmdSettings.h"

namespace MarCmd
{
	class LiveAsmInterpreter
	{
	public:
		static int run(const Settings& settings);
	private:
		LiveAsmInterpreter(const Settings& settings);
	private:
		int run();
	private:
		enum class RecoverBegin
		{
			None,
			Tokenizer,
			Assembler,
			Linker,
			Interpreter,
		};
		void recover(RecoverBegin rs);
		std::string readCodeFromConsole();
		static std::string readFile(const std::string& filepath);
	private:
		Settings m_settings;
	private:
		uint64_t m_backupCodeStrSize = 0;
		std::string m_codeStr;
		std::shared_ptr<MarC::AsmTokenizer> m_pTokenizer;
		std::shared_ptr<MarC::Assembler> m_pAssembler;
		std::shared_ptr<MarC::Linker> m_pLinker;
		std::shared_ptr<MarC::Interpreter> m_pInterpreter;
	};
}