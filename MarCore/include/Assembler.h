#pragma once

#include <string>
#include "AssemblerOutput.h"

namespace MarC
{

	class AssemblerError
	{
	public:
		enum class Code
		{
			Success = 0,
			OpCodeUnknown,
			OpCodeMissing,
			DatatypeUnknown,
			DatatypeMissing,
			DatatypeMismatch,
			TokenUnexpectedNum,
			LiteralMissingAfterDerefOp,
			NumericArgumentBroken,
			NumericLiteralBroken,
			CharInvalid,
			NotImplemented,
			DirectiveUnknown,
			LabelAlreadyDefined,
		};
	public:
		AssemblerError() = default;
		AssemblerError(Code code, uint64_t errLine, const std::string& errText, uint64_t sysErrLine, const std::string& sysErrFile)
			: m_code(code), m_errLine(errLine), m_errText(errText), m_sysErrLine(sysErrLine), m_sysErrFile(sysErrFile)
		{}
	public:
		operator bool() const;
		const std::string& getText() const;
		std::string getMessage() const;
	private:
		Code m_code = Code::Success;
		uint64_t m_errLine = 0;
		std::string m_errText = "Success!";
		uint64_t m_sysErrLine = 0;
		std::string m_sysErrFile = "";
	};

	typedef AssemblerError::Code AsmErrCode;

	#define RETURN_WITH_ERROR(errCode, errText) { err = AssemblerError(errCode, mi.getErrLine(), errText, __LINE__, __FILE__); return false; }

	struct AsmArgInfo
	{
		std::string* pString;
		BC_OpCodeEx* pOcx;
		BC_TypeCell* pArg;
		uint64_t nthArg;
		BC_Datatype datatype;
		uint64_t offsetInInstruction;
	};

	class Assembler
	{
	public:
		Assembler(const std::string& asmCode, MemoryRef staticStack);
	public:
		bool assemble();
	public:
		ModuleInfoRef getModuleInfo();
	public:
		const AssemblerError& lastError() const;
	private:
		bool parseLine(ModuleInfo& mi, AssemblerError& err);
		static bool parseNumericArgument(ModuleInfo& mi, AsmArgInfo& aai, AssemblerError& err);
		static bool isLiteral(const std::vector<std::string>& tokens);
		static bool parseLiteral(const ModuleInfo& mi, std::vector<std::string>& tokens, bool deref, AsmArgInfo& aai, AssemblerError& err);
	private:
		static bool parseInstruction(ModuleInfo& mi, std::vector<std::string>& tokens, AssemblerError& err);
		static bool parse_insAlgebraicBinary(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insConvert(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPush(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPop(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPushCopy(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPopCopy(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPushFrame(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPopFrame(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insJump(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insExit(ModuleInfo& mi, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
	private:
		static bool parseDirective(ModuleInfo& mi, std::vector<std::string>& tokens, AssemblerError& err);
		static bool parse_dirLabel(ModuleInfo& mi, std::vector<std::string>& tokens, AssemblerError& err);
	private:
		bool tokenizeLine(const ModuleInfo& mi, std::vector<std::string>& tokensOut, AssemblerError& err);
		static bool tokenizeNumericArgument(const ModuleInfo& mi, const std::string& argument, std::vector<std::string>& tokensOut, AssemblerError& err);
		static bool isInstruction(const std::string& token);
		static bool isDirective(const std::string& token);
		static std::pair<std::string, std::string> getOpCodePartsFromToken(const std::string& token);
		
		static bool literalToU64(const std::string& literalStr, uint64_t& output);
		static bool literalToF64(const std::string& literalStr, double& output);
	
		static bool isCorrectTokenNum(uint64_t expected, uint64_t provided, const ModuleInfo& mi, AssemblerError& err);
		static bool isCorrectTokenNum(uint64_t expectedMin, uint64_t expectedMax, uint64_t provided, const ModuleInfo& mi, AssemblerError& err);
	private:
		static BC_MemAddress currCodeAddr(ModuleInfo& mi);
	private:
		std::string m_asmCode;
		MemoryRef m_staticStack;
		AssemblerError m_lastErr;
		ModuleInfoRef m_pModInfo;
		uint64_t m_nextCharToAssemble = 0;
	};
}