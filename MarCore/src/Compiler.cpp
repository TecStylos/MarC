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
			"  -> " + getText();
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

	void Compiler::resetError()
	{
		m_lastErr = CompilerError();
	}

	void Compiler::backup()
	{
		m_pModInfo->backup();
		m_backupNextTokenToCompile = m_nextTokenToCompile;
	}
	
	void Compiler::recover()
	{
		m_pModInfo->recover();
		m_nextTokenToCompile = m_backupNextTokenToCompile;
	}

	bool Compiler::compile()
	{
		resetError();

		backup();

		try
		{
			while (!isEndOfCode())
				compileStatement();
		}
		catch (CompilerError& err)
		{
			m_lastErr = err;
			recover();
		}

		return !m_lastErr;
	}

	void Compiler::compileStatement()
	{
		if (isInstructionLike())
			compileInstruction();
		if (isDirectiveLike())
			compileDirective();

		uint64_t nNewlines = 0;
		while (nextToken().type == AsmToken::Type::Sep_Newline)
			++nNewlines;
		if (nNewlines == 0 && !isEndOfCode())
			COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Newline, currToken());
	}

	void Compiler::compileStatement(const std::string& statement)
	{
		AsmTokenizer tok(statement);
		if (!tok.tokenize())
			COMPILER_THROW_ERROR(CompErrCode::InternalError, "Unable to tokenize internal statement '" + statement +"'!");
		
		{
			VirtualAsmTokenList vtl(*this, tok.getTokenList());

			compileStatement();
		}
	}

	void Compiler::compileInstruction()
	{
		BC_OpCodeEx ocx;

		ocx.opCode = BC_OpCodeFromString(currToken().value);
		if (ocx.opCode == BC_OC_NONE || ocx.opCode == BC_OC_UNKNOWN)
			COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to opCode!");

		auto& layout = InstructionLayoutFromOpCode(ocx.opCode);

		if (layout.insDt != InsDt::None)
		{
			if (nextToken().type == AsmToken::Type::Sep_Dot)
			{
				ocx.datatype = BC_DatatypeFromString(nextToken().value);
				if (ocx.datatype == BC_DT_UNKNOWN)
					COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");
			}
			else
			{
				if (layout.insDt == InsDt::Required)
					COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());
				prevToken();
			}
		}

		DelayedPush<BC_OpCodeEx> ocxDelayed(*this, ocx);

		if (layout.needsCustomImplementation)
		{
			compileSpecializedInstruction(ocx);
			return;
		}

		for (auto& arg : layout.args)
		{
			removeNecessaryColon();

			compileArgument(ocx, arg);
		}
	}

	void Compiler::compileSpecializedInstruction(BC_OpCodeEx& ocx)
	{
		switch (ocx.opCode)
		{
		case BC_OC_CALL: compileSpecCall(ocx); return;
		case BC_OC_CALL_EXTERN: compileSpecCallExtern(ocx); return;
		}

		COMPILER_THROW_ERROR(CompErrCode::InternalError, "OpCode '" + BC_OpCodeToString(ocx.opCode) + "' is not specialized!");
	}

	void Compiler::compileSpecCall(BC_OpCodeEx& ocx)
	{
		removeNecessaryColon();

		compileArgAddress(ocx, { InsArgType::Address, BC_DT_NONE, 0 });

		std::string retDtStr;
		std::string retVal;
		if (ocx.datatype != BC_DT_NONE)
		{
			removeNecessaryColon();

			retDtStr = BC_DatatypeToString(ocx.datatype);
			retVal = getArgAsString();
		}

		DelayedPush<BC_FuncCallData> fcd(*this);

		while (nextToken().type == AsmToken::Type::Sep_Colon)
		{
			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());
			BC_Datatype argDt = BC_DatatypeFromString(currToken().value);
			fcd->argType.set(fcd->nArgs, argDt);

			if (nextToken().type != AsmToken::Type::Sep_Dot)
				COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());

			compileArgument(ocx, { InsArgType::TypedValue, argDt, fcd->nArgs + 1ull });

			++fcd->nArgs;
		}

		prevToken();

		if (ocx.datatype != BC_DT_NONE)
			compileStatement("popc." + retDtStr + " : " + retVal);
	}

	void Compiler::compileSpecCallExtern(BC_OpCodeEx& ocx)
	{
		uint64_t argIndex = 0;

		removeNecessaryColon();

		compileArgAddress(ocx, { InsArgType::Address, BC_DT_NONE, argIndex++ });

		DelayedPush<BC_FuncCallData> fcd(*this);

		if (ocx.datatype != BC_DT_NONE)
		{
			removeNecessaryColon();
			compileArgument(ocx, { InsArgType::Address, ocx.datatype, argIndex++ });
		}

		while (nextToken().type == AsmToken::Type::Sep_Colon)
		{
			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());
			BC_Datatype argDt = BC_DatatypeFromString(currToken().value);
			fcd->argType.set(fcd->nArgs, argDt);

			if (nextToken().type != AsmToken::Type::Sep_Dot)
				COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());

			compileArgument(ocx, { InsArgType::TypedValue, argDt, fcd->nArgs + argIndex });

			++fcd->nArgs;
		}
		prevToken();
	}

	void Compiler::compileArgument(BC_OpCodeEx& ocx, const InsArgument& arg)
	{
		switch (arg.type)
		{
		case InsArgType::Address:
			compileArgAddress(ocx, arg); break;
		case InsArgType::Value:
		case InsArgType::TypedValue:
			compileArgValue(ocx, arg); break;
		case InsArgType::Datatype:
			compileArgDatatype(ocx, arg); break;
		default:
			COMPILER_THROW_ERROR(CompErrCode::UnknownInsArgType, "Cannot handle InsArgType '" + std::to_string((uint64_t)arg.type) + "'!");
		}
	}

	void Compiler::compileArgAddress(BC_OpCodeEx& ocx, const InsArgument& arg)
	{
		compileArgValue(ocx, arg);
	}

	void Compiler::compileArgValue(BC_OpCodeEx& ocx, const InsArgument& arg)
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
		generateTypeCell(tc, getsDereferenced);

		if (getsDereferenced)
			ocx.derefArg.set(arg.index);

		pushCode(&tc.cell, BC_DatatypeSize(tc.datatype));
	}

	void Compiler::generateTypeCell(TypeCell& tc, bool& getsDereferenced)
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
			generateTypeCellRegister(tc); break;
		case AsmToken::Type::Op_FP_Relative:
			generateTypeCellFPRelative(tc); break;
		case AsmToken::Type::Op_DT_Size:
			generateTypeCellDTSize(tc); break;
		case AsmToken::Type::Name:
			generateTypeCellName(tc); break;
		case AsmToken::Type::String:
			generateTypeCellString(tc); break;
		case AsmToken::Type::Float:
			generateTypeCellFloat(tc, getsDereferenced); break;
		case AsmToken::Type::Integer:
			generateTypeCellInteger(tc, getsDereferenced); break;
		default:
			COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::None, currToken());
		}
	}

	void Compiler::generateTypeCellRegister(TypeCell& tc)
	{
		auto reg = BC_RegisterFromString(nextToken().value);
		if (reg == BC_MEM_REG_NONE || reg == BC_MEM_REG_UNKNOWN)
			COMPILER_THROW_ERROR(CompErrCode::UnknownRegisterName, "Unknown register name '" + currToken().value + "'!");
		tc.cell.as_ADDR = BC_MemAddress(BC_MEM_BASE_REGISTER, reg);
	}

	void Compiler::generateTypeCellFPRelative(TypeCell& tc)
	{
		if (nextToken().type == AsmToken::Type::Op_DT_Size)
		{
			generateTypeCellDTSize(tc);

			auto base = BC_MEM_BASE_DYN_FRAME_ADD; // TODO: Add support for BC_MEM_BASE_DYN_FRAME_SUB

			tc.cell.as_ADDR = BC_MemAddress(base, tc.cell.as_U_64);

			return;
		}

		if (currToken().type == AsmToken::Type::Integer)
		{
			uint64_t offset = std::stoull(positiveString(currToken().value));

			bool isNegative = isNegativeString(currToken().value);
			auto base = isNegative ? BC_MEM_BASE_DYN_FRAME_SUB : BC_MEM_BASE_DYN_FRAME_ADD;

			tc.cell.as_ADDR = BC_MemAddress(base, offset);

			return;
		}

		COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Expected token of type 'Op_DT_Size' or 'Integer'! Got '" + AsmTokenTypeToString(currToken().type) + "' with value '" + currToken().value + "'!");
	}

	void Compiler::generateTypeCellDTSize(TypeCell& tc)
	{
		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		tc.cell.as_U_64 = BC_DatatypeSize(BC_DatatypeFromString(currToken().value));
		if (tc.cell.as_U_64 == 0)
			COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Unable to convert name '" + currToken().value + "' to datatype!");
	}

	void Compiler::generateTypeCellName(TypeCell& tc)
	{
		m_pModInfo->unresolvedSymbolRefs.push_back(
			{
				getScopedName(currToken().value),
				currCodeOffset(),
				tc.datatype
			}
		);
	}

	void Compiler::generateTypeCellString(TypeCell& tc)
	{
		if (tc.datatype != BC_DT_U_64)
			COMPILER_THROW_ERROR(CompErrCode::DatatypeMismatch, "Cannot use string in instruction with datatype '" + BC_DatatypeToString(tc.datatype) + "'!");

		tc.cell.as_ADDR = currStaticStackAddr();
		m_pModInfo->staticStack->push(currToken().value.c_str(), currToken().value.size() + 1);
	}

	void Compiler::generateTypeCellFloat(TypeCell& tc, bool getsDereferenced)
	{
		if (getsDereferenced)
			COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Cannot dereference argument of type 'float'!");
		switch (tc.datatype)
		{
		case BC_DT_F_32:
			tc.cell.as_F_32 = std::stof(currToken().value); break;
		case BC_DT_F_64:
			tc.cell.as_F_64 = std::stod(currToken().value); break;
		default:
			COMPILER_THROW_ERROR(CompErrCode::DatatypeMismatch, "Cannot use floating-point literal '" + currToken().value + "' in instruction with datatype '" + BC_DatatypeToString(tc.datatype) + "'!");
		}
	}

	void Compiler::generateTypeCellInteger(TypeCell& tc, bool getsDereferenced)
	{
		if (!getsDereferenced && (tc.datatype == BC_DT_F_32 || tc.datatype == BC_DT_F_64))
			COMPILER_THROW_ERROR(CompErrCode::DatatypeMismatch, "Cannot use integer '" + currToken().value + "' in instruction with datatype '" + BC_DatatypeToString(tc.datatype) + "'!");

		tc.cell.as_U_64 = std::stoull(positiveString(currToken().value));
		if (isNegativeString(currToken().value))
		{
			if (getsDereferenced)
				COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Cannot dereference literal with negative value!");

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
	}

	void Compiler::compileArgDatatype(BC_OpCodeEx& ocx, const InsArgument& arg)
	{
		auto dt = BC_DatatypeFromString(nextToken().value);
		if (dt == BC_DT_UNKNOWN)
			COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");

		pushCode(dt);
	}

	void Compiler::compileDirective()
	{
		switch (DirectiveIDFromString(nextToken().value))
		{
		case DirectiveID::None:
		case DirectiveID::Unknown:
			COMPILER_THROW_ERROR(CompErrCode::UnknownDirectiveID, "Unknown directive '" + currToken().value + "'!");
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
		case DirectiveID::FunctionExtern:
			return compileDirFunctionExtern();
		case DirectiveID::Local:
			return compileDirLocal();
		}

		COMPILER_THROW_ERROR(CompErrCode::UnknownDirectiveID, "Unknown directive '" + currToken().value + "'!");
	}

	void Compiler::compileDirLabel()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		addSymbol({ currToken().value, SymbolUsage::Address, currCodeAddr() });
	}

	void Compiler::compileDirAlias()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		Symbol symbol;
		symbol.name = currToken().value;

		removeNecessaryColon();

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
			COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Unexpected token'" + currToken().value + "' for alias value!");
		}

		bool getsDereferenced;
		generateTypeCell(tc, getsDereferenced);

		if (getsDereferenced)
			COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Usage of the deref operator is not allowed in the alias directive!");

		symbol.value = tc.cell;

		addSymbol(symbol);
	}

	void Compiler::compileDirStatic()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		std::string name = currToken().value;

		removeNecessaryColon();

		TypeCell tc;
		tc.datatype = BC_DT_U_64;

		nextToken();

		bool getsDereferenced;
		generateTypeCell(tc, getsDereferenced);

		if (getsDereferenced)
			COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Usage of the deref operator is not allowed in the static directive!");

		BC_MemCell mc;
		mc.as_ADDR = currStaticStackAddr();
		m_pModInfo->staticStack->resize(m_pModInfo->staticStack->size() + tc.cell.as_U_64);

		addSymbol({ name, SymbolUsage::Address, mc });
	}

	void Compiler::compileDirRequestModule()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::String)
			COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::String, currToken());

		std::string modName = currToken().value;

		m_pModInfo->requiredModules.push_back(modName);
	}

	void Compiler::compileDirScope()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		addScope(currToken().value);
	}

	void Compiler::compileDirEnd()
	{
		if (m_scopeList.size() == 0)
			COMPILER_THROW_ERROR(CompErrCode::AlreadyInGlobalScope, "Cannot end scope, already in global scope!");

		removeScope();
	}

	void Compiler::compileDirFunction()
	{
		bool hasDatatype = true;
		if (nextToken().type != AsmToken::Type::Sep_Dot)
		{
			hasDatatype = false;
			prevToken();
		}

		BC_Datatype dt = BC_DT_NONE;
		if (hasDatatype)
		{
			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			dt = BC_DatatypeFromString(currToken().value);
			if (dt == BC_DT_UNKNOWN)
				COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");
		}

		removeNecessaryColon();

		std::string funcName;
		{
			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			funcName = currToken().value;

			compileStatement("jmp : " + funcName + ">>SCOPE_END");

			addFuncScope(funcName);
			compileStatement("pushn : SCOPE_FUNC_LOCAL_SIZE");
		}

		if (hasDatatype)
		{
			removeNecessaryColon();

			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			std::string retName = currToken().value;

			compileStatement("#alias : " + retName + " : ~-" + std::to_string(2 * BC_DatatypeSize(BC_DT_U_64) + BC_DatatypeSize(dt)));
		}

		uint16_t paramOffset = 0;
		while (nextToken().type == AsmToken::Type::Sep_Colon)
		{
			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			BC_Datatype dt = BC_DatatypeFromString(currToken().value);
			if (dt == BC_DT_UNKNOWN)
				COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");

			if (nextToken().type != AsmToken::Type::Sep_Dot)
				COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());

			if (nextToken().type != AsmToken::Type::Name)
				COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			std::string valName = currToken().value;

			compileStatement("#alias : " + valName + " : ~+" + std::to_string(paramOffset));

			paramOffset += (uint16_t)BC_DatatypeSize(dt);
		}

		m_scopeList.back().paramSize = paramOffset;

		prevToken();
	}

	void Compiler::compileDirFunctionExtern()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		std::string name = currToken().value;

		compileStatement("#alias : " + name + " : \"" + getScopedName(name) + "\"");

		m_pModInfo->extensionRequired = true;
	}

	void Compiler::compileDirLocal()
	{
		if (m_scopeList.empty())
			COMPILER_THROW_ERROR(CompErrCode::InvalidScope, "The 'local' directive cannot be used in global scope!");
		if (!m_scopeList.back().isFuncScope)
			COMPILER_THROW_ERROR(CompErrCode::InvalidScope, "The 'local' directive cannot be used in normal scopes!");

		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		std::string name = currToken().value;

		TypeCell tc;
		tc.datatype = BC_DT_U_64;

		removeNecessaryColon();
		nextToken();

		bool getsDereferenced;
		generateTypeCell(tc, getsDereferenced);

		if (getsDereferenced)
			COMPILER_THROW_ERROR(CompErrCode::UnexpectedToken, "Usage of the deref operator is not allowed in the static directive!");

		BC_MemCell mc;
		mc.as_ADDR.base = BC_MEM_BASE_DYN_FRAME_ADD;
		mc.as_ADDR.addr = m_scopeList.back().paramSize + m_scopeList.back().localSize;

		addSymbol({ name, SymbolUsage::Address, mc });

		m_scopeList.back().localSize += tc.cell.as_U_64;
	}

	void Compiler::removeNecessaryColon()
	{
		if (nextToken().type != AsmToken::Type::Sep_Colon)
			COMPILER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Colon, currToken());
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

	void Compiler::addSymbol(Symbol symbol)
	{
		symbol.name = getScopedName(symbol.name);
		m_pModInfo->definedSymbols.push_back(symbol);
	}

	void Compiler::addScope(const std::string& name)
	{
		addSymbol({ name, SymbolUsage::Address, currCodeAddr() });

		m_scopeList.push_back({ name });
	}

	void Compiler::addFuncScope(const std::string& name)
	{
		addScope(name);
		m_scopeList.back().isFuncScope = true;
	}

	void Compiler::removeScope()
	{
		addSymbol({ "SCOPE_END", SymbolUsage::Address, currCodeAddr() });

		if (m_scopeList.back().isFuncScope)
		{
			BC_MemCell mc;
			mc.as_U_64 = m_scopeList.back().localSize;
			addSymbol({ "SCOPE_FUNC_LOCAL_SIZE", SymbolUsage::Value, mc });
		}

		m_scopeList.pop_back();
	}

	std::string Compiler::getScopedName(const std::string& name)
	{
		if (name.find(">>") == 0)
			return name;

		std::string fullName = ">>";
		for (auto& elem : m_scopeList)
			fullName.append(elem.name + ">>");
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

	uint64_t Compiler::currStaticStackOffset() const
	{
		return m_pModInfo->staticStack->size();
	}

	BC_MemAddress Compiler::currStaticStackAddr() const
	{
		return BC_MemAddress(BC_MEM_BASE_STATIC_STACK, currStaticStackOffset());
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
