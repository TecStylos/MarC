#include "Linker.h"

namespace MarC
{
	Linker::Linker()
	{
		m_pExeInfo = std::make_shared<ExecutableInfo>();
	}

	bool Linker::addModule(ModuleInfoRef pModInfo)
	{
		m_pExeInfo->modules.push_back(pModInfo);
		return true;
	}

	bool Linker::link()
	{
		return true;
	}

	ExecutableInfoRef Linker::getExeInfo()
	{
		return m_pExeInfo;
	}
}