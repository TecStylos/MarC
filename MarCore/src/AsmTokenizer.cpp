#include "AsmTokenizer.h"

namespace MarC
{
	AsmTokenizerError::operator bool() const
	{
		return m_code != Code::Success;
	}

	const std::string& AsmTokenizerError::getText() const
	{
		return m_errText;
	}

	std::string AsmTokenizerError::getMessage() const
	{
		return "Error on line " + std::to_string(m_errLine) + ": " + getText() + "\n"
			+ "SYSFILE: " + m_sysErrFile + "  SYSLINE: " + std::to_string(m_sysErrLine);
	}

	AsmTokenizer::AsmTokenizer(const std::string& asmCode)
		: m_asmCode(asmCode)
	{
		m_pTokenList = std::make_shared<AsmTokenList>();
	}

	bool AsmTokenizer::tokenize()
	{
		enum class CurrAction
		{
			BeginToken,
			EndToken,
			TokenizeString,
			TokenizeComment,
			TokenizeInteger,
			TokenizeFloat,
			TokenizeName,
		} currAction = CurrAction::BeginToken;

		AsmToken currToken;

		if (m_pTokenList->size() > 0 && (*m_pTokenList)[m_pTokenList->size() - 1].type == AsmToken::Type::END_OF_CODE)
			m_pTokenList->pop_back();

		try
		{
			for (uint64_t nextChar = m_nextCharToTokenize; nextChar <= m_asmCode.size(); ++nextChar)
			{
				char c = m_asmCode[nextChar];

				switch (currAction)
				{
				case CurrAction::BeginToken:
					switch (c)
					{
					case ' ':
						break;
					case '\t':
						break;
					case '\0':
						break;
					case '.':
						currToken.type = AsmToken::Type::Sep_Dot;
						currToken.value = ".";
						currAction = CurrAction::EndToken;
						break;
					case ':':
						currToken.type = AsmToken::Type::Sep_Colon;
						currToken.value = ":";
						currAction = CurrAction::EndToken;
						break;
					case '\n':
						currToken.type = AsmToken::Type::Sep_Newline;
						currToken.value = "\n";
						currAction = CurrAction::EndToken;
						break;
					case '@':
						currToken.type = AsmToken::Type::Op_Deref;
						currToken.value = "@";
						currAction = CurrAction::EndToken;
						break;
					case '~':
						currToken.type = AsmToken::Type::Op_FP_Relative;
						currToken.value = "~";
						currAction = CurrAction::EndToken;
						break;
					case '$':
						currToken.type = AsmToken::Type::Op_Register;
						currToken.value = "$";
						currAction = CurrAction::EndToken;
						break;
					case '#':
						currToken.type = AsmToken::Type::Op_Directive;
						currToken.value = "#";
						currAction = CurrAction::EndToken;
						break;
					case '"':
						currToken.type = AsmToken::Type::String;
						currAction = CurrAction::TokenizeString;
						break;
					case '/':
						currToken.type = AsmToken::Type::Comment;
						currAction = CurrAction::TokenizeComment;
						break;
					default:
						if (std::isdigit(c) || c == '+' || c == '-')
						{
							currToken.type = AsmToken::Type::Integer;
							currToken.value.push_back(c);
							currAction = CurrAction::TokenizeInteger;
						}
						else if (std::isalpha(c))
						{
							currToken.type = AsmToken::Type::Name;
							currToken.value.push_back(c);
							currAction = CurrAction::TokenizeName;
						}
					}
					break;
				case CurrAction::TokenizeString:
					if (c == '"')
					{
						currAction = CurrAction::EndToken;
					}
					else
					{
						currToken.value.push_back(c);
					}
					break;
				case CurrAction::TokenizeComment:
					if (c == '\n')
					{
						currAction = CurrAction::EndToken;
						--nextChar;
					}
					else
					{
						currToken.value.push_back(c);
					}
					break;
				case CurrAction::TokenizeInteger:
					if (std::isdigit(c))
					{
						currToken.value.push_back(c);
					}
					else if (c == '.')
					{
						currToken.type = AsmToken::Type::Float;
						currToken.value.push_back(c);
						currAction = CurrAction::TokenizeFloat;
					}
					else if (c == ':' || c == ' ' || c == '\t' || c == '\n' || c == '\0')
					{
						currAction = CurrAction::EndToken;
						--nextChar;
					}
					else
					{
						ASM_TOKENIZER_THROW_ERROR(AsmTokErrCode::UnexpectedChar, std::string("Unexpected char '") + c + "' while tokenizing an integer!");
					}
					break;
				case CurrAction::TokenizeFloat:
					if (std::isdigit(c))
					{
						currToken.value.push_back(c);
					}
					else if (c == ':' || c == ' ' || c == '\t' || c == '\n' || c == '\0')
					{
						currAction = CurrAction::EndToken;
						--nextChar;
					}
					else
					{
						ASM_TOKENIZER_THROW_ERROR(AsmTokErrCode::UnexpectedChar, std::string("Unexpected char '") + c + "' while tokenizing a float!");
					}
					break;
				case CurrAction::TokenizeName:
					if (std::isalnum(c))
					{
						currToken.value.push_back(c);
					}
					else if (c == ':' || c == ' ' || c == '\t' || c == '\n' || c == '\0' || c == '.')
					{
						currAction = CurrAction::EndToken;
						--nextChar;
					}
					else
					{
						ASM_TOKENIZER_THROW_ERROR(AsmTokErrCode::UnexpectedChar, std::string("Unexpected char '") + c + "' while tokenizing a name!");
					}
					break;
				default:
					break;
				}

				if (currAction == CurrAction::EndToken)
				{
					if (currToken.type != AsmToken::Type::Comment)
						m_pTokenList->push_back(currToken);
					currToken = AsmToken();
					currAction = CurrAction::BeginToken;
				}
			}
		}
		catch (const AsmTokenizerError& ate)
		{
			m_lastErr = ate;
			return false;
		}

		m_nextCharToTokenize = m_asmCode.size();

		m_pTokenList->push_back(AsmToken(AsmToken::Type::END_OF_CODE, "<END_OF_CODE>"));

		return true;
	}

	AsmTokenListRef AsmTokenizer::getTokenList()
	{
		return m_pTokenList;
	}

	const AsmTokenizerError& AsmTokenizer::lastError() const
	{
		return m_lastErr;
	}
}