#include "VirtualAsmTokenList.h"

namespace MarC
{
	VirtualAsmTokenList::VirtualAsmTokenList(Compiler& comp, AsmTokenListRef tokens)
		: m_comp(comp)
	{
		m_pTokenListBackup = m_comp.m_pTokenList;
		m_nextTokenToCompileBackup = m_comp.m_nextTokenToCompile;

		m_comp.m_pTokenList = tokens;
		m_comp.m_nextTokenToCompile = 0;
	}

	VirtualAsmTokenList::~VirtualAsmTokenList()
	{
		m_comp.m_pTokenList = m_pTokenListBackup;
		m_comp.m_nextTokenToCompile = m_nextTokenToCompileBackup;
	}
}