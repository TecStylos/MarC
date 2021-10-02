#pragma once

#include <string>
#include "AssemblerOutput.h"

namespace MarC
{
	struct AssemblerInfo
	{
		std::string* pAssemblyCode;
		uint64_t nextCharToAssemble;
	};

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

	#define RETURN_WITH_ERROR(errCode, errText) { err = AssemblerError(errCode, bci.getErrLine(), errText, __LINE__, __FILE__); return false; }

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
		static bool assemble(BytecodeInfo& bci, AssemblerInfo& asmInfo, AssemblerError& err);
	private:
		static bool parseLine(BytecodeInfo& bci, AssemblerInfo& asmInfo, AssemblerError& err);
		static bool parseNumericArgument(BytecodeInfo& bci, AsmArgInfo& aai, AssemblerError& err);
		static bool isLiteral(const std::vector<std::string>& tokens);
		static bool parseLiteral(const BytecodeInfo& bci, std::vector<std::string>& tokens, bool deref, AsmArgInfo& aai, AssemblerError& err);
	private:
		static bool parse_insAlgebraicBinary(BytecodeInfo& bci, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insConvert(BytecodeInfo& bci, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPush(BytecodeInfo& bci, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPop(BytecodeInfo& bci, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPushCopy(BytecodeInfo& bci, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPopCopy(BytecodeInfo& bci, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPushFrame(BytecodeInfo& bci, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insPopFrame(BytecodeInfo& bci, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
		static bool parse_insExit(BytecodeInfo& bci, std::vector<std::string>& tokens, BC_OpCodeEx& ocx, AssemblerError& err);
	private:
		static void resolveUnresolvedRefs(BytecodeInfo& bci);
		static bool tokenizeLine(const BytecodeInfo& bci, AssemblerInfo& asmInfo, std::vector<std::string>& tokensOut, AssemblerError& err);
		static bool tokenizeNumericArgument(const BytecodeInfo& bci, const std::string& argument, std::vector<std::string>& tokensOut, AssemblerError& err);
		static bool isInstruction(const std::string& token);
		static std::pair<std::string, std::string> getOpCodePartsFromToken(const std::string& token);
		
		static bool literalToU64(const std::string& literalStr, uint64_t& output);
		static bool literalToF64(const std::string& literalStr, double& output);
	
		static bool isCorrectTokenNum(uint64_t expected, uint64_t provided, const BytecodeInfo& bci, AssemblerError& err);
		static bool isCorrectTokenNum(uint64_t expectedMin, uint64_t expectedMax, uint64_t provided, const BytecodeInfo& bci, AssemblerError& err);
	};
}