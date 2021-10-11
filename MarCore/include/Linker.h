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
		ExecutableInfoRef m_pExeInfo;
	};
}