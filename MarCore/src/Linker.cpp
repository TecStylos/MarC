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

		m_missingModules.erase(pModInfo->moduleName);

		m_pExeInfo->moduleNameMap.insert({ pModInfo->moduleName, m_pExeInfo->modules.size() });
		m_pExeInfo->modules.push_back(pModInfo);

		copySymbols(pModInfo);
		copyReqMods(pModInfo);
		return true;
	}

	void Linker::update()
	{
		copySymbols();
		copyReqMods();
	}

	bool Linker::link()
	{
		update();

		if (hasMissingModules())
			return false; // Cannot link with missing modules

		if (!resolveSymbols())
			return false;

		return true;
	}

	ExecutableInfoRef Linker::getExeInfo()
	{
		return m_pExeInfo;
	}

	bool Linker::hasModule(const std::string& name) const
	{
		return m_pExeInfo->moduleNameMap.find(name) != m_pExeInfo->moduleNameMap.end();
	}

	bool Linker::hasMissingModules() const
	{
		return !m_missingModules.empty();
	}

	const std::set<std::string>& Linker::getMissingModules() const
	{
		return m_missingModules;
	}

	void Linker::copySymbols()
	{
		for (auto& mod : m_pExeInfo->modules)
			copySymbols(mod);
	}

	void Linker::copySymbols(ModuleInfoRef pModInfo)
	{
		for (auto& symbol : pModInfo->symbols)
		{
			if (symbol.second.usage == SymbolUsage::Address &&
				(
					symbol.second.value.as_ADDR.base == BC_MEM_BASE_CODE_MEMORY ||
					symbol.second.value.as_ADDR.base == BC_MEM_BASE_STATIC_STACK
					)
				)
				symbol.second.value.as_ADDR.asCode.page = m_pExeInfo->moduleNameMap.find(pModInfo->moduleName)->second;
			m_symbols.insert(symbol);
		}
		pModInfo->symbols.clear();
	}

	void Linker::copyReqMods()
	{
		for (auto& mod : m_pExeInfo->modules)
			copyReqMods(mod);
	}

	void Linker::copyReqMods(ModuleInfoRef pModInfo)
	{
		for (auto& reqmod : pModInfo->requiredModules)
		{
			if (!hasModule(reqmod))
				m_missingModules.insert(reqmod);
		}
		pModInfo->requiredModules.clear();
	}

	bool Linker::resolveSymbols()
	{
		bool resolvedAll = true;

		for (auto& mod : m_pExeInfo->modules)
		{
			for (uint64_t i = 0; i < mod->unresolvedRefs.size(); ++i)
			{
				auto& ref = mod->unresolvedRefs[i];
				auto& result = m_symbols.find(ref.name);
				if (result == m_symbols.end())
					continue;

				mod->codeMemory->write(&result->second.value, BC_DatatypeSize(ref.datatype), ref.offset);

				mod->unresolvedRefs.erase(mod->unresolvedRefs.begin() + i);
				--i;
			}

			if (!mod->unresolvedRefs.empty())
				resolvedAll = false;
		}

		return resolvedAll;
	}
}
