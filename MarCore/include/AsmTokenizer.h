#pragma once

#include <string>

#include "types/AsmTokenizerTypes.h"
#include "errors/AsmTokenizerError.h"

namespace MarC
{
	class AsmTokenizer
	{
	public:
		AsmTokenizer(const std::string& asmCode);
	public:
		bool tokenize();
	public:
		AsmTokenListRef getTokenList();
	public:
		const AsmTokenizerError& lastError() const;
		void resetError();
	private:
		void backup();
	public:
		void recover();
	private:
		const std::string& m_asmCode;
		AsmTokenListRef m_pTokenList;
		AsmTokenizerError m_lastErr;
		uint64_t m_nextCharToTokenize = 0;
		struct BackupData
		{
			uint64_t tokenListSize = 0;
			uint64_t nextCharToTokenize = 0;
		} m_bud;
	};
}