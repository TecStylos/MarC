#pragma once

#include "Assembler.h"

namespace MarC
{
	class VirtualAsmTokenList
	{
	public:
		VirtualAsmTokenList(Assembler& comp, AsmTokenListRef tokens);
		~VirtualAsmTokenList();
	private:
		Assembler& m_comp;
	private:
		AsmTokenListRef m_pTokenListBackup;
		uint64_t m_nextTokenToCompileBackup;
	};
}