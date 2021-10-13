#include "Assembler.h"

#include <vector>
#include <string>
#include <stdexcept>

namespace MarC
{
	AssemblerError::operator bool() const
	{
		return m_code != Code::Success;
	}

	const std::string& AssemblerError::getText() const
	{
		return m_errText;
	}

	std::string AssemblerError::getMessage() const
	{
		return "Error on line " + std::to_string(m_errLine) + ": " + getText() + "\n"
			+ "SYSFILE: " + m_sysErrFile + "  SYSLINE: " + std::to_string(m_sysErrLine);
	}

	Assembler::Assembler(const std::string& asmCode, MemoryRef staticStack)
		: m_asmCode(asmCode), m_staticStack(staticStack)
	{
		m_pModInfo = std::make_shared<ModuleInfo>();
	}

	bool Assembler::assemble()
	{
		m_pModInfo->backup();

		while (parseLine(*m_pModInfo, m_lastErr) && m_nextCharToAssemble < m_asmCode.size())
			++m_pModInfo->nLinesParsed;

		if (m_lastErr)
		{
			m_pModInfo->recover();

			return false;
		}

		return true;
	}

	ModuleInfoRef Assembler::getModuleInfo()
	{
		return m_pModInfo;
	}

	const AssemblerError& Assembler::lastError() const
	{
		return m_lastErr;
	}

	bool Assembler::parseLine(ModuleInfo& mi, AssemblerError& err)
	{
		std::vector<std::string> tokens;
		if (!tokenizeLine(mi, tokens, err))
			return false;
		++m_nextCharToAssemble;

		if (tokens.size() == 0)
			return true;

		if (isInstruction(tokens[0]))
			return parseInstruction(mi, tokens, err);

		if (isDirective(tokens[0]))
			return parseDirective(mi, tokens, err);

		return true;
	}

	bool Assembler::parseNumericArgument(ModuleInfo& mi, AsmArgInfo& aai, AssemblerError& err)
	{
		//auto& memArg = *(BC_MemAddress*)(aai.pArg);

		std::vector<std::string> tokens;
		if (!tokenizeNumericArgument(mi ,*aai.pString, tokens, err))
			return false;

		aai.pArg->datatype = aai.datatype;
		bool derefThisArg = false;
		if (tokens[0] == "@")
		{
			bool derefThisArg = true;

			aai.pArg->datatype = BC_DT_U_64;

			switch (aai.nthArg)
			{
			case 0: aai.pOcx->derefArg0 = true; break;
			case 1: aai.pOcx->derefArg1 = true; break;
			}

			tokens.erase(tokens.begin());

			if (tokens.empty())
				RETURN_WITH_ERROR(AsmErrCode::LiteralMissingAfterDerefOp, "Missing literal after dereference operator!");
		}

		if (tokens[0][0] == '$')
		{
			aai.pArg->datatype = BC_DT_U_64;

			uint64_t regID = BC_MemRegisterID(BC_RegisterFromString(tokens[0]));

			aai.pArg->cell.as_ADDR = BC_MemAddress(BC_MEM_BASE_REGISTER, regID);

			return true;
		}
		else if (tokens[0] == "~")
		{
			tokens.erase(tokens.begin());
			if (tokens.empty())
				RETURN_WITH_ERROR(AsmErrCode::NumericArgumentBroken, "Missing literal after frame-relative operator!");

			bool isNegative = (tokens[0] == "-");
			if (isNegative || tokens[0] == "+")
				tokens.erase(tokens.begin());

			if (tokens.empty())
				RETURN_WITH_ERROR(AsmErrCode::NumericLiteralBroken, "Missing digits after sign!");

			uint64_t offset;
			if (!literalToU64(tokens[0], offset))
				RETURN_WITH_ERROR(AsmErrCode::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type U64!");

			aai.pArg->datatype = BC_DT_U_64;

			aai.pArg->cell.as_ADDR = BC_MemAddress(
				isNegative ? BC_MEM_BASE_DYN_FRAME_SUB : BC_MEM_BASE_DYN_FRAME_ADD,
				offset
			);

			return true;
		}

		if (isLiteral(tokens))
		{
			if (!parseLiteral(mi, tokens, derefThisArg, aai, err))
				return false;
			return true;
		}

		if (std::isalpha(tokens[0][0]))
		{
			if (aai.pOcx->datatype != BC_DT_U_64)
				RETURN_WITH_ERROR(AsmErrCode::DatatypeMismatch, "Labels without a dereference operator can only be used as U64 values.");

			//mi.unresolvedRefs.push_back(std::make_pair(tokens[0], mi.codeMemory->size() + aai.offsetInInstruction));
			mi.unresolvedRefs.push_back({ tokens[0], mi.codeMemory->size() + aai.offsetInInstruction });

			return true;
		}

		RETURN_WITH_ERROR(AsmErrCode::NumericArgumentBroken, "Cannot parse '" + *aai.pString + "' as a numeric argument!");
	}

	bool Assembler::isLiteral(const std::vector<std::string>& tokens)
	{
		return tokens[0].size() == 1 && (tokens[0][0] == '+' || tokens[0][0] == '-' || ('0' <= tokens[0][0] && tokens[0][0] <= '9'));
	}

	bool Assembler::parseLiteral(const ModuleInfo& mi, std::vector<std::string>& tokens, bool deref, AsmArgInfo& aai, AssemblerError& err)
	{
		auto& memArg = *(BC_MemAddress*)(aai.pArg);

		bool isNegative = (tokens[0] == "-");
		if (isNegative || tokens[0] == "+")
			tokens.erase(tokens.begin());

		if (tokens.empty())
			RETURN_WITH_ERROR(AsmErrCode::NumericLiteralBroken, "Missing digits after sign!");

		if (deref)
		{
			uint64_t val;
			if (!literalToU64(tokens[0], val))
				RETURN_WITH_ERROR(AsmErrCode::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type U64!")

			memArg.addr = val;

			return true;
		}

		switch (aai.pOcx->datatype)
		{
		case BC_DT_I_8:
			if (!literalToU64(tokens[0], aai.pArg->cell.as_U_64))
				RETURN_WITH_ERROR(AssemblerError::Code::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type I8!")
			if (isNegative)
				aai.pArg->cell.as_I_8 *= -1;
			break;
		case BC_DT_I_16:
			if (!literalToU64(tokens[0], aai.pArg->cell.as_U_64))
				RETURN_WITH_ERROR(AssemblerError::Code::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type I16!")
			if (isNegative)
				aai.pArg->cell.as_I_16 *= -1;
			break;
		case BC_DT_I_32:
			if (!literalToU64(tokens[0], aai.pArg->cell.as_U_64))
				RETURN_WITH_ERROR(AssemblerError::Code::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type I32!")
			if (isNegative)
				aai.pArg->cell.as_I_32 *= -1;
			break;
		case BC_DT_I_64:
			if (!literalToU64(tokens[0], aai.pArg->cell.as_U_64))
				RETURN_WITH_ERROR(AssemblerError::Code::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type I64!")
			if (isNegative)
				aai.pArg->cell.as_I_64 *= -1;
			break;
		case BC_DT_U_8:
			if (!literalToU64(tokens[0], aai.pArg->cell.as_U_64))
				RETURN_WITH_ERROR(AssemblerError::Code::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type U8!")
			if (isNegative)
				aai.pArg->cell.as_U_8 *= -1;
			break;
		case BC_DT_U_16:
			if (!literalToU64(tokens[0], aai.pArg->cell.as_U_64))
				RETURN_WITH_ERROR(AssemblerError::Code::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type U16!")
			if (isNegative)
				aai.pArg->cell.as_U_16 *= -1;
			break;
		case BC_DT_U_32:
			if (!literalToU64(tokens[0], aai.pArg->cell.as_U_64))
				RETURN_WITH_ERROR(AssemblerError::Code::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type U32!")
			if (isNegative)
				aai.pArg->cell.as_U_32 *= -1;
			break;
		case BC_DT_U_64:
			if (!literalToU64(tokens[0], aai.pArg->cell.as_U_64))
				RETURN_WITH_ERROR(AssemblerError::Code::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type U64!")
			if (isNegative)
				aai.pArg->cell.as_U_64 *= -1;
			break;
		case BC_DT_F_32:
			if (!literalToF64(tokens[0], aai.pArg->cell.as_F_64))
				RETURN_WITH_ERROR(AssemblerError::Code::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type F32!")
				aai.pArg->cell.as_F_32 = aai.pArg->cell.as_F_64;
			if (isNegative)
				aai.pArg->cell.as_F_32 *= -1.0f;
			break;
		case BC_DT_F_64:
			if (!literalToF64(tokens[0], aai.pArg->cell.as_F_64))
				RETURN_WITH_ERROR(AssemblerError::Code::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type F64!")
			if (isNegative)
				aai.pArg->cell.as_F_64 *= -1.0f;
			break;
		case BC_DT_BOOL:
			if (!literalToU64(tokens[0], aai.pArg->cell.as_U_64))
				RETURN_WITH_ERROR(AssemblerError::Code::NumericLiteralBroken, "Cannot convert literal '" + tokens[0] + "' to type BOOL!");
			aai.pArg->cell.as_BOOL = aai.pArg->cell.as_U_64;
			break;
		}

		return true;
	}

	bool Assembler::parseInstruction(ModuleInfo& mi, std::vector<std::string>& tokens, AssemblerError& err)
	{
		auto opCodeParts = getOpCodePartsFromToken(tokens[0]);
		BC_OpCodeEx ocx = { 0 };
		ocx.opCode = BC_OpCodeFromString(opCodeParts.first);
		ocx.datatype = BC_DatatypeFromString(opCodeParts.second);

		if (ocx.opCode == BC_OC_UNKNOWN)
			RETURN_WITH_ERROR(AsmErrCode::OpCodeUnknown, "Unknown opCode '" + opCodeParts.first + "'!");
		if (ocx.opCode == BC_OC_NONE)
			RETURN_WITH_ERROR(AsmErrCode::OpCodeMissing, "Missing opCode!");
		if (ocx.datatype == BC_DT_UNKNOWN)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeUnknown, "Unknown datatype '" + opCodeParts.second + "'!");

		switch (ocx.opCode)
		{
		case BC_OC_MOVE:
		case BC_OC_ADD:
		case BC_OC_SUBTRACT:
		case BC_OC_MULTIPLY:
		case BC_OC_DIVIDE:
			if (!parse_insAlgebraicBinary(mi, tokens, ocx, err))
				return false;
			break;

		case BC_OC_CONVERT:
			if (!parse_insConvert(mi, tokens, ocx, err))
				return false;
			break;

		case BC_OC_COPY:
			//if (!isCorrectTokenNum(4, tokens.size(), mi, err))
			//	return false;
			break;

		case BC_OC_PUSH:
			if (!parse_insPush(mi, tokens, ocx, err))
				return false;
			break;
		case BC_OC_POP:
			if (!parse_insPop(mi, tokens, ocx, err))
				return false;
			break;
		case BC_OC_PUSHC:
			if (!parse_insPushCopy(mi, tokens, ocx, err))
				return false;
			break;
		case BC_OC_POPC:
			if (!parse_insPopCopy(mi, tokens, ocx, err))
				return false;
			break;

		case BC_OC_PUSH_FRAME:
			if (!parse_insPushFrame(mi, tokens, ocx, err))
				return false;
			break;
		case BC_OC_POP_FRAME:
			if (!parse_insPopFrame(mi, tokens, ocx, err))
				return false;
			break;

		case BC_OC_JUMP:
			if (!parse_insJump(mi, tokens, ocx, err))
				return false;
			break;

		case BC_OC_CALL:
			break;
		case BC_OC_RETURN:
			//if (!isCorrectTokenNum(1, tokens.size(), mi, err))
			//	return false;
			break;

		case BC_OC_EXIT:
			if (!parse_insExit(mi, tokens, ocx, err))
				return false;
			break;
		default:
			RETURN_WITH_ERROR(AsmErrCode::OpCodeUnknown, "Function 'parseLine' cannot handle opCode '" + std::to_string(ocx.opCode) + "'!");
		}

		return true;
	}

	bool Assembler::parse_insAlgebraicBinary(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err)
	{
		if (!isCorrectTokenNum(3, 4, tokens.size(), mi, err))
			return false;
		if (ocx.datatype == BC_OC_NONE)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMissing, "Missing datatype!");

		BC_TypeCell args[2] = {};

		AsmArgInfo aai;
		aai.pOcx = &ocx;
		aai.datatype = (BC_Datatype)ocx.datatype;

		aai.nthArg = 0;
		aai.pString = &tokens[1];
		aai.pArg = &args[0];
		aai.offsetInInstruction = sizeof(BC_OpCodeEx);
		if (!parseNumericArgument(mi, aai, err))
			return false;
		if (aai.pArg->datatype != BC_DT_U_64)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMismatch, "Invalid datatype for destination argument!");

		aai.nthArg = 1;
		aai.pString = &tokens[2];
		aai.pArg = &args[1];
		aai.offsetInInstruction += BC_DatatypeSize(args[0].datatype);
		if (!parseNumericArgument(mi, aai, err))
			return false;

		mi.codeMemory->push(ocx);
		mi.codeMemory->push(&args[0].cell, BC_DatatypeSize(args[0].datatype));
		mi.codeMemory->push(&args[1].cell, BC_DatatypeSize(args[1].datatype));

		return true;
	}

	bool Assembler::parse_insConvert(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err)
	{
		if (!isCorrectTokenNum(3, tokens.size(), mi, err))
			return false;

		if (ocx.datatype == BC_OC_NONE)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMissing, "Missing datatype!");

		AsmArgInfo aai;
		BC_TypeCell arg;
		aai.pOcx = &ocx;
		aai.datatype = (BC_Datatype)ocx.datatype;

		aai.nthArg = 0;
		aai.pString = &tokens[1];
		aai.pArg = &arg;
		aai.offsetInInstruction = sizeof(BC_OpCodeEx);
		if (!parseNumericArgument(mi, aai, err))
			return false;
		if (aai.pArg->datatype != BC_DT_U_64)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMismatch, "Invalid datatype for destination argument!");

		BC_Datatype newDT = BC_DatatypeFromString(tokens[2]);

		if (newDT == BC_DT_NONE)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMissing, "Missing datatype!");
		if (newDT == BC_DT_UNKNOWN)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeUnknown, "Unknown datatype!");

		mi.codeMemory->push(ocx);
		mi.codeMemory->push(&arg.cell, BC_DatatypeSize(arg.datatype));
		mi.codeMemory->push(newDT);

		return true;
	}

	bool Assembler::parse_insPush(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err)
	{
		if (!isCorrectTokenNum(1, tokens.size(), mi, err))
			return false;
		if (ocx.datatype == BC_OC_NONE)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMissing, "Missing datatype!");

		mi.codeMemory->push(ocx);

		return true;
	}

	bool Assembler::parse_insPop(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err)
	{
		if (!isCorrectTokenNum(1, tokens.size(), mi, err))
			return false;
		if (ocx.datatype == BC_OC_NONE)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMissing, "Missing datatype!");

		mi.codeMemory->push(ocx);

		return true;
	}

	bool Assembler::parse_insPushCopy(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err)
	{
		if (!isCorrectTokenNum(2, tokens.size(), mi, err))
			return false;
		if (ocx.datatype == BC_OC_NONE)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMissing, "Missing datatype!");

		AsmArgInfo aai;
		BC_TypeCell arg;
		aai.pOcx = &ocx;
		aai.datatype = (BC_Datatype)ocx.datatype;

		aai.nthArg = 0;
		aai.pString = &tokens[1];
		aai.pArg = &arg;
		aai.offsetInInstruction = sizeof(BC_OpCodeEx);
		if (!parseNumericArgument(mi, aai, err))
			return false;

		mi.codeMemory->push(ocx);
		mi.codeMemory->push(&arg.cell, BC_DatatypeSize(arg.datatype));

		return true;
	}

	bool Assembler::parse_insPopCopy(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err)
	{
		if (!isCorrectTokenNum(2, tokens.size(), mi, err))
			return false;
		if (ocx.datatype == BC_OC_NONE)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMissing, "Missing datatype!");

		AsmArgInfo aai;
		BC_TypeCell arg;
		aai.pOcx = &ocx;
		aai.datatype = (BC_Datatype)ocx.datatype;

		aai.nthArg = 0;
		aai.pString = &tokens[1];
		aai.pArg = &arg;
		aai.offsetInInstruction = sizeof(BC_OpCodeEx);
		if (!parseNumericArgument(mi, aai, err))
			return false;
		if (aai.pArg->datatype != BC_DT_U_64)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMismatch, "Invalid datatype for destination argument!");
		
		mi.codeMemory->push(ocx);
		mi.codeMemory->push(&arg.cell, BC_DatatypeSize(arg.datatype));

		return true;
	}

	bool Assembler::parse_insPushFrame(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err)
	{
		if (!isCorrectTokenNum(1, tokens.size(), mi, err))
			return false;
		if (ocx.datatype != BC_OC_NONE)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMissing, "Instruction 'pushf' doesn't take any datatype!");

		mi.codeMemory->push(ocx);

		return true;
	}

	bool Assembler::parse_insPopFrame(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err)
	{
		if (!isCorrectTokenNum(1, tokens.size(), mi, err))
			return false;
		if (ocx.datatype != BC_OC_NONE)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMissing, "Instruction 'popf' doesn't take any datatype!");

		mi.codeMemory->push(ocx);

		return true;
	}

	bool Assembler::parse_insJump(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err)
	{
		if (!isCorrectTokenNum(2, tokens.size(), mi, err))
			return false;
		if (ocx.datatype != BC_OC_NONE)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMissing, "Instruction 'jmp' doesn't take any datatype!");
		ocx.datatype = BC_DT_U_64;

		AsmArgInfo aai;
		BC_TypeCell arg;
		aai.pOcx = &ocx;
		aai.datatype = BC_DT_U_64;
		arg.datatype = BC_DT_U_64;

		aai.nthArg = 0;
		aai.pString = &tokens[1];
		aai.pArg = &arg;
		aai.offsetInInstruction = sizeof(BC_OpCodeEx);
		if (!parseNumericArgument(mi, aai, err))
			return false;
		if (aai.pArg->datatype != BC_DT_U_64)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMismatch, "Invalid datatype for destination argument!");

		mi.codeMemory->push(ocx);
		mi.codeMemory->push(&arg.cell, BC_DatatypeSize(arg.datatype));

		return true;
	}

	bool Assembler::parse_insExit(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err)
	{
		if (!isCorrectTokenNum(1, tokens.size(), mi, err))
			return false;
		if (ocx.datatype != BC_OC_NONE)
			RETURN_WITH_ERROR(AsmErrCode::DatatypeMissing, "Instruction 'exit' doesn't take any datatype!");

		mi.codeMemory->push(ocx);

		return true;
	}

	bool Assembler::parseDirective(ModuleInfo& mi, std::vector<std::string>& tokens, AssemblerError& err)
	{
		if (tokens[0] == "#module")
			RETURN_WITH_ERROR(AsmErrCode::NotImplemented, "The directive '" + tokens[0] + "' has not been implemented yet!");
		if (tokens[0] == "#insmod")
			RETURN_WITH_ERROR(AsmErrCode::NotImplemented, "The directive '" + tokens[0] + "' has not been implemented yet!");
		if (tokens[0] == "#label")
			return parse_dirLabel(mi, tokens, err);
		if (tokens[0] == "#static")
			RETURN_WITH_ERROR(AsmErrCode::NotImplemented, "The directive '" + tokens[0] + "' has not been implemented yet!");
		if (tokens[0] == "#string")
			RETURN_WITH_ERROR(AsmErrCode::NotImplemented, "The directive '" + tokens[0] + "' has not been implemented yet!");
		if (tokens[0] == "#permission")
			RETURN_WITH_ERROR(AsmErrCode::NotImplemented, "The directive '" + tokens[0] + "' has not been implemented yet!");

		RETURN_WITH_ERROR(AsmErrCode::DirectiveUnknown, "Unknown directive '" + tokens[0] + "'!");
	}

	bool Assembler::parse_dirLabel(ModuleInfo& mi, std::vector<std::string>& tokens, AssemblerError& err)
	{
		if (!isCorrectTokenNum(2, tokens.size(), mi, err))
			return false;
		if (mi.labels.find(tokens[1]) != mi.labels.end())
			RETURN_WITH_ERROR(AsmErrCode::LabelAlreadyDefined, "A label with name '" + tokens[1] + "' has already been defined!");
		mi.labels.insert(std::make_pair(tokens[1], currCodeAddr(mi)));

		return true;
	}

	bool Assembler::tokenizeLine(const ModuleInfo& mi, std::vector<std::string>& tokensOut, AssemblerError& err)
	{
		uint64_t lineEnd = m_asmCode.find_first_of('\n', m_nextCharToAssemble);
		if (lineEnd == std::string::npos)
			lineEnd = m_asmCode.size();
		
		enum class State
		{
			BeginToken = 0,
			ParseLiteral,
			ParseString,
			ParseComment,
			EndToken,
		} state = State::BeginToken;

		std::string currentToken;

		while (m_nextCharToAssemble < lineEnd)
		{
			char c = (m_asmCode)[m_nextCharToAssemble];

			switch (state)
			{
			case State::ParseString:
				switch (c)
				{
				case '"':
					state = State::EndToken;
					break;
				}
				currentToken.push_back(c);
				break;
			case State::BeginToken:
				switch (c)
				{
				case '"':
					currentToken.push_back(c);
					state = State::ParseString;
					break;
				case ':':
					state = State::EndToken;
					break;
				case ' ':
					break;
				case '/':
					state = State::ParseComment;
					break;
				default:
					if (!std::isalpha(c) && c != '@' && c != '$' && c != '~' && c != '+' && c != '-' && c != '#')
						RETURN_WITH_ERROR(AsmErrCode::CharInvalid, std::string("Character '") + c + "' is not allowed in this context!");
					currentToken.push_back(c);
					state = State::ParseLiteral;
				}
				break;
			case State::ParseComment:
				break;
			case State::ParseLiteral:
				if (std::isalnum(c) || c == '.' || c == '$' || c == '~' || c == '+' || c == '-' || c == '#')
					currentToken.push_back(c);
				else
				{
					state = State::EndToken;
					m_nextCharToAssemble -= 1;
				}
			}

			if (state == State::EndToken || m_nextCharToAssemble == lineEnd - 1)
			{
				if (!currentToken.empty())
					tokensOut.push_back(currentToken);
				currentToken.clear();
				state = State::BeginToken;
			}

			++m_nextCharToAssemble;
		}

		return true;
	}

	bool Assembler::tokenizeNumericArgument(const ModuleInfo& mi, const std::string& argument, std::vector<std::string>& tokensOut, AssemblerError& err)
	{
		enum State
		{
			STATE_BEGIN_TOKEN,
			STATE_PARSE_REGISTER,
			STATE_PARSE_LABEL,
			STATE_PARSE_LITERAL,
			STATE_END_TOKEN,
		} state = STATE_BEGIN_TOKEN;

		std::string currToken;

		for (uint64_t i = 0; i < argument.size(); ++i)
		{
			char c = argument[i];

			switch (state)
			{
			case STATE_BEGIN_TOKEN:
				switch (c)
				{
				case '~':
				case '+':
				case '-':
				case '@':
					currToken.push_back(c);
					state = STATE_END_TOKEN;
					break;
				case '$':
					currToken.push_back(c);
					state = STATE_PARSE_REGISTER;
					break;
				default:
					if (std::isalpha(c))
						state = STATE_PARSE_LABEL;
					else if ('0' <= c && c <= '9')
						state = STATE_PARSE_LITERAL;
					else
						RETURN_WITH_ERROR(AsmErrCode::CharInvalid, std::string("Character '") + c + "' is not allowed in this context!");

					currToken.push_back(c);
				}
				break;
			case STATE_PARSE_REGISTER:
				if (std::isalpha(c))
					currToken.push_back(c);
				else
				{
					state = STATE_END_TOKEN;
					--i;
				}
				break;
			case STATE_PARSE_LABEL:
				if (std::isalnum(c))
					currToken.push_back(c);
				else
				{
					state = STATE_END_TOKEN;
					--i;
				}
				break;
			case STATE_PARSE_LITERAL:
				if (('0' <= c && c <= '9') || c == '.')
					currToken.push_back(c);
				else
				{
					state = STATE_END_TOKEN;
					--i;
				}
				break;
			}

			if (state == STATE_END_TOKEN)
			{
				tokensOut.push_back(currToken);
				currToken.clear();
				state = STATE_BEGIN_TOKEN;
			}
		}

		if (!currToken.empty())
			tokensOut.push_back(currToken);


		if (tokensOut.empty())
			RETURN_WITH_ERROR(AsmErrCode::TokenUnexpectedNum, "No tokens found in mueric argument '" + argument + "'!")
		return true;
	}

	bool Assembler::isInstruction(const std::string& token)
	{
		return token.size() > 0 && token[0] != '#';
	}

	bool Assembler::isDirective(const std::string& token)
	{
		return !isInstruction(token);
	}

	std::pair<std::string, std::string> Assembler::getOpCodePartsFromToken(const std::string& token)
	{
		uint64_t dot = token.find_first_of('.');

		if (dot == std::string::npos)
			return { token, "" };

		return { token.substr(0, dot), token.substr(dot + 1) };
	}

	bool Assembler::literalToU64(const std::string& literalStr, uint64_t& output)
	{
		try
		{
			output = std::stoull(literalStr.c_str());
		}
		catch (const std::invalid_argument&)
		{
			return false;
		}
		return true;
	}

	bool Assembler::literalToF64(const std::string& literalStr, double& output)
	{
		try
		{
			output = std::stod(literalStr.c_str());
		}
		catch (const std::invalid_argument&)
		{
			return false;
		}
		return true;
	}

	bool Assembler::isCorrectTokenNum(uint64_t expected, uint64_t provided, const ModuleInfo& mi, AssemblerError& err)
	{
		return isCorrectTokenNum(expected, expected, provided, mi, err);
	}
	bool Assembler::isCorrectTokenNum(uint64_t expectedMin, uint64_t expectedMax, uint64_t provided, const ModuleInfo& mi, AssemblerError& err)
	{
		if (!(expectedMin <= provided && provided <= expectedMax))
			RETURN_WITH_ERROR(
				AsmErrCode::TokenUnexpectedNum,
				"Unexpected number of tokens! (Range: " + std::to_string(expectedMin) + "-" + std::to_string(expectedMax) + "; Provided: " + std::to_string(provided) + ")"
			);
		return true;
	}

	BC_MemAddress Assembler::currCodeAddr(ModuleInfo& mi)
	{
		return BC_MemAddress(BC_MEM_BASE_CODE_MEMORY, 0, mi.codeMemory->size());
	}
}