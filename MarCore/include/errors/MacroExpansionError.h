#pragma once

#include "MarCoreError.h"

namespace MarC
{
	class MacroExpansionError : public MarCoreError
	{
	public:
		MacroExpansionError()
			: MarCoreError("MacroExpansionError"), m_isSuccess(true)
		{}
		MacroExpansionError(const MarCoreError& other, uint64_t line)
			: MarCoreError("MacroExpansionError"), m_isSuccess(false)
		{
			m_whatBuff = "(" + std::to_string(line) + "): " + "MacroExpansionError:\n";

			m_whatBuff.append(other.what());
		}
	public:
		virtual explicit operator bool() const override { return !m_isSuccess; }
	private:
		bool m_isSuccess;
	};

	#define MARC_MACRO_EXPANSION_THROW(origErr) throw MacroExpansionError(origErr, currToken().line)
}