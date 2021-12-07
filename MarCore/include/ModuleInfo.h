#pragma once

#include "ExecutableInfo.h"

namespace MarC
{
	class ModuleInfo;

	typedef std::shared_ptr<ModuleInfo> ModuleInfoRef;

	class ModuleInfo
	{
	public:
		ExecutableInfoRef exeInfo;
		std::set<SymbolAlias> symbolAliases;
		std::vector<SymbolRef> unresolvedSymbolRefs;
	public:
		ModuleInfo();
	public:
		void backup();
		void recover();
	public:
		static ModuleInfoRef create();
	private:
		struct BackupData
		{
			uint64_t symAliases = 0;
			uint64_t unresSymRefSize = 0;
		} bud;
	};
}