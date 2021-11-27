#include "Assembler.h"

#include "AsmTokenizer.h"
#include "VirtualAsmTokenList.h"

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
		return
			"ERROR ON L: " + std::to_string(m_line) + " C: " + std::to_string(m_column) + "\n" +
			"  -> " + getText();
	}

	Assembler::Assembler(const AsmTokenListRef tokenList, const std::string& moduleName)
		: m_pTokenList(tokenList)
	{
		m_pModInfo = ModuleInfo::create();
		m_pModInfo->moduleName = moduleName;
	}

	ModuleInfoRef Assembler::getModuleInfo()
	{
		return m_pModInfo;
	}

	const AssemblerError& Assembler::lastError() const
	{
		return m_lastErr;
	}

	void Assembler::resetError()
	{
		m_lastErr = AssemblerError();
	}

	void Assembler::backup()
	{
		m_pModInfo->backup();
		m_backupNextTokenToCompile = m_nextTokenToCompile;
	}
	
	void Assembler::recover()
	{
		m_pModInfo->recover();
		m_nextTokenToCompile = m_backupNextTokenToCompile;
	}

	bool Assembler::assemble()
	{
		resetError();

		backup();

		try
		{
			while (!isEndOfCode())
				assembleStatement();
		}
		catch (AssemblerError& err)
		{
			m_lastErr = err;
			recover();
		}

		return !m_lastErr;
	}

	void Assembler::assembleStatement()
	{
		if (isInstructionLike())
			assembleInstruction();
		if (isDirectiveLike())
			assembleDirective();

		uint64_t nNewlines = 0;
		while (nextToken().type == AsmToken::Type::Sep_Newline)
			++nNewlines;
		if (nNewlines == 0 && !isEndOfCode())
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Newline, currToken());
	}

	void Assembler::assembleStatement(const std::string& statement)
	{
		AsmTokenizer tok(statement);
		if (!tok.tokenize())
			ASSEMBLER_THROW_ERROR(AsmErrCode::InternalError, "Unable to tokenize internal statement '" + statement +"'!");
		
		{
			VirtualAsmTokenList vtl(*this, tok.getTokenList());

			assembleStatement();
		}
	}

	void Assembler::assembleInstruction()
	{
		BC_OpCodeEx ocx;

		ocx.opCode = BC_OpCodeFromString(currToken().value);
		if (ocx.opCode == BC_OC_NONE || ocx.opCode == BC_OC_UNKNOWN)
			ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to opCode!");

		auto& layout = InstructionLayoutFromOpCode(ocx.opCode);

		if (layout.insDt != InsDt::None)
		{
			if (nextToken().type == AsmToken::Type::Sep_Dot)
			{
				ocx.datatype = BC_DatatypeFromString(nextToken().value);
				if (ocx.datatype == BC_DT_UNKNOWN)
					ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");
			}
			else
			{
				if (layout.insDt == InsDt::Required)
					ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());
				prevToken();
			}
		}

		DelayedPush<BC_OpCodeEx> ocxDelayed(*this, ocx);

		if (layout.needsCustomImplementation)
		{
			assembleSpecializedInstruction(ocx);
			return;
		}

		for (auto& arg : layout.args)
		{
			removeNecessaryColon();

			assembleArgument(ocx, arg);
		}
	}

	void Assembler::assembleSpecializedInstruction(BC_OpCodeEx& ocx)
	{
		switch (ocx.opCode)
		{
		case BC_OC_CALL:
			return assembleSpecCall(ocx);
		case BC_OC_CALL_EXTERN:
			return assembleSpecCallExtern(ocx);
		}

		ASSEMBLER_THROW_ERROR(AsmErrCode::InternalError, "OpCode '" + BC_OpCodeToString(ocx.opCode) + "' is not specialized!");
	}

	void Assembler::assembleSpecCall(BC_OpCodeEx& ocx)
	{
		removeNecessaryColon();

		assembleArgValue(ocx, { InsArgType::Address, BC_DT_NONE, 0 });

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
				ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());
			BC_Datatype argDt = BC_DatatypeFromString(currToken().value);
			fcd->argType.set(fcd->nArgs, argDt);

			if (nextToken().type != AsmToken::Type::Sep_Dot)
				ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());

			assembleArgument(ocx, { InsArgType::TypedValue, argDt, fcd->nArgs + 1ull });

			++fcd->nArgs;
		}

		prevToken();

		if (ocx.datatype != BC_DT_NONE)
			assembleStatement("popc." + retDtStr + " : " + retVal);
	}

	void Assembler::assembleSpecCallExtern(BC_OpCodeEx& ocx)
	{
		uint64_t argIndex = 0;

		removeNecessaryColon();

		assembleArgValue(ocx, { InsArgType::Address, BC_DT_NONE, argIndex++ });

		DelayedPush<BC_FuncCallData> fcd(*this);

		if (ocx.datatype != BC_DT_NONE)
		{
			removeNecessaryColon();
			assembleArgument(ocx, { InsArgType::Address, ocx.datatype, argIndex++ });
		}

		while (nextToken().type == AsmToken::Type::Sep_Colon)
		{
			if (nextToken().type != AsmToken::Type::Name)
				ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());
			BC_Datatype argDt = BC_DatatypeFromString(currToken().value);
			fcd->argType.set(fcd->nArgs, argDt);

			if (nextToken().type != AsmToken::Type::Sep_Dot)
				ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());

			assembleArgument(ocx, { InsArgType::TypedValue, argDt, fcd->nArgs + argIndex });

			++fcd->nArgs;
		}
		prevToken();
	}

	void Assembler::assembleArgument(BC_OpCodeEx& ocx, const InsArgument& arg)
	{
		switch (arg.type)
		{
		case InsArgType::Address:
		case InsArgType::Value:
		case InsArgType::TypedValue:
			assembleArgValue(ocx, arg); break;
		case InsArgType::Datatype:
			assembleArgDatatype(ocx, arg); break;
		default:
			ASSEMBLER_THROW_ERROR(AsmErrCode::UnknownInsArgType, "Cannot handle InsArgType '" + std::to_string((uint64_t)arg.type) + "'!");
		}
	}

	void Assembler::assembleArgValue(BC_OpCodeEx& ocx, const InsArgument& arg)
	{
		nextToken();

		TypeCell tc;
		switch (arg.type)
		{
		case InsArgType::Address: tc.datatype = BC_DT_U_64; break;
		case InsArgType::TypedValue: tc.datatype = arg.datatype; break;
		case InsArgType::Value: tc.datatype = ocx.datatype; break;
		case InsArgType::Datatype: tc.datatype = BC_DT_DATATYPE; break;
		}

		bool getsDereferenced;
		generateTypeCell(tc, getsDereferenced);

		if (getsDereferenced)
			ocx.derefArg.set(arg.index);

		pushCode(&tc.cell, BC_DatatypeSize(tc.datatype));
	}

	void Assembler::generateTypeCell(TypeCell& tc, bool& getsDereferenced)
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
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::None, currToken());
		}
	}

	void Assembler::generateTypeCellRegister(TypeCell& tc)
	{
		auto reg = BC_RegisterFromString(nextToken().value);
		if (reg == BC_MEM_REG_NONE || reg == BC_MEM_REG_UNKNOWN)
			ASSEMBLER_THROW_ERROR(AsmErrCode::UnknownRegisterName, "Unknown register name '" + currToken().value + "'!");
		tc.cell.as_ADDR = BC_MemAddress(BC_MEM_BASE_REGISTER, reg);
	}

	void Assembler::generateTypeCellFPRelative(TypeCell& tc)
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

		ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Expected token of type 'Op_DT_Size' or 'Integer'! Got '" + AsmTokenTypeToString(currToken().type) + "' with value '" + currToken().value + "'!");
	}

	void Assembler::generateTypeCellDTSize(TypeCell& tc)
	{
		if (nextToken().type != AsmToken::Type::Name)
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		tc.cell.as_U_64 = BC_DatatypeSize(BC_DatatypeFromString(currToken().value));
		if (tc.cell.as_U_64 == 0)
			ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Unable to convert name '" + currToken().value + "' to datatype!");
	}

	void Assembler::generateTypeCellName(TypeCell& tc)
	{
		if (tc.datatype == BC_DT_DATATYPE)
		{
			tc.cell.as_Datatype = BC_DatatypeFromString(currToken().value);
			if (tc.cell.as_Datatype == BC_DT_UNKNOWN)
				ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");
			return;
		}

		m_pModInfo->unresolvedSymbolRefs.push_back(
			{
				getScopedName(currToken().value),
				currCodeOffset(),
				tc.datatype
			}
		);
	}

	void Assembler::generateTypeCellString(TypeCell& tc)
	{
		if (tc.datatype != BC_DT_ADDR)
			ASSEMBLER_THROW_ERROR(AsmErrCode::DatatypeMismatch, "Cannot use string in instruction with datatype '" + BC_DatatypeToString(tc.datatype) + "'!");

		tc.cell.as_ADDR = currStaticStackAddr();
		m_pModInfo->staticStack->push(currToken().value.c_str(), currToken().value.size() + 1);
	}

	void Assembler::generateTypeCellFloat(TypeCell& tc, bool getsDereferenced)
	{
		if (getsDereferenced)
			ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Cannot dereference argument of type 'float'!");
		switch (tc.datatype)
		{
		case BC_DT_F_32:
			tc.cell.as_F_32 = std::stof(currToken().value); break;
		case BC_DT_F_64:
			tc.cell.as_F_64 = std::stod(currToken().value); break;
		default:
			ASSEMBLER_THROW_ERROR(AsmErrCode::DatatypeMismatch, "Cannot use floating-point literal '" + currToken().value + "' in instruction with datatype '" + BC_DatatypeToString(tc.datatype) + "'!");
		}
	}

	void Assembler::generateTypeCellInteger(TypeCell& tc, bool getsDereferenced)
	{
		if (!getsDereferenced && (tc.datatype == BC_DT_F_32 || tc.datatype == BC_DT_F_64))
			ASSEMBLER_THROW_ERROR(AsmErrCode::DatatypeMismatch, "Cannot use integer '" + currToken().value + "' in instruction with datatype '" + BC_DatatypeToString(tc.datatype) + "'!");

		tc.cell.as_U_64 = std::stoull(positiveString(currToken().value));
		if (isNegativeString(currToken().value))
		{
			if (getsDereferenced)
				ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Cannot dereference literal with negative value!");

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

	void Assembler::assembleArgDatatype(BC_OpCodeEx& ocx, const InsArgument& arg)
	{
		auto dt = BC_DatatypeFromString(nextToken().value);
		if (dt == BC_DT_UNKNOWN)
			ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");

		pushCode(dt);
	}

	void Assembler::assembleDirective()
	{
		switch (DirectiveIDFromString(nextToken().value))
		{
		case DirectiveID::None:
		case DirectiveID::Unknown:
			ASSEMBLER_THROW_ERROR(AsmErrCode::UnknownDirectiveID, "Unknown directive '" + currToken().value + "'!");
			break;
		case DirectiveID::Label:
			return assembleDirLabel();
		case DirectiveID::Alias:
			return assembleDirAlias();
		case DirectiveID::Static:
			return assembleDirStatic();
		case DirectiveID::RequestModule:
			return assembleDirRequestModule();
		case DirectiveID::Scope:
			return assembleDirScope();
		case DirectiveID::End:
			return assembleDirEnd();
		case DirectiveID::Function:
			return assembleDirFunction();
		case DirectiveID::FunctionExtern:
			return assembleDirFunctionExtern();
		case DirectiveID::Local:
			return assembleDirLocal();
		case DirectiveID::MandatoryPermission:
			return assembleDirMandatoryPermission();
		case DirectiveID::OptionalPermission:
			return assembleDirOptionalPermission();
		}

		ASSEMBLER_THROW_ERROR(AsmErrCode::UnknownDirectiveID, "Unknown directive '" + currToken().value + "'!");
	}

	void Assembler::assembleDirLabel()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		addSymbol({ currToken().value, SymbolUsage::Address, currCodeAddr() });
	}

	void Assembler::assembleDirAlias()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		Symbol symbol;
		symbol.name = currToken().value;

		removeNecessaryColon();

		TypeCell tc;
		switch (nextToken().type)
		{
		case AsmToken::Type::Op_Register:
			tc.datatype = BC_DT_ADDR;
			symbol.usage = SymbolUsage::Address;
			break;
		case AsmToken::Type::Op_FP_Relative:
			tc.datatype = BC_DT_ADDR;
			symbol.usage = SymbolUsage::Address;
			break;
		case AsmToken::Type::String:
			tc.datatype = BC_DT_ADDR;
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
		case AsmToken::Type::Name:
			addUnresolvedSymbol({ symbol.name, currToken().value });
			return;
		default:
			ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Unexpected token'" + currToken().value + "' for alias value!");
		}

		bool getsDereferenced;
		generateTypeCell(tc, getsDereferenced);

		if (getsDereferenced)
			ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Usage of the deref operator is not allowed in the alias directive!");

		symbol.value = tc.cell;

		addSymbol(symbol);
	}

	void Assembler::assembleDirStatic()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		std::string name = currToken().value;

		removeNecessaryColon();

		TypeCell tc;
		tc.datatype = BC_DT_U_64;

		nextToken();

		bool getsDereferenced;
		generateTypeCell(tc, getsDereferenced);

		if (getsDereferenced)
			ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Usage of the deref operator is not allowed in the static directive!");

		BC_MemCell mc;
		mc.as_ADDR = currStaticStackAddr();
		m_pModInfo->staticStack->resize(m_pModInfo->staticStack->size() + tc.cell.as_U_64);

		addSymbol({ name, SymbolUsage::Address, mc });
	}

	void Assembler::assembleDirRequestModule()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::String)
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::String, currToken());

		std::string modName = currToken().value;

		m_pModInfo->requiredModules.push_back(modName);
	}

	void Assembler::assembleDirScope()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		addScope(currToken().value);
	}

	void Assembler::assembleDirEnd()
	{
		if (m_scopeList.size() == 0)
			ASSEMBLER_THROW_ERROR(AsmErrCode::AlreadyInGlobalScope, "Cannot end scope, already in global scope!");

		removeScope();
	}

	void Assembler::assembleDirFunction()
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
				ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			dt = BC_DatatypeFromString(currToken().value);
			if (dt == BC_DT_UNKNOWN)
				ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");
		}

		removeNecessaryColon();

		std::string funcName;
		{
			if (nextToken().type != AsmToken::Type::Name)
				ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			funcName = currToken().value;

			assembleStatement("jmp : " + funcName + ">>SCOPE_END");

			addFuncScope(funcName);
			assembleStatement("pushn : SCOPE_FUNC_LOCAL_SIZE");
		}

		if (hasDatatype)
		{
			removeNecessaryColon();

			if (nextToken().type != AsmToken::Type::Name)
				ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			std::string retName = currToken().value;

			assembleStatement("#alias : " + retName + " : ~-" + std::to_string(2 * BC_DatatypeSize(BC_DT_U_64) + BC_DatatypeSize(dt)));
		}

		uint16_t paramOffset = 0;
		while (nextToken().type == AsmToken::Type::Sep_Colon)
		{
			if (nextToken().type != AsmToken::Type::Name)
				ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			BC_Datatype dt = BC_DatatypeFromString(currToken().value);
			if (dt == BC_DT_UNKNOWN)
				ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Unable to convert token '" + currToken().value + "' to datatype!");

			if (nextToken().type != AsmToken::Type::Sep_Dot)
				ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Dot, currToken());

			if (nextToken().type != AsmToken::Type::Name)
				ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

			std::string valName = currToken().value;

			assembleStatement("#alias : " + valName + " : ~+" + std::to_string(paramOffset));

			paramOffset += (uint16_t)BC_DatatypeSize(dt);
		}

		m_scopeList.back().paramSize = paramOffset;

		prevToken();
	}

	void Assembler::assembleDirFunctionExtern()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		std::string name = currToken().value;

		assembleStatement("#alias : " + name + " : \"" + getScopedName(name) + "\"");

		m_pModInfo->extensionRequired = true;
	}

	void Assembler::assembleDirLocal()
	{
		if (m_scopeList.empty())
			ASSEMBLER_THROW_ERROR(AsmErrCode::InvalidScope, "The 'local' directive cannot be used in global scope!");
		if (!m_scopeList.back().isFuncScope)
			ASSEMBLER_THROW_ERROR(AsmErrCode::InvalidScope, "The 'local' directive cannot be used in normal scopes!");

		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		std::string name = currToken().value;

		TypeCell tc;
		tc.datatype = BC_DT_U_64;

		removeNecessaryColon();
		nextToken();

		bool getsDereferenced;
		generateTypeCell(tc, getsDereferenced);

		if (getsDereferenced)
			ASSEMBLER_THROW_ERROR(AsmErrCode::UnexpectedToken, "Usage of the deref operator is not allowed in the static directive!");

		BC_MemCell mc;
		mc.as_ADDR.base = BC_MEM_BASE_DYN_FRAME_ADD;
		mc.as_ADDR.addr = m_scopeList.back().paramSize + m_scopeList.back().localSize;

		addSymbol({ name, SymbolUsage::Address, mc });

		m_scopeList.back().localSize += tc.cell.as_U_64;
	}

	void Assembler::assembleDirMandatoryPermission()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		std::string name = currToken().value;

		m_pModInfo->mandatoryPermissions.push_back(name);
	}

	void Assembler::assembleDirOptionalPermission()
	{
		removeNecessaryColon();

		if (nextToken().type != AsmToken::Type::Name)
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Name, currToken());

		std::string name = currToken().value;

		m_pModInfo->optionalPermissions.push_back(name);
	}

	void Assembler::removeNecessaryColon()
	{
		if (nextToken().type != AsmToken::Type::Sep_Colon)
			ASSEMBLER_THROW_ERROR_UNEXPECTED_TOKEN(AsmToken::Type::Sep_Colon, currToken());
	}

	std::string Assembler::getArgAsString()
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

	void Assembler::addSymbol(Symbol symbol)
	{
		symbol.name = getScopedName(symbol.name);
		m_pModInfo->definedSymbols.push_back(symbol);
	}

	void Assembler::addScope(const std::string& name)
	{
		addSymbol({ name, SymbolUsage::Address, currCodeAddr() });

		m_scopeList.push_back({ name });
	}

	void Assembler::addFuncScope(const std::string& name)
	{
		addScope(name);
		m_scopeList.back().isFuncScope = true;
	}

	void Assembler::removeScope()
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

	std::string Assembler::getScopedName(const std::string& name)
	{
		if (name.find(">>") == 0)
			return name;

		std::string fullName = ">>";
		for (auto& elem : m_scopeList)
			fullName.append(elem.name + ">>");
		fullName.append(name);

		return fullName;
	}

	void Assembler::addUnresolvedSymbol(UnresolvedSymbol unresSymbol)
	{
		unresSymbol.name = getScopedName(unresSymbol.name);
		unresSymbol.refName = getScopedName(unresSymbol.refName);
		m_pModInfo->unresolvedSymbols.push_back(unresSymbol);
	}

	bool Assembler::isInstructionLike()
	{
		return currToken().type == AsmToken::Type::Name;
	}

	bool Assembler::isDirectiveLike()
	{
		return currToken().type == AsmToken::Type::Op_Directive;
	}

	const AsmToken& Assembler::nextToken()
	{
		++m_nextTokenToCompile;
		return currToken();
	}

	const AsmToken& Assembler::prevToken()
	{
		--m_nextTokenToCompile;
		return currToken();
	}

	const AsmToken& Assembler::currToken() const
	{
		return (*m_pTokenList)[m_nextTokenToCompile];
	}

	bool Assembler::isEndOfCode() const
	{
		return currToken().type == AsmToken::Type::END_OF_CODE;
	}

	void Assembler::pushCode(const void* data, uint64_t size)
	{
		m_pModInfo->codeMemory->push(data, size);
	}

	void Assembler::writeCode(const void* data, uint64_t size, uint64_t offset)
	{
		m_pModInfo->codeMemory->write(data, size, offset);
	}

	uint64_t Assembler::currCodeOffset() const
	{
		return m_pModInfo->codeMemory->size();
	}

	BC_MemAddress Assembler::currCodeAddr() const
	{
		return BC_MemAddress(BC_MEM_BASE_CODE_MEMORY, 0, currCodeOffset());
	}

	uint64_t Assembler::currStaticStackOffset() const
	{
		return m_pModInfo->staticStack->size();
	}

	BC_MemAddress Assembler::currStaticStackAddr() const
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
