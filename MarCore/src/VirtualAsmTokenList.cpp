#include "VirtualAsmTokenList.h"

namespace MarC
{
	VirtualAsmTokenList::VirtualAsmTokenList(Assembler& comp, AsmTokenListRef tokens)
		: m_comp(comp)
	{
		m_pTokenListBackup = m_comp.m_pCurrTokenList;
		m_nextTokenToCompileBackup = m_comp.m_nextTokenToCompile;

		m_comp.m_pCurrTokenList = tokens;
		m_comp.m_nextTokenToCompile = 0;
	}

	VirtualAsmTokenList::~VirtualAsmTokenList()
	{
		m_comp.m_pCurrTokenList = m_pTokenListBackup;
		m_comp.m_nextTokenToCompile = m_nextTokenToCompileBackup;
	}
}