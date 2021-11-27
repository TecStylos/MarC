#pragma once

#include "DisAsmTypes.h"

namespace MarC
{
	class Disassembler
	{
	private:
		class InstructionParser
		{
		public:
			InstructionParser() = delete;
			InstructionParser(const void* pInstruction);
		public:
			template <typename T>
			const T& read();
			uint64_t insSize() const;
		private:
			const void* m_pInsOrig;
			const void* m_pIns;
		};
	public:
		static DisAsmInsInfo disassemble(const void* pInstruction);
	private:
		static void disassembleArgument(DisAsmInsInfo& daii, InstructionParser& ip, const InsArgument& arg);
		static DisAsmArg disassembleArgAddress(DisAsmInsInfo& daii, InstructionParser& ip, const InsArgument& arg);
		static DisAsmArg disassembleArgValue(DisAsmInsInfo& daii, InstructionParser& ip, const InsArgument& arg);
		static void disassembleSpecializedInstruction(DisAsmInsInfo& daii, InstructionParser& ip);
		static void disassembleSpecCall(DisAsmInsInfo& daii, InstructionParser& ip);
		static void disassembleSpecCallExtern(DisAsmInsInfo& daii, InstructionParser& ip);
	};

	template <typename T>
	const T& Disassembler::InstructionParser::read()
	{
		const T& temp = *(const T*)m_pIns;
		m_pIns = (char*)m_pIns + sizeof(T);
		return temp;
	}
}