#pragma once

#include "Compiler.h"

namespace MarC
{
	class VirtualAsmTokenList
	{
	public:
		VirtualAsmTokenList(Compiler& comp, AsmTokenListRef tokens);
		~VirtualAsmTokenList();
	private:
		Compiler& m_comp;
	private:
		AsmTokenListRef m_pTokenListBackup;
		uint64_t m_nextTokenToCompileBackup;
	};
}