#include "AsmTokenizer.h"

namespace MarC
{
	AsmTokenizer::AsmTokenizer(const std::string& asmCode)
		: m_asmCode(asmCode)
	{
		m_pTokenList = std::make_shared<AsmTokenList>();
	}

	bool AsmTokenizer::tokenize()
	{
		resetError();

		enum class CurrAction
		{
			BeginToken,
			EndToken,
			TokenizeString,
			TokenizePragmaInsertion,
			TokenizeStringEscapeCode,
			TokenizeComment,
			TokenizeInteger,
			TokenizeFloat,
			TokenizeName,
			TokenizeChar,
			TokenizeCharEscapeCode,
			TokenizeCharDelim,
		} currAction = CurrAction::BeginToken;

		uint16_t line = 1;
		uint64_t lineBegin = 0;

		AsmToken currToken(line, 1);

		auto tokenizeEscapeCode = [&](char c) {
			switch (c)
			{
			case 'a':
				currToken.value.push_back('\a');
				break;
			case 'b':
				currToken.value.push_back('\b');
				break;
			case 't':
				currToken.value.push_back('\t');
				break;
			case 'n':
				currToken.value.push_back('\n');
				break;
			case 'v':
				currToken.value.push_back('\v');
				break;
			case 'f':
				currToken.value.push_back('\f');
				break;
			case 'r':
				currToken.value.push_back('\r');
				break;
			case 'e':
				currToken.value.push_back('\033');
				break;
			case '\'':
				currToken.value.push_back(c);
				break;
			case '\"':
				currToken.value.push_back(c);
				break;
			case '\\':
				currToken.value.push_back(c);
				break;
			default:
				ASM_TOKENIZER_THROW_ERROR(AsmTokErrCode::UnexpectedChar, std::string("Unexpected char '") + c + "' for escape sequence!");
			}
		};

		if (m_pTokenList->size() > 0 && (*m_pTokenList)[m_pTokenList->size() - 1].type == AsmToken::Type::END_OF_CODE)
			m_pTokenList->pop_back();

		backup();

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
					case '\t':
					case '\0':
						currAction = CurrAction::EndToken;
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
						++line;
						lineBegin = nextChar;
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
					case '^':
						currToken.type = AsmToken::Type::Op_DT_Size;
						currToken.value = "^";
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
					case '\\':
						currToken.type = AsmToken::Type::Op_Ignore_Directive;
						currToken.value = "\\";
						currAction = CurrAction::EndToken;
						break;
					case '"':
						currToken.type = AsmToken::Type::String;
						currAction = CurrAction::TokenizeString;
						break;
					case '%':
						currToken.type = AsmToken::Type::Pragma_Insertion;
						currAction = CurrAction::TokenizePragmaInsertion;
						break;
					case '\'':
						currToken.type = AsmToken::Type::Integer;
						currAction = CurrAction::TokenizeChar;
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
						else if (std::isalpha(c) || c == '>' || c == '_')
						{
							currToken.type = AsmToken::Type::Name;
							currToken.value.push_back(c);
							currAction = CurrAction::TokenizeName;
						}
						else
						{
							ASM_TOKENIZER_THROW_ERROR(AsmTokErrCode::UnexpectedChar, std::string("Unexpected char '") + c + "' while beginning a token!");
						}
					}
					break;
				case CurrAction::TokenizeString:
					switch (c)
					{
					case '"':
						currAction = CurrAction::EndToken;
						break;
					case '\\':
						currAction = CurrAction::TokenizeStringEscapeCode;
						break;
					default:
						currToken.value.push_back(c);
					}
					break;
				case CurrAction::TokenizePragmaInsertion:
					if (!std::isdigit(c))
					{
						if (currToken.value.empty())
							ASM_TOKENIZER_THROW_ERROR(AsmTokErrCode::UnexpectedChar, "Expected integer after pragma insertion sign!");
						currAction = CurrAction::EndToken;
						--nextChar;
					}
					else
					{
						currToken.value.push_back(c);
					}
					break;
				case CurrAction::TokenizeStringEscapeCode:
					tokenizeEscapeCode(c);
					currAction = CurrAction::TokenizeString;
					break;
				case CurrAction::TokenizeChar:
					switch (c)
					{
					case '\'':
						ASM_TOKENIZER_THROW_ERROR(AsmTokErrCode::UnexpectedChar, "Missing character in char literal!");
					case '\\':
						currAction = CurrAction::TokenizeCharEscapeCode;
						break;
					default:
						currToken.value.push_back(c);
						currAction = CurrAction::TokenizeCharDelim;
					}
					break;
				case CurrAction::TokenizeCharEscapeCode:
					tokenizeEscapeCode(c);
					currAction = CurrAction::TokenizeCharDelim;
					break;
				case CurrAction::TokenizeCharDelim:
					if (c != '\'')
						ASM_TOKENIZER_THROW_ERROR(AsmTokErrCode::UnexpectedChar, "Expected delimiter for char literal!");
					currToken.value = std::to_string((int)currToken.value[0]);
					currAction = CurrAction::EndToken;
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
					if (std::isalnum(c) || c == '_' || c == '>')
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
					if (
						currToken.type != AsmToken::Type::None &&
						currToken.type != AsmToken::Type::Comment
						)
					{
						m_pTokenList->push_back(currToken);
					}
					currToken = AsmToken(line, uint16_t(nextChar - lineBegin));
					currAction = CurrAction::BeginToken;
				}
			}
		}
		catch (const AsmTokenizerError& ate)
		{
			m_lastErr = ate;
			recover();
			return false;
		}

		m_nextCharToTokenize = m_asmCode.size();

		m_pTokenList->push_back({ uint16_t(line + 1u), 1u, AsmToken::Type::END_OF_CODE, "<END_OF_CODE>" });

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

	void AsmTokenizer::resetError()
	{
		m_lastErr = AsmTokenizerError();
	}

	void AsmTokenizer::backup()
	{
		m_bud.tokenListSize = m_pTokenList->size();
		m_bud.nextCharToTokenize = m_nextCharToTokenize;
	}

	void AsmTokenizer::recover()
	{
		m_pTokenList->resize(m_bud.tokenListSize);
		m_nextCharToTokenize = m_bud.nextCharToTokenize;
	}
}
