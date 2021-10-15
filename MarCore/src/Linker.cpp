#include "Linker.h"

namespace MarC
{
	Linker::Linker()
	{
		m_pExeInfo = std::make_shared<ExecutableInfo>();
	}

	bool Linker::addModule(ModuleInfoRef pModInfo)
	{
		if (hasModule(pModInfo->moduleName))
			return false;

		m_pExeInfo->modules.push_back(pModInfo);
		return true;
	}

	bool Linker::link()
	{
		for (uint64_t i = 0; i < m_pExeInfo->modules.size(); ++i)
		{
			auto& mod = *m_pExeInfo->modules[i];

			for (auto label : mod.labels)
			{
				if (label.second.usage == SYMBOL_USAGE_ADDRESS && label.second.value.as_ADDR.base == BC_MEM_BASE_CODE_MEMORY)
				//if (label.second.base == BC_MEM_BASE_CODE_MEMORY)
					label.second.value.as_ADDR.asCode.page = i;
				m_labels.insert(label);
			}
			mod.labels.clear();
		}

		bool resolvedAll = true;

		for (uint64_t i = 0; i < m_pExeInfo->modules.size(); ++i)
		{
			auto& mod = *m_pExeInfo->modules[i];

			for (uint64_t i = 0; i < mod.unresolvedRefs.size(); ++i)
			{
				auto& ref = mod.unresolvedRefs[i];
				auto result = m_labels.find(ref.name);
				if (result == m_labels.end())
					continue;

				mod.codeMemory->write(&result->second.value, BC_DatatypeSize(ref.datatype), ref.offset);

				mod.unresolvedRefs.erase(mod.unresolvedRefs.begin() + i);
				--i;
			}

			if (!mod.unresolvedRefs.empty())
				resolvedAll = false;
		}

		return resolvedAll;
	}

	ExecutableInfoRef Linker::getExeInfo()
	{
		return m_pExeInfo;
	}

	bool Linker::hasModule(const std::string& name)
	{
		for (auto& mod : m_pExeInfo->modules)
		{
			if (mod->moduleName == name)
				return true;
		}
		return false;
	}
}
