#include "Compiler.h"

#include "AsmTokenizer.h"
#include "VirtualAsmTokenList.h"

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
		return 
			"ERROR ON L: " + std::to_string(m_line) + " C: " + std::to_string(m_column) + "\n" +
			"  -> " + getText() + "\n" +
			"  SYSFILE: " + m_sysErrFile + "  SYSLINE: " + std::to_string(m_sysErrLine)
			;
	}

	Compiler::Compiler(const AsmTokenListRef tokenList, const std::string& moduleName)
		: m_pTokenList(tokenList)
	{
		m_pModInfo = std::make_shared<ModuleInfo>();
		m_pModInfo->moduleName = moduleName;
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
			COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Newline, currToken());

		return true;
	}

	bool Compiler::compileStatement(const std::string& statement)
	{
		AsmTokenizer tok(statement);
		if (!tok.tokenize())
			COMPILER_RETURN_WITH_ERROR(CompErrCode::InternalError, "Unable to tokenize internal statement '" + statement +"'!");
		
		{
			VirtualAsmTokenList vtl(*this, tok.getTokenList());

			if (!compileStatement())
				return false;
		}

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
				COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());
			ocx.datatype = BC_DatatypeFromString(nextToken().value);
			if (ocx.datatype == BC_DT_NONE || ocx.datatype == BC_DT_UNKNOWN)
				COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");
		}

		if (layout.needsCustomImplementation)
			return compileSpecializedInstruction(ocx);
		
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

	bool Compiler::compileSpecializedInstruction(BC_OpCodeEx& ocx)
	{
		switch (ocx.opCode)
		{
		case BC_OC_CALL:
			return compileSpecCall(ocx);
		}

		COMPILER_RETURN_WITH_ERROR(CompErrCode::InternalError, "OpCode '" + BC_OpCodeToString(ocx.opCode) + "' is not specialized!");
	}

	bool Compiler::compileSpecCall(BC_OpCodeEx& ocx)
	{
		if (!removeNecessaryColon())
			return false;

		uint64_t opCodeBegin = currCodeOffset();
		pushCode(ocx);

		if (!compileArgAddress(ocx, { InsArgType::Address, BC_DT_NONE, 0 }))
			return false;

		if (!removeNecessaryColon())
			return false;

		std::string retDtStr = nextToken().value;
		if (nextToken().type != AsmToken::Type::Sep_Dot)
			COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());
		std::string retVal = getArgAsString();

		BC_FuncCallData fcd;
		uint64_t fcdBegin = currCodeOffset();
		pushCode(fcd);
		while (nextToken().type == AsmToken::Type::Sep_Colon)
		{
			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());
			BC_Datatype argDt = BC_DatatypeFromString(currToken().value);
			fcd.argType.set(fcd.nArgs, argDt);

			if (nextToken().type != AsmToken::Type::Sep_Dot)
				COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());

			if (!compileArgument(ocx, { InsArgType::Value, BC_DT_NONE, (uint64_t)1 + fcd.nArgs }))
				return false;

			++fcd.nArgs;
		}
		prevToken();

		writeCode(ocx, opCodeBegin);
		writeCode(fcd, fcdBegin);

		if (!compileStatement("popc." + retDtStr + " : " + retVal))
			return false;

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
		case InsArgType::TypedValue:
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

		TypeCell tc;
		switch (arg.type)
		{
		case InsArgType::Address: tc.datatype = BC_DT_U_64; break;
		case InsArgType::TypedValue: tc.datatype = arg.datatype; break;
		case InsArgType::Value: tc.datatype = ocx.datatype; break;
		}

		bool getsDereferenced;
		if (!generateTypeCell(tc, getsDereferenced))
			return false;

		if (getsDereferenced)
			ocx.derefArg.set(arg.index);

		pushCode(&tc.cell, BC_DatatypeSize(tc.datatype));

		return true;
	}

	bool Compiler::generateTypeCell(TypeCell& tc, bool& getsDereferenced)
	{
		getsDereferenced = false;
		if (currToken().type == AsmToken::Type::Op_Deref)
		{
			tc.datatype = BC_DT_U_64;
			getsDereferenced = true;
			nextToken();
		}

		switch (currToken().type)
		{
		case AsmToken::Type::Op_Register:
			if (!generateTypeCellRegister(tc))
				return false;
			break;
		case AsmToken::Type::Op_FP_Relative:
			if (!generateTypeCellFPRelative(tc))
				return false;
			break;
		case AsmToken::Type::Name:
			if (!generateTypeCellName(tc))
				return false;
			break;
		case AsmToken::Type::String:
			if (!generateTypeCellString(tc))
				return false;
			break;
		case AsmToken::Type::Float:
			if (!generateTypeCellFloat(tc, getsDereferenced))
				return false;
			break;
		case AsmToken::Type::Integer:
			if (!generateTypeCellInteger(tc, getsDereferenced))
				return false;
			break;
		}
		return true;
	}

	bool Compiler::generateTypeCellRegister(TypeCell& tc)
	{
		auto reg = BC_RegisterFromString(nextToken().value);
		if (reg == BC_MEM_REG_NONE || reg == BC_MEM_REG_UNKNOWN)
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnknownRegisterName, "Unknown register name '" + currToken().value + "'!");
		tc.cell.as_ADDR = BC_MemAddress(BC_MEM_BASE_REGISTER, reg);
		return true;
	}

	bool Compiler::generateTypeCellFPRelative(TypeCell& tc)
	{
		if (nextToken().type != AsmToken::Type::Integer)
			COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Integer, currToken());

		uint64_t offset = std::stoull(positiveString(currToken().value));

		bool isNegative = isNegativeString(currToken().value);
		auto base = isNegative ? BC_MEM_BASE_DYN_FRAME_SUB : BC_MEM_BASE_DYN_FRAME_ADD;

		tc.cell.as_ADDR = BC_MemAddress(base, offset);

		return true;
	}

	bool Compiler::generateTypeCellName(TypeCell& tc)
	{
		m_pModInfo->unresolvedRefs.push_back(
			{
				getScopedName(currToken().value),
				currCodeOffset(),
				tc.datatype
			}
		);

		return true;
	}

	bool Compiler::generateTypeCellString(TypeCell& tc)
	{
		if (tc.datatype != BC_DT_U_64)
			COMPILER_RETURN_WITH_ERROR(CompErrCode::DatatypeMismatch, "Cannot use string in instruction with datatype '" + BC_DatatypeToString(tc.datatype) + "'!");

		tc.cell.as_ADDR = BC_MemAddress(BC_MEM_BASE_STATIC_STACK, 0, m_pModInfo->staticStack->size());
		m_pModInfo->staticStack->push(currToken().value.c_str(), currToken().value.size() + 1);

		return true;
	}

	bool Compiler::generateTypeCellFloat(TypeCell& tc, bool getsDereferenced)
	{
		if (getsDereferenced)
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Cannot dereference argument of type 'float'!");
		switch (tc.datatype)
		{
		case BC_DT_F_32:
			tc.cell.as_F_32 = std::stof(currToken().value); break;
		case BC_DT_F_64:
			tc.cell.as_F_64 = std::stod(currToken().value); break;
		default:
			COMPILER_RETURN_WITH_ERROR(CompErrCode::DatatypeMismatch, "Cannot use floating-point literal '" + currToken().value + "' in instruction with datatype '" + BC_DatatypeToString(tc.datatype) + "'!");
		}

		return true;
	}

	bool Compiler::generateTypeCellInteger(TypeCell& tc, bool getsDereferenced)
	{
		if (!getsDereferenced && (tc.datatype == BC_DT_F_32 || tc.datatype == BC_DT_F_64))
			COMPILER_RETURN_WITH_ERROR(CompErrCode::DatatypeMismatch, "Cannot use integer '" + currToken().value + "' in instruction with datatype '" + BC_DatatypeToString(tc.datatype) + "'!");

		tc.cell.as_U_64 = std::stoull(positiveString(currToken().value));
		if (isNegativeString(currToken().value))
		{
			if (getsDereferenced)
				COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Cannot dereference literal with negative value!");

			switch (tc.datatype)
			{
			case BC_DT_I_8:
			case BC_DT_U_8:
				tc.cell.as_I_8 *= -1; break;
			case BC_DT_I_16:
			case BC_DT_U_16:
				tc.cell.as_I_16 *= -1; break;
			case BC_DT_I_32:
			case BC_DT_U_32:
				tc.cell.as_I_32 *= -1; break;
			case BC_DT_I_64:
			case BC_DT_U_64:
				tc.cell.as_I_64 *= -1; break;
			}
		}

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
		case DirectiveID::Alias:
			return compileDirAlias();
		case DirectiveID::Static:
			return compileDirStatic();
		case DirectiveID::RequestModule:
			return compileDirRequestModule();
		case DirectiveID::Scope:
			return compileDirScope();
		case DirectiveID::End:
			return compileDirEnd();
		case DirectiveID::Function:
			return compileDirFunction();
		}

		COMPILER_RETURN_WITH_ERROR(CompErrCode::UnknownDirectiveID, "Unknown directive '" + currToken().value + "'!");
	}

	bool Compiler::compileDirLabel()
	{
		if (!removeNecessaryColon())
			return false;

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		if (!addSymbol(currToken().value, Symbol(SymbolUsage::Address, currCodeAddr())))
			return false;

		return true;
	}

	bool Compiler::compileDirAlias()
	{
		if (!removeNecessaryColon())
			return false;

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		std::string name = currToken().value;

		if (!removeNecessaryColon())
			return false;

		Symbol symbol;

		TypeCell tc;
		switch (nextToken().type)
		{
		case AsmToken::Type::Op_Register:
			tc.datatype = BC_DT_U_64;
			symbol.usage = SymbolUsage::Address;
			break;
		case AsmToken::Type::Op_FP_Relative:
			tc.datatype = BC_DT_U_64;
			symbol.usage = SymbolUsage::Address;
			break;
		case AsmToken::Type::String:
			tc.datatype = BC_DT_U_64;
			symbol.usage = SymbolUsage::Address;
			break;
		case AsmToken::Type::Float:
			tc.datatype = BC_DT_F_64;
			symbol.usage = SymbolUsage::Value;
			break;
		case AsmToken::Type::Integer:
			tc.datatype = BC_DT_U_64;
			symbol.usage = SymbolUsage::Value;
			break;
		default:
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Unexpected token'" + currToken().value + "' for alias value!");
		}

		bool getsDereferenced;
		if (!generateTypeCell(tc, getsDereferenced))
			return false;

		if (getsDereferenced)
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Usage of the deref operator is not allowed in the alias directive!");

		symbol.value = tc.cell;

		if (!addSymbol(name, symbol))
			return false;

		return true;
	}

	bool Compiler::compileDirStatic()
	{
		if (!removeNecessaryColon())
			return false;

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		std::string name = currToken().value;

		if (!removeNecessaryColon())
			return false;

		TypeCell tc;
		tc.datatype = BC_DT_U_64;

		nextToken();

		bool getsDereferenced;
		if (!generateTypeCell(tc, getsDereferenced))
			return false;

		if (getsDereferenced)
			COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Usage of the deref operator is not allowed in the static directive!");

		BC_MemCell mc;
		mc.as_ADDR = BC_MemAddress(BC_MEM_BASE_STATIC_STACK, 0, m_pModInfo->staticStack->size());
		m_pModInfo->staticStack->resize(m_pModInfo->staticStack->size() + tc.cell.as_U_64);

		if (!addSymbol(name, Symbol(SymbolUsage::Address, mc)))
			return false;

		return true;
	}

	bool Compiler::compileDirRequestModule()
	{
		if (!removeNecessaryColon())
			return false;

		if (nextToken().type != AsmToken::Type::String)
			COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::String, currToken());

		std::string modName = currToken().value;

		m_pModInfo->requiredModules.push_back(modName);

		return true;
	}

	bool Compiler::compileDirScope()
	{
		if (!removeNecessaryColon())
			return false;

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		if (!addScope(currToken().value))
			return false;

		return true;
	}

	bool Compiler::compileDirEnd()
	{
		if (m_scopeList.size() == 0)
			COMPILER_RETURN_WITH_ERROR(CompErrCode::AlreadyInGlobalScope, "Cannot end scope, already in global scope!");

		if (!addSymbol("SCOPE_END", Symbol(SymbolUsage::Address, currCodeAddr())))
			return false;

		m_scopeList.pop_back();

		return true;
	}

	bool Compiler::compileDirFunction()
	{
		if (!removeNecessaryColon())
			return false;

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		{
			std::string funcName = currToken().value;

			if (!compileStatement("jmp : " + funcName + ">>SCOPE_END"))
				return false;
			if (!addScope(funcName))
				return false;
		}

		if (!removeNecessaryColon())
			return false;

		{
			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			BC_Datatype dt = BC_DatatypeFromString(currToken().value);
			if (dt == BC_DT_NONE || dt == BC_DT_UNKNOWN)
				COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");

			if (nextToken().type != AsmToken::Type::Sep_Dot)
				COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());

			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			std::string retName = currToken().value;

			if (!compileStatement("#alias : " + retName + " : ~-" + std::to_string(2 * BC_DatatypeSize(BC_DT_U_64) + BC_DatatypeSize(dt))))
				return false;
		}

		uint64_t offset = 0;
		while (nextToken().type == AsmToken::Type::Sep_Colon)
		{
			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			BC_Datatype dt = BC_DatatypeFromString(currToken().value);
			if (dt == BC_DT_NONE || dt == BC_DT_UNKNOWN)
				COMPILER_RETURN_WITH_ERROR(CompErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");

			if (nextToken().type != AsmToken::Type::Sep_Dot)
				COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());

			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			std::string retName = currToken().value;

			if (!compileStatement("#alias : " + retName + " : ~+" + std::to_string(offset)))
				return false;

			offset += BC_DatatypeSize(dt);
		}

		prevToken();

		return true;
	}

	bool Compiler::removeNecessaryColon()
	{
		if (nextToken().type != AsmToken::Type::Sep_Colon)
			COMPILER_RETURN_ERR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Colon, currToken());
		return true;
	}

	std::string Compiler::getArgAsString()
	{
		std::string val;

		bool breakLoop = false;
		while (true)
		{
			auto& tok = nextToken();
			switch (tok.type)
			{
			case AsmToken::Type::Op_Deref:
			case AsmToken::Type::Op_FP_Relative:
			case AsmToken::Type::Op_Register:
			case AsmToken::Type::Name:
			case AsmToken::Type::Float:
			case AsmToken::Type::Integer:
				val.append(tok.value);
				break;
			case AsmToken::Type::String:
				val.append("\"" + tok.value + "\"");
				break;
			default:
				prevToken();
				breakLoop = true;
			}
			if (breakLoop)
				break;
		}

		return val;
	}

	bool Compiler::addSymbol(const std::string& name, const Symbol& symbol)
	{
		std::string fullName = getScopedName(name);

		if (m_pModInfo->symbols.find(fullName) != m_pModInfo->symbols.end())
			COMPILER_RETURN_WITH_ERROR(CompErrCode::SymbolAlreadyDefined, "A symbol with name '" + fullName + "' has already been defined!");
		m_pModInfo->symbols.insert({ fullName, symbol });

		return true;
	}

	bool Compiler::addScope(const std::string& name)
	{
		if (!addSymbol(name, Symbol(SymbolUsage::Address, currCodeAddr())))
			return false;

		m_scopeList.push_back(name);

		return true;
	}

	std::string Compiler::getScopedName(const std::string& name)
	{
		if (name.find(">>") == 0)
			return name;

		std::string fullName = ">>";
		for (auto& elem : m_scopeList)
			fullName.append(elem + ">>");
		fullName.append(name);

		return fullName;
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

	const AsmToken& Compiler::prevToken()
	{
		--m_nextTokenToCompile;
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
