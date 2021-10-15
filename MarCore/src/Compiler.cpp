#include "Compiler.h"

namespace MarC
{
	CompilerError::operator bool() const
	{
		return m_code != Code::Success;
	}

	const std::string& CompilerError::getText() const
	{
		return m_errText;
	}

	std::string CompilerError::getMessage() const
	{
		return "Error on token " + std::to_string(m_errToken) + ": " + getText() + "\n"
			+ "SYSFILE: " + m_sysErrFile + "  SYSLINE: " + std::to_string(m_sysErrLine);
	}

	Compiler::Compiler(const AsmTokenListRef tokenList, MemoryRef staticStack)
		: m_pTokenList(tokenList), m_staticStack(staticStack)
	{
		m_pModInfo = std::make_shared<ModuleInfo>();
	}

	ModuleInfoRef Compiler::getModuleInfo()
	{
		return m_pModInfo;
	}

	const CompilerError& Compiler::lastError() const
	{
		return m_lastErr;
	}

	bool Compiler::compile()
	{
		m_pModInfo->backup();

		while (!isEndOfCode())
			if (!compileStatement())
				break;

		if (m_lastErr)
		{
			m_pModInfo->recover();
			return false;
		}
		return true;
	}

	bool Compiler::compileStatement()
	{
		if (isInstructionLike() && !compileInstruction())
			return false;
		else if (isDirectiveLike() && !compileDirective())
			return false;

		uint64_t nNewlines = 0;
		while (nextToken().type == AsmToken::Type::Sep_Newline)
			++nNewlines;
		if (nNewlines == 0 && !isEndOfCode())
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Expected token '\\n' but found '" + currToken().value + "'!");

		return true;
	}

	bool Compiler::compileInstruction()
	{
		BC_OpCodeEx ocx;
		ocx.opCode = BC_OpCodeFromString(currToken().value);
		if (ocx.opCode == BC_OC_NONE || ocx.opCode == BC_OC_UNKNOWN)
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to opCode!");

		auto& layout = InstructionLayoutFromOpCode((BC_OpCode)ocx.opCode);

		if (layout.requiresDatatype)
		{
			if (nextToken().type != AsmToken::Type::Sep_Dot)
				COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Expected token '.' but found '" + currToken().value + "'!");
			ocx.datatype = BC_DatatypeFromString(nextToken().value);
			if (ocx.datatype == BC_DT_NONE || ocx.datatype == BC_DT_UNKNOWN)
				COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");
		}
		
		uint64_t instructionBegin = currCodeOffset();
		pushCode(ocx);

		for (auto& arg : layout.args)
		{
			if (!removeNecessaryColon())
				return false;

			if (!compileArgument(ocx, arg))
				return false;
		}

		writeCode(ocx, instructionBegin);

		return true;
	}

	bool Compiler::compileArgument(BC_OpCodeEx& ocx, const InsArgument& arg)
	{
		switch (arg.type)
		{
		case InsArgType::Address:
			if (!compileArgAddress(ocx, arg))
				return false;
			break;
		case InsArgType::Value:
			if (!compileArgValue(ocx, arg))
				return false;
			break;
		case InsArgType::Datatype:
			if (!compileArgDatatype(ocx, arg))
				return false;
			break;
		default:
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnknownInsArgType, "Cannot handle InsArgType '" + std::to_string((uint64_t)arg.type) + "'!");
		}

		return true;
	}

	bool Compiler::compileArgAddress(BC_OpCodeEx& ocx, const InsArgument& arg)
	{
		if (!compileArgValue(ocx, arg))
			return false;
		return true;
	}

	bool Compiler::compileArgValue(BC_OpCodeEx& ocx, const InsArgument& arg)
	{
		nextToken();

		TypeCell argData;
		argData.datatype = (arg.type == InsArgType::Address) ? BC_DT_U_64 : ocx.datatype;

		if (currToken().type == AsmToken::Type::Op_Deref)
		{
			argData.datatype = BC_DT_U_64;
			ocx.derefArg.set(arg.index);
			nextToken();
		}

		switch (currToken().type)
		{
		case AsmToken::Type::Op_Register:
		{
			auto reg = BC_RegisterFromString(nextToken().value);
			if (reg == BC_MEM_REG_NONE || reg == BC_MEM_REG_UNKNOWN)
				COMPILER_RETURN_WITH_ERROR(CompErrCode::UnknownRegisterName, "Unknown register name '" + currToken().value + "'!");
			argData.cell.as_ADDR = BC_MemAddress(BC_MEM_BASE_REGISTER, BC_MemRegisterID(reg));
		}
			break;
		case AsmToken::Type::Op_FP_Relative:
		{
			if (nextToken().type != AsmToken::Type::Integer)
				COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Expected token of type integer, got type '" + std::to_string((uint64_t)currToken().type) + "' with value '" + currToken().value + "'!");

			uint64_t offset = std::stoull(positiveString(currToken().value));

			bool isNegative = isNegativeString(currToken().value);
			auto base = isNegative ? BC_MEM_BASE_DYN_FRAME_SUB : BC_MEM_BASE_DYN_FRAME_ADD;

			argData.cell.as_ADDR = BC_MemAddress(base, offset);
		}
			break;
		case AsmToken::Type::Name:
		{
			m_pModInfo->unresolvedRefs.push_back(
				{
					currToken().value,
					currCodeOffset(),
					argData.datatype
				}
			);
		}
			break;
		case AsmToken::Type::String:
		{
			// TODO
		}
			break;
		case AsmToken::Type::Float:
			if (ocx.derefArg[arg.index])
				COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Cannot dereference argument of type 'float'!");
			switch (argData.datatype)
			{
			case BC_DT_F_32:
				argData.cell.as_F_32 = std::stof(currToken().value); break;
			case BC_DT_F_64:
				argData.cell.as_F_64 = std::stod(currToken().value); break;
			default:
				COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Cannot use floating-point literal '" + currToken().value + "' in instruction with datatype '" + std::to_string((uint64_t)argData.datatype) + "'!");
			}
			break;
		case AsmToken::Type::Integer:
		{
			if (!ocx.derefArg[arg.index] && (argData.datatype == BC_DT_F_32 || argData.datatype == BC_DT_F_64))
				COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Cannot use integer '" + currToken().value + "' in instruction with datatype '" + std::to_string((uint64_t)argData.datatype) + "'!");

			argData.cell.as_U_64 = std::stoull(positiveString(currToken().value));
			if (isNegativeString(currToken().value))
			{
				if (ocx.derefArg[arg.index])
					COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Cannot dereference literal with negative value!");

				switch (argData.datatype)
				{
				case BC_DT_I_8:
				case BC_DT_U_8:
					argData.cell.as_I_8 *= -1; break;
				case BC_DT_I_16:
				case BC_DT_U_16:
					argData.cell.as_I_16 *= -1; break;
				case BC_DT_I_32:
				case BC_DT_U_32:
					argData.cell.as_I_32 *= -1; break;
				case BC_DT_I_64:
				case BC_DT_U_64:
					argData.cell.as_I_64 *= -1; break;
				}
			}
		}
			break;
		}

		pushCode(&argData.cell, BC_DatatypeSize(argData.datatype));

		return true;
	}

	bool Compiler::compileArgDatatype(BC_OpCodeEx& ocx, const InsArgument& arg)
	{
		auto dt = BC_DatatypeFromString(nextToken().value);
		if (dt == BC_DT_NONE || dt == BC_DT_UNKNOWN)
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");

		pushCode(dt);

		return true;
	}

	bool Compiler::compileDirective()
	{
		switch (DirectiveIDFromString(nextToken().value))
		{
		case DirectiveID::None:
		case DirectiveID::Unknown:
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnknownDirectiveID, "Unknown directive '" + currToken().value + "'!");
			break;
		case DirectiveID::Label:
			return compileDirLabel();
		}

		COMPILER_RETURN_WITH_ERROR(CompErrCode::UnknownDirectiveID, "Unknown directive '" + currToken().value + "'!");
	}

	bool Compiler::compileDirLabel()
	{
		if (!removeNecessaryColon())
			return false;

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Expected token of type 'Name'! Got '" + std::to_string((uint64_t)currToken().type) + "'!");

		if (!addSymbol(currToken().value, Symbol(SYMBOL_USAGE_ADDRESS, currCodeAddr())))
			return false;

		return true;
	}

	bool Compiler::removeNecessaryColon()
	{
		if (nextToken().type != AsmToken::Type::Sep_Colon)
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Expected token ':' but found '" + currToken().value + "'!");
		return true;
	}

	bool Compiler::addSymbol(const std::string& name, const Symbol& symbol)
	{
		if (m_pModInfo->labels.find(name) != m_pModInfo->labels.end())
			COMPILER_RETURN_WITH_ERROR(CompErrCode::SymbolAlreadyDefined, "A symbol with name '" + name + "' has already been defined!");
		m_pModInfo->labels.insert({ name, symbol });

		return true;
	}

	bool Compiler::isInstructionLike()
	{
		return currToken().type == AsmToken::Type::Name;
	}

	bool Compiler::isDirectiveLike()
	{
		return currToken().type == AsmToken::Type::Op_Directive;
	}

	const AsmToken& Compiler::nextToken()
	{
		++m_nextTokenToCompile;
		return currToken();
	}

	const AsmToken& Compiler::currToken() const
	{
		return (*m_pTokenList)[m_nextTokenToCompile];
	}

	bool Compiler::isEndOfCode() const
	{
		return currToken().type == AsmToken::Type::END_OF_CODE;
	}

	void Compiler::pushCode(const void* data, uint64_t size)
	{
		m_pModInfo->codeMemory->push(data, size);
	}

	void Compiler::writeCode(const void* data, uint64_t size, uint64_t offset)
	{
		m_pModInfo->codeMemory->write(data, size, offset);
	}

	uint64_t Compiler::currCodeOffset() const
	{
		return m_pModInfo->codeMemory->size();
	}

	BC_MemAddress Compiler::currCodeAddr() const
	{
		return BC_MemAddress(BC_MEM_BASE_CODE_MEMORY, 0, currCodeOffset());
	}

	bool isNegativeString(const std::string& value)
	{
		return value.size() > 0 && value[0] == '-';
	}

	std::string positiveString(const std::string& value)
	{
		if (value.size() == 0)
			return "";
		if (value[0] == '-' || value[0] == '+')
			return value.substr(1);
		return value;
	}
}