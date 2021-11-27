#include "DisAsmTypes.h"

namespace MarC
{
	std::string DisAsmInsInfoToString(const DisAsmInsInfo& daii)
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
			case InsArgType::Datatype:
				insStr.append(BC_DatatypeToString(arg.value.cell.as_Datatype));
				break;
			case InsArgType::TypedValue:
				insStr.append(BC_DatatypeToString(arg.value.datatype) + ".");
			case InsArgType::Value:
				//if (daii.ocx.derefArg.get(i))
				//	insStr.append("@");
				if (arg.getsDereferenced)
				{
					insStr.append("@");
				}
				else
				{
					switch (arg.value.datatype)
					{
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
					}
					break;
				}
			case InsArgType::Address:
				insStr.append(BC_MemAddressToString(arg.value.cell.as_ADDR));
				break;
			}

			if (i + 1 < daii.args.size())
				insStr.append(" : ");
		}

		return insStr;
	}
}