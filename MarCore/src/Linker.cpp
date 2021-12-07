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

	Linker::Linker(ModuleInfoRef modInfo)
		: m_modInfo(modInfo)
	{}

	bool Linker::addModule(ModuleInfoRef pModInfo)
	{
		resetError();

		return !m_lastErr;
	}

	bool Linker::update()
	{
		resetError();

		return !m_lastErr;
	}

	bool Linker::link()
	{
		resetError();

		if (!update())
			return false;

		try
		{
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
		return m_modInfo->exeInfo;
	}

	void Linker::resolveUnresolvedSymbols()
	{
		uint64_t nResolved;

		do
		{
			nResolved = 0;

			auto it = m_modInfo->unresolvedSymbols.begin();

			while (it != m_modInfo->unresolvedSymbols.end())
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

				m_modInfo->unresolvedSymbols.erase(current);

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

			m_modInfo->exeInfo->codeMemory->write(&result->value, BC_DatatypeSize(ref.datatype), ref.offset);

			m_modInfo->unresolvedSymbolRefs.erase(m_modInfo->unresolvedSymbolRefs.begin() + i);
			--i;
		}

		if (!m_modInfo->unresolvedSymbolRefs.empty())
			resolvedAll = false;

		if (!resolvedAll)
			throw LinkerError(LinkErrCode::UnresolvedSymbols, "Cannot resolve all symbol references!");
	}

	bool Linker::symbolNameExists(const std::string& name)
	{
		if (m_modInfo->exeInfo->symbols.find(Symbol(name)) != m_modInfo->exeInfo->symbols.end())
			return true;
		if (m_modInfo->unresolvedSymbols.find(UnresolvedSymbol(name, "")) != m_modInfo->unresolvedSymbols.end())
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
