#include "types/DisAsmTypes.h"

namespace MarC
{
	std::set<Symbol>::const_iterator getSymbolForAddress(BC_MemAddress addr, const std::set<Symbol>& symbols, const std::string& scopeName)
	{
		auto it = symbols.begin();
		while (it != symbols.end())
		{
			if (it->usage == SymbolUsage::Address &&
				it->value.as_ADDR == addr &&
				it->name.find(scopeName) == 0
				)
				break;

			++it;
		}

		return it;
	}

	std::string DisAsmInsInfoToString(const DisAsmInsInfo& daii, const std::set<Symbol>& symbols)
	{
		std::string insStr;

		insStr.append(BC_OpCodeToString(daii.ocx.opCode));

		if (daii.ocx.datatype != BC_DT_NONE)
			insStr.append("." + BC_DatatypeToString(daii.ocx.datatype));

		if (!daii.args.empty())
			insStr.append(" : ");

		for (uint64_t i = 0; i < daii.args.size(); ++i)
		{
			auto& arg = daii.args[i];

			switch (arg.argType)
			{
			case InsArgType::None:
				break;
			case InsArgType::Datatype:
				insStr.append(BC_DatatypeToString(arg.value.cell.as_Datatype));
				break;
			case InsArgType::TypedValue:
				insStr.append(BC_DatatypeToString(arg.value.datatype) + ".");
				[[fallthrough]];
			case InsArgType::Value:
				if (arg.derefCount)
				{
					for (DerefCount dc = 0; dc < arg.derefCount; ++dc)
						insStr.append("@");
				}
				else
				{
					switch (arg.value.datatype)
					{
					case BC_DT_NONE: insStr.append("<NONE>"); break;
					case BC_DT_UNKNOWN: insStr.append("<UNKNOWN>"); break;
					case BC_DT_I_8:  insStr.append(std::to_string(arg.value.cell.as_I_8));  break;
					case BC_DT_I_16: insStr.append(std::to_string(arg.value.cell.as_I_16)); break;
					case BC_DT_I_32: insStr.append(std::to_string(arg.value.cell.as_I_32)); break;
					case BC_DT_I_64: insStr.append(std::to_string(arg.value.cell.as_I_64)); break;
					case BC_DT_U_8:  insStr.append(std::to_string(arg.value.cell.as_U_8));  break;
					case BC_DT_U_16: insStr.append(std::to_string(arg.value.cell.as_U_16)); break;
					case BC_DT_U_32: insStr.append(std::to_string(arg.value.cell.as_U_32)); break;
					case BC_DT_U_64: insStr.append(std::to_string(arg.value.cell.as_U_64)); break;
					case BC_DT_F_32: insStr.append(std::to_string(arg.value.cell.as_F_32)); break;
					case BC_DT_F_64: insStr.append(std::to_string(arg.value.cell.as_F_64)); break;
					case BC_DT_ADDR:
					{
						auto it = getSymbolForAddress(arg.value.cell.as_ADDR, symbols);
						if (it != symbols.end())
							insStr.append(it->name);
						else
							insStr.append(BC_MemAddressToString(arg.value.cell.as_ADDR));
						break;
					}
					case BC_DT_DATATYPE: insStr.append(BC_DatatypeToString(arg.value.cell.as_Datatype)); break;
					}
					break;
				}
				[[fallthrough]];
			case InsArgType::Address:
				auto it = getSymbolForAddress(arg.value.cell.as_ADDR, symbols);
				if (it != symbols.end())
					insStr.append(it->name);
				else
					insStr.append(BC_MemAddressToString(arg.value.cell.as_ADDR));
				break;
			}

			if (i + 1 < daii.args.size())
				insStr.append(" : ");

			if (i == 0 && (daii.ocx.opCode == BC_OC_CALL || daii.ocx.opCode == BC_OC_CALL_EXTERN) && daii.ocx.datatype != BC_DT_NONE)
				insStr.append("??? : ");
		}

		return insStr;
	}
}