#pragma once

#include "LinkerOutput.h"

namespace MarC
{
	class Linker
	{
	public:
		Linker();
	public:
		bool addModule(ModuleInfoRef pModInfo);
		bool link();
	public:
		ExecutableInfoRef getExeInfo();
	private:
		bool hasModule(const std::string& name);
	private:
		ExecutableInfoRef m_pExeInfo;
		SymbolMap m_labels;
	};
}