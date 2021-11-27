#include "Disassembler.h"

namespace MarC
{
	Disassembler::InstructionParser::InstructionParser(const void* pInstruction)
		: m_pIns(pInstruction), m_pInsOrig(pInstruction)
	{}

	uint64_t Disassembler::InstructionParser::insSize() const
	{
		return ((char*)m_pIns - (char*)m_pInsOrig);
	}

	DisAsmInsInfo Disassembler::disassemble(const void* pInstruction)
	{
		InstructionParser ip(pInstruction);
		DisAsmInsInfo daii;

		daii.ocx = ip.read<BC_OpCodeEx>();

		auto& layout = InstructionLayoutFromOpCode(daii.ocx.opCode);

		if (layout.needsCustomImplementation)
		{
			disassembleSpecializedInstruction(daii, ip);
		}
		else
		{
			for (auto& arg : layout.args)
				disassembleArgument(daii, ip, arg);
		}

		daii.rawData.resize(ip.insSize());
		memcpy(daii.rawData.data(), pInstruction, daii.rawData.size());

		return daii;
	}

	void Disassembler::disassembleArgument(DisAsmInsInfo& daii, InstructionParser& ip, const InsArgument& arg)
	{
		DisAsmArg daa;

		switch (arg.type)
		{
		case InsArgType::Address:
			daa = disassembleArgAddress(daii, ip, arg); break;
		case InsArgType::Value:
		case InsArgType::TypedValue:
		case InsArgType::Datatype:
			daa = disassembleArgValue(daii, ip, arg); break;
		}

		daii.args.push_back(daa);
	}

	DisAsmArg Disassembler::disassembleArgAddress(DisAsmInsInfo& daii, InstructionParser& ip, const InsArgument& arg)
	{
		return disassembleArgValue(daii, ip, arg);
	}

	DisAsmArg Disassembler::disassembleArgValue(DisAsmInsInfo& daii, InstructionParser& ip, const InsArgument& arg)
	{
		DisAsmArg daa;
		daa.getsDereferenced = daii.ocx.derefArg.get(arg.index);

		switch (arg.type)
		{
		case InsArgType::Address: daa.value.datatype = BC_DT_U_64; break;
		case InsArgType::TypedValue: daa.value.datatype = arg.datatype; break;
		case InsArgType::Value: daa.value.datatype = daii.ocx.datatype; break;
		case InsArgType::Datatype: daa.value.datatype = BC_DT_DATATYPE; break;
		}

		switch (daa.getsDereferenced ? BC_DT_U_64 : daa.value.datatype)
		{
		case BC_DT_I_8: daa.value.cell.as_I_8 = ip.read<int8_t>(); break;
		case BC_DT_I_16: daa.value.cell.as_I_16 = ip.read<int16_t>(); break;
		case BC_DT_I_32: daa.value.cell.as_I_32 = ip.read<int32_t>(); break;
		case BC_DT_I_64: daa.value.cell.as_I_64 = ip.read<int64_t>(); break;
		case BC_DT_U_8: daa.value.cell.as_U_8 = ip.read<uint8_t>(); break;
		case BC_DT_U_16: daa.value.cell.as_U_16 = ip.read<uint16_t>(); break;
		case BC_DT_U_32: daa.value.cell.as_U_32 = ip.read<uint32_t>(); break;
		case BC_DT_U_64: daa.value.cell.as_U_64 = ip.read<uint64_t>(); break;
		case BC_DT_F_32: daa.value.cell.as_F_32 = ip.read<float>(); break;
		case BC_DT_F_64: daa.value.cell.as_F_64 = ip.read<double>(); break;
		case BC_DT_DATATYPE: daa.value.cell.as_Datatype = ip.read<BC_Datatype>(); break;
		}

		return daa;
	}

	void Disassembler::disassembleSpecializedInstruction(DisAsmInsInfo& daii, InstructionParser& ip)
	{
		switch (daii.ocx.opCode)
		{
		case BC_OC_CALL:
			return disassembleSpecCall(daii, ip);
		case BC_OC_CALL_EXTERN:
			return disassembleSpecCallExtern(daii, ip);
		}
	}

	void Disassembler::disassembleSpecCall(DisAsmInsInfo& daii, InstructionParser& ip)
	{
		daii.args.push_back(disassembleArgAddress(daii, ip, { InsArgType::Address, BC_DT_NONE, 0 }));

		const BC_FuncCallData& fcd = ip.read<BC_FuncCallData>();

		for (uint8_t i = 0; i < fcd.nArgs; ++i)
			disassembleArgument(daii, ip, { InsArgType::TypedValue, fcd.argType.get(i), (uint64_t)i + 1 });
	}

	void Disassembler::disassembleSpecCallExtern(DisAsmInsInfo& daii, InstructionParser& ip)
	{
		uint64_t argIndex = 0;
		daii.args.push_back(disassembleArgAddress(daii, ip, { InsArgType::Address, BC_DT_NONE, argIndex++ }));

		if (daii.ocx.datatype != BC_DT_NONE)
			disassembleArgument(daii, ip, { InsArgType::Address, daii.ocx.datatype, argIndex++ });

		const BC_FuncCallData& fcd = ip.read<BC_FuncCallData>();

		for (uint8_t i = 0; i < fcd.nArgs; ++i)
			disassembleArgument(daii, ip, { InsArgType::TypedValue, fcd.argType.get(i), argIndex + i });
	}
}