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

	#define RETURN_WITH_ERROR(errCode, errText) { m_lastErr = AssemblerError(errCode, m_pModInfo->getErrLine(), errText, __LINE__, __FILE__); return false; }

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
		bool parseLine();
		bool parseNumericArgument(AsmArgInfo& aai);
		static bool isLiteral(const std::vector<std::string>& tokens);
		bool parseLiteral(std::vector<std::string>& tokens, bool deref, AsmArgInfo& aai);
	private:
		bool parseInstruction(std::vector<std::string>& tokens);
		bool parse_insAlgebraicBinary(std::vector<std::string>& tokens, BC_OpCodeEx& ocx);
		bool parse_insConvert(std::vector<std::string>& tokens, BC_OpCodeEx& ocx);
		bool parse_insPush(std::vector<std::string>& tokens, BC_OpCodeEx& ocx);
		bool parse_insPop(std::vector<std::string>& tokens, BC_OpCodeEx& ocx);
		bool parse_insPushCopy(std::vector<std::string>& tokens, BC_OpCodeEx& ocx);
		bool parse_insPopCopy(std::vector<std::string>& tokens, BC_OpCodeEx& ocx);
		bool parse_insPushFrame(std::vector<std::string>& tokens, BC_OpCodeEx& ocx);
		bool parse_insPopFrame(std::vector<std::string>& tokens, BC_OpCodeEx& ocx);
		bool parse_insJump(std::vector<std::string>& tokens, BC_OpCodeEx& ocx);
		bool parse_insExit(std::vector<std::string>& tokens, BC_OpCodeEx& ocx);
	private:
		bool parseDirective(std::vector<std::string>& tokens);
		bool parse_dirLabel(std::vector<std::string>& tokens);
	private:
		bool tokenizeLine(std::vector<std::string>& tokensOut);
		bool tokenizeNumericArgument(const const std::string& argument, std::vector<std::string>& tokensOut);
		static bool isInstruction(const std::string& token);
		static bool isDirective(const std::string& token);
		static std::pair<std::string, std::string> getOpCodePartsFromToken(const std::string& token);
		
		static bool literalToU64(const std::string& literalStr, uint64_t& output);
		static bool literalToF64(const std::string& literalStr, double& output);
	
		bool isCorrectTokenNum(uint64_t expected, uint64_t provided);
		bool isCorrectTokenNum(uint64_t expectedMin, uint64_t expectedMax, uint64_t provided);
	private:
		BC_MemAddress currCodeAddr();
	private:
		std::string m_asmCode;
		MemoryRef m_staticStack;
		AssemblerError m_lastErr;
		ModuleInfoRef m_pModInfo;
		uint64_t m_nextCharToAssemble = 0;
	};
}