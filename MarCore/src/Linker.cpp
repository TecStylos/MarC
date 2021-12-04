#include "Linker.h"

#include "ModuleLocator.h"

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

	Linker::Linker(std::set<std::string> modDirs)
		: m_modDirs(modDirs)
	{
		m_pExeInfo = ExecutableInfo::create();
	}

	bool Linker::addModule(ModuleInfoRef pModInfo)
	{
		resetError();

		try
		{
			if (hasModule(pModInfo->moduleName))
				throw LinkerError(LinkErrCode::ModuleAlreadyExisting, "Cannot add module '" + pModInfo->moduleName + "'! Module with same name already added!");

			m_missingModules.erase(pModInfo->moduleName);

			m_pExeInfo->moduleNameMap.insert({ pModInfo->moduleName, m_pExeInfo->modules.size() });
			m_pExeInfo->modules.push_back(pModInfo);

			copyReqMods(pModInfo);

			copyUnresolvedSymbols(pModInfo);
			copySymbols(pModInfo);
		}
		catch (const LinkerError& err)
		{
			m_lastErr = err;
		}

		return !m_lastErr;
	}

	bool Linker::update()
	{
		resetError();

		try
		{
			for (auto& pModInfo : m_pExeInfo->modules)
				update(pModInfo);
		}
		catch (const LinkerError& err)
		{
			m_lastErr = err;
		}

		return !m_lastErr;
	}

	bool Linker::link()
	{
		resetError();

		if (!update())
			return false;

		try
		{
			if (hasMissingModules())
				throw LinkerError(LinkErrCode::MissingModules, "Cannot link with missing modules! (" + misModListStr() + ")");

			resolveUnresolvedSymbols();
			resolveUnresolvedSymbolRefs();
		}
		catch (LinkerError& err)
		{
			m_lastErr = err;
		}

		return !m_lastErr;
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

	void Linker::copySymbols(ModuleInfoRef pModInfo)
	{
		for (auto& symbol : pModInfo->definedSymbols)
		{
			if (symbolNameExists(symbol.name))
				throw LinkerError(LinkErrCode::SymbolAlreadyDefined, "A symbol with name '" + symbol.name + "' has already been defined!");

			if (symbol.usage == SymbolUsage::Address &&
				(
					symbol.value.as_ADDR.base == BC_MEM_BASE_CODE_MEMORY ||
					symbol.value.as_ADDR.base == BC_MEM_BASE_STATIC_STACK
					)
				)
				symbol.value.as_ADDR.asCode.page = m_pExeInfo->moduleNameMap.find(pModInfo->moduleName)->second;
			m_pExeInfo->symbols.insert(symbol);
		}
		pModInfo->definedSymbols.clear();
	}

	void Linker::copyUnresolvedSymbols(ModuleInfoRef pModInfo)
	{
		for (auto& unresSym : pModInfo->unresolvedSymbols)
		{
			if (symbolNameExists(unresSym.name))
				throw LinkerError(LinkErrCode::SymbolAlreadyDefined, "A symbol with name '" + unresSym.name + "' has already been defined!");
			m_pExeInfo->unresolvedSymbols.insert(unresSym);
		}
		pModInfo->unresolvedSymbols.clear();
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

	void Linker::copyPerms(ModuleInfoRef pModInfo)
	{
		for (auto& manperm : pModInfo->mandatoryPermissions)
		{
			m_pExeInfo->mandatoryPermissions.insert(manperm);
			if (m_pExeInfo->optionalPermissions.find(manperm) != m_pExeInfo->optionalPermissions.end())
				m_pExeInfo->optionalPermissions.erase(manperm);
		}
		pModInfo->mandatoryPermissions.clear();
		for (auto& optperm : pModInfo->optionalPermissions)
		{
			if (m_pExeInfo->mandatoryPermissions.find(optperm) == m_pExeInfo->mandatoryPermissions.end())
				m_pExeInfo->optionalPermissions.insert(optperm);
		}
		pModInfo->optionalPermissions.clear();
	}

	void Linker::update(ModuleInfoRef pModInfo)
	{
		copySymbols(pModInfo);
		copyReqMods(pModInfo);
		copyPerms(pModInfo);
	}

	const std::set<std::string> Linker::getModDirs() const
	{
		return m_modDirs;
	}

	bool Linker::autoAddMissingModules(AddModuleCallback amc, void* pParam)
	{
		try
		{
			while (hasMissingModules())
			{
				auto& misMods = getMissingModules();
				auto modPaths = MarC::locateModules(m_modDirs, misMods);

				for (auto& pair : modPaths)
				{
					if (pair.second.empty())
						throw LinkerError(LinkErrCode::ModuleNotFound, "Unable to find module '" + pair.first + "'!");

					if (pair.second.size() > 1)
					{
						std::string errStr = "Module name '" + pair.first + "' is ambigious! Found " + std::to_string(pair.second.size()) + " matching modules!\n";
						for (auto& p : pair.second)
							errStr.append("  " + p + "\n");
						throw LinkerError(LinkErrCode::AmbigiousModule, errStr);
					}
					amc(*this, *pair.second.begin(), pair.first, pParam);
				}
			}
		}
		catch (const LinkerError& err)
		{
			m_lastErr = err;
		}

		return !m_lastErr;
	}

	void Linker::resolveUnresolvedSymbols()
	{
		uint64_t nResolved;

		do
		{
			nResolved = 0;

			auto it = m_pExeInfo->unresolvedSymbols.begin();

			while (it != m_pExeInfo->unresolvedSymbols.end())
			{
				auto current = it++;

				auto itRef = m_pExeInfo->symbols.find(current->refName);
				if (itRef == m_pExeInfo->symbols.end())
					continue;

				Symbol sym;
				sym.name = current->name;
				sym.usage = itRef->usage;
				sym.value = itRef->value;

				m_pExeInfo->symbols.insert(sym);

				m_pExeInfo->unresolvedSymbols.erase(current);

				++nResolved;
			}
		}
		while (nResolved != 0);
	}

	void Linker::resolveUnresolvedSymbolRefs()
	{
		bool resolvedAll = true;

		for (auto& mod : m_pExeInfo->modules)
		{
			for (uint64_t i = 0; i < mod->unresolvedSymbolRefs.size(); ++i)
			{
				auto& ref = mod->unresolvedSymbolRefs[i];
				auto result = m_pExeInfo->symbols.find(ref.name);
				if (result == m_pExeInfo->symbols.end())
					continue;

				mod->codeMemory->write(&result->value, BC_DatatypeSize(ref.datatype), ref.offset);

				mod->unresolvedSymbolRefs.erase(mod->unresolvedSymbolRefs.begin() + i);
				--i;
			}

			if (!mod->unresolvedSymbolRefs.empty())
				resolvedAll = false;
		}

		if (!resolvedAll)
			throw LinkerError(LinkErrCode::UnresolvedSymbols, "Cannot resolve all symbol references! (" + unresSymRefsListStr() + ")");
	}

	bool Linker::symbolNameExists(const std::string& name)
	{
		if (m_pExeInfo->symbols.find(Symbol(name)) != m_pExeInfo->symbols.end())
			return true;
		if (m_pExeInfo->unresolvedSymbols.find(UnresolvedSymbol(name, "")) != m_pExeInfo->unresolvedSymbols.end())
			return true;
		return false;
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
