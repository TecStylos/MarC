#include "Linker.h"

namespace MarC
{
	LinkerError::operator bool() const
	{
		return m_code != Code::Success;
	}

	const std::string& LinkerError::getText() const
	{
		return m_errText;
	}

	std::string LinkerError::getMessage() const
	{
		return
			"ERROR\n  -> " + getText();
	}

	Linker::Linker()
	{
		m_pExeInfo = std::make_shared<ExecutableInfo>();
	}

	bool Linker::addModule(ModuleInfoRef pModInfo)
	{
		if (hasModule(pModInfo->moduleName))
			LINKER_RETURN_WITH_ERROR(LinkErrCode::ModuleAlreadyExisting, "Cannot add module '" + pModInfo->moduleName + "'! Module with same name already added!");

		m_missingModules.erase(pModInfo->moduleName);

		m_pExeInfo->moduleNameMap.insert({ pModInfo->moduleName, m_pExeInfo->modules.size() });
		m_pExeInfo->modules.push_back(pModInfo);

		if (!copySymbols(pModInfo))
			return false;

		copyReqMods(pModInfo);

		return true;
	}

	bool Linker::update()
	{
		if (!copySymbols())
			return false;

		copyReqMods();

		return true;
	}

	bool Linker::link()
	{
		resetError();

		if (!update())
			return false;

		if (hasMissingModules())
			LINKER_RETURN_WITH_ERROR(LinkErrCode::MissingModules, "Cannot link with missing modules! (" + misModListStr() + ")");

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

	bool Linker::copySymbols()
	{
		for (auto& mod : m_pExeInfo->modules)
			if (!copySymbols(mod))
				return false;

		return true;
	}

	bool Linker::copySymbols(ModuleInfoRef pModInfo)
	{
		for (auto& symbol : pModInfo->definedSymbols)
		{
			if (m_symbols.find(symbol) != m_symbols.end())
				LINKER_RETURN_WITH_ERROR(LinkErrCode::SymbolAlreadyDefined, "A symbol with name '" + symbol.name + "' has already been defined!");

			if (symbol.usage == SymbolUsage::Address &&
				(
					symbol.value.as_ADDR.base == BC_MEM_BASE_CODE_MEMORY ||
					symbol.value.as_ADDR.base == BC_MEM_BASE_STATIC_STACK
					)
				)
				symbol.value.as_ADDR.asCode.page = m_pExeInfo->moduleNameMap.find(pModInfo->moduleName)->second;
			m_symbols.insert(symbol);
		}
		pModInfo->definedSymbols.clear();

		return true;
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
			for (uint64_t i = 0; i < mod->unresolvedSymbolRefs.size(); ++i)
			{
				auto& ref = mod->unresolvedSymbolRefs[i];
				auto result = m_symbols.find(ref.name);
				if (result == m_symbols.end())
					continue;

				mod->codeMemory->write(&result->value, BC_DatatypeSize(ref.datatype), ref.offset);

				mod->unresolvedSymbolRefs.erase(mod->unresolvedSymbolRefs.begin() + i);
				--i;
			}

			if (!mod->unresolvedSymbolRefs.empty())
				resolvedAll = false;
		}

		if (!resolvedAll)
			LINKER_RETURN_WITH_ERROR(LinkErrCode::UnresolvedSymbols, "Cannot resolve all symbols references! (" + unresSymRefsListStr() + ")")

		return true;
	}

	std::string Linker::misModListStr() const
	{
		std::string out;
		uint64_t count = 0;
		for (auto& elem : m_missingModules)
		{
			out.append("'" + elem + "'");
			if (++count < m_missingModules.size())
				out.append(", ");
		}
		return out;
	}

	std::string Linker::unresSymRefsListStr() const
	{
		std::string out;
		uint64_t modCount = 0;
		for (auto& mod : m_pExeInfo->modules)
		{
			uint64_t symCount = 0;
			for (auto& sym : mod->unresolvedSymbolRefs)
			{
				out.append("'" + sym.name + "'");
				if (++symCount < mod->unresolvedSymbolRefs.size())
					out.append(", ");
			}
			if (++modCount < m_pExeInfo->modules.size())
				out.append(", ");
		}
		return out;
	}

	const LinkerError& Linker::lastError() const
	{
		return m_lastErr;
	}

	void Linker::resetError()
	{
		m_lastErr = LinkerError();
	}
}
