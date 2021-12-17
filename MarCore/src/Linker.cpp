#include "Linker.h"

#include "fileio/ModuleLocator.h"

namespace MarC
{
	Linker::Linker(ModuleInfoRef modInfo)
		: m_modInfo(modInfo)
	{}

	bool Linker::link()
	{
		resetError();

		try
		{
			resolveSymbolAliases();
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
		return m_modInfo->exeInfo;
	}

	void Linker::resolveSymbolAliases()
	{
		uint64_t nResolved;

		do
		{
			nResolved = 0;

			auto it = m_modInfo->symbolAliases.begin();

			while (it != m_modInfo->symbolAliases.end())
			{
				auto current = it++;

				auto itRef = m_modInfo->exeInfo->symbols.find(current->refName);
				if (itRef == m_modInfo->exeInfo->symbols.end())
					continue;

				Symbol sym;
				sym.name = current->name;
				sym.usage = itRef->usage;
				sym.value = itRef->value;

				m_modInfo->exeInfo->symbols.insert(sym);

				m_modInfo->symbolAliases.erase(current);

				++nResolved;
			}
		}
		while (nResolved != 0);
	}

	void Linker::resolveUnresolvedSymbolRefs()
	{
		bool resolvedAll = true;

		for (uint64_t i = 0; i < m_modInfo->unresolvedSymbolRefs.size(); ++i)
		{
			auto& ref = m_modInfo->unresolvedSymbolRefs[i];
			auto result = m_modInfo->exeInfo->symbols.find(ref.name);
			if (result == m_modInfo->exeInfo->symbols.end())
				continue;

			m_modInfo->exeInfo->codeMemory.write(&result->value, BC_DatatypeSize(ref.datatype), ref.offset);

			m_modInfo->unresolvedSymbolRefs.erase(m_modInfo->unresolvedSymbolRefs.begin() + i);
			--i;
		}

		if (!m_modInfo->unresolvedSymbolRefs.empty())
			resolvedAll = false;

		if (!resolvedAll)
		{
			std::string unresString;
			for (uint64_t i = 0; i < m_modInfo->unresolvedSymbolRefs.size(); ++i)
			{
				unresString.append(m_modInfo->unresolvedSymbolRefs[i].name);
				if (i + 1 < m_modInfo->unresolvedSymbolRefs.size())
					unresString.append(", ");
			}
			throw LinkerError(LinkErrCode::UnresolvedSymbols, unresString);
		}
	}

	bool Linker::symbolNameExists(const std::string& name)
	{
		if (m_modInfo->exeInfo->symbols.find(Symbol(name)) != m_modInfo->exeInfo->symbols.end())
			return true;
		if (m_modInfo->symbolAliases.find(SymbolAlias(name, "")) != m_modInfo->symbolAliases.end())
			return true;
		return false;
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
