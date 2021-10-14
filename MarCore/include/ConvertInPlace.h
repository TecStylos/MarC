#pragma once

#include "BytecodeTypes.h"

namespace MarC
{
	void ConvertInPlace(BC_MemCell& mc, BC_Datatype dtOld, BC_Datatype dtNew)
	{
		#pragma warning(disable: 4244)
		#define COMB_DT(left, right) (((uint32_t)left << 16) | (uint32_t)right)
		switch (COMB_DT(dtOld, dtNew))
		{
		case COMB_DT(BC_DT_I_8, BC_DT_I_8):  mc.as_I_8 = mc.as_I_8;  break;
		case COMB_DT(BC_DT_I_8, BC_DT_I_16): mc.as_I_16 = mc.as_I_8;  break;
		case COMB_DT(BC_DT_I_8, BC_DT_I_32): mc.as_I_32 = mc.as_I_8;  break;
		case COMB_DT(BC_DT_I_8, BC_DT_I_64): mc.as_I_64 = mc.as_I_8;  break;
		case COMB_DT(BC_DT_I_8, BC_DT_U_8):  mc.as_U_8 = mc.as_I_8;  break;
		case COMB_DT(BC_DT_I_8, BC_DT_U_16): mc.as_U_16 = mc.as_I_8;  break;
		case COMB_DT(BC_DT_I_8, BC_DT_U_32): mc.as_U_32 = mc.as_I_8;  break;
		case COMB_DT(BC_DT_I_8, BC_DT_U_64): mc.as_U_64 = mc.as_I_8;  break;
		case COMB_DT(BC_DT_I_8, BC_DT_F_32): mc.as_F_32 = mc.as_I_8;  break;
		case COMB_DT(BC_DT_I_8, BC_DT_F_64): mc.as_F_64 = mc.as_I_8;  break;
		case COMB_DT(BC_DT_I_8, BC_DT_BOOL): mc.as_BOOL = mc.as_I_8;  break;

		case COMB_DT(BC_DT_I_16, BC_DT_I_8):  mc.as_I_8 = mc.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_I_16): mc.as_I_16 = mc.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_I_32): mc.as_I_32 = mc.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_I_64): mc.as_I_64 = mc.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_U_8):  mc.as_U_8 = mc.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_U_16): mc.as_U_16 = mc.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_U_32): mc.as_U_32 = mc.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_U_64): mc.as_U_64 = mc.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_F_32): mc.as_F_32 = mc.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_F_64): mc.as_F_64 = mc.as_I_16; break;
		case COMB_DT(BC_DT_I_16, BC_DT_BOOL): mc.as_BOOL = mc.as_I_16; break;

		case COMB_DT(BC_DT_I_32, BC_DT_I_8):  mc.as_I_8 = mc.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_I_16): mc.as_I_16 = mc.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_I_32): mc.as_I_32 = mc.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_I_64): mc.as_I_64 = mc.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_U_8):  mc.as_U_8 = mc.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_U_16): mc.as_U_16 = mc.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_U_32): mc.as_U_32 = mc.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_U_64): mc.as_U_64 = mc.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_F_32): mc.as_F_32 = mc.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_F_64): mc.as_F_64 = mc.as_I_32; break;
		case COMB_DT(BC_DT_I_32, BC_DT_BOOL): mc.as_BOOL = mc.as_I_32; break;

		case COMB_DT(BC_DT_I_64, BC_DT_I_8):  mc.as_I_8 = mc.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_I_16): mc.as_I_16 = mc.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_I_32): mc.as_I_32 = mc.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_I_64): mc.as_I_64 = mc.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_U_8):  mc.as_U_8 = mc.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_U_16): mc.as_U_16 = mc.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_U_32): mc.as_U_32 = mc.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_U_64): mc.as_U_64 = mc.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_F_32): mc.as_F_32 = mc.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_F_64): mc.as_F_64 = mc.as_I_64; break;
		case COMB_DT(BC_DT_I_64, BC_DT_BOOL): mc.as_BOOL = mc.as_I_64; break;

		case COMB_DT(BC_DT_U_8, BC_DT_I_8):  mc.as_I_8 = mc.as_U_8;  break;
		case COMB_DT(BC_DT_U_8, BC_DT_I_16): mc.as_I_16 = mc.as_U_8;  break;
		case COMB_DT(BC_DT_U_8, BC_DT_I_32): mc.as_I_32 = mc.as_U_8;  break;
		case COMB_DT(BC_DT_U_8, BC_DT_I_64): mc.as_I_64 = mc.as_U_8;  break;
		case COMB_DT(BC_DT_U_8, BC_DT_U_8):  mc.as_U_8 = mc.as_U_8;  break;
		case COMB_DT(BC_DT_U_8, BC_DT_U_16): mc.as_U_16 = mc.as_U_8;  break;
		case COMB_DT(BC_DT_U_8, BC_DT_U_32): mc.as_U_32 = mc.as_U_8;  break;
		case COMB_DT(BC_DT_U_8, BC_DT_U_64): mc.as_U_64 = mc.as_U_8;  break;
		case COMB_DT(BC_DT_U_8, BC_DT_F_32): mc.as_F_32 = mc.as_U_8;  break;
		case COMB_DT(BC_DT_U_8, BC_DT_F_64): mc.as_F_64 = mc.as_U_8;  break;
		case COMB_DT(BC_DT_U_8, BC_DT_BOOL): mc.as_BOOL = mc.as_U_8;  break;

		case COMB_DT(BC_DT_U_16, BC_DT_I_8):  mc.as_I_8 = mc.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_I_16): mc.as_I_16 = mc.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_I_32): mc.as_I_32 = mc.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_I_64): mc.as_I_64 = mc.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_U_8):  mc.as_U_8 = mc.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_U_16): mc.as_U_16 = mc.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_U_32): mc.as_U_32 = mc.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_U_64): mc.as_U_64 = mc.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_F_32): mc.as_F_32 = mc.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_F_64): mc.as_F_64 = mc.as_U_16; break;
		case COMB_DT(BC_DT_U_16, BC_DT_BOOL): mc.as_BOOL = mc.as_U_16; break;

		case COMB_DT(BC_DT_U_32, BC_DT_I_8):  mc.as_I_8 = mc.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_I_16): mc.as_I_16 = mc.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_I_32): mc.as_I_32 = mc.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_I_64): mc.as_I_64 = mc.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_U_8):  mc.as_U_8 = mc.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_U_16): mc.as_U_16 = mc.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_U_32): mc.as_U_32 = mc.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_U_64): mc.as_U_64 = mc.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_F_32): mc.as_F_32 = mc.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_F_64): mc.as_F_64 = mc.as_U_32; break;
		case COMB_DT(BC_DT_U_32, BC_DT_BOOL): mc.as_BOOL = mc.as_U_32; break;

		case COMB_DT(BC_DT_U_64, BC_DT_I_8):  mc.as_I_8 = mc.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_I_16): mc.as_I_16 = mc.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_I_32): mc.as_I_32 = mc.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_I_64): mc.as_I_64 = mc.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_U_8):  mc.as_U_8 = mc.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_U_16): mc.as_U_16 = mc.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_U_32): mc.as_U_32 = mc.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_U_64): mc.as_U_64 = mc.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_F_32): mc.as_F_32 = mc.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_F_64): mc.as_F_64 = mc.as_U_64; break;
		case COMB_DT(BC_DT_U_64, BC_DT_BOOL): mc.as_BOOL = mc.as_U_64; break;

		case COMB_DT(BC_DT_F_32, BC_DT_I_8):  mc.as_I_8 = mc.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_I_16): mc.as_I_16 = mc.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_I_32): mc.as_I_32 = mc.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_I_64): mc.as_I_64 = mc.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_U_8):  mc.as_U_8 = mc.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_U_16): mc.as_U_16 = mc.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_U_32): mc.as_U_32 = mc.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_U_64): mc.as_U_64 = mc.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_F_32): mc.as_F_32 = mc.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_F_64): mc.as_F_64 = mc.as_F_32; break;
		case COMB_DT(BC_DT_F_32, BC_DT_BOOL): mc.as_BOOL = mc.as_F_32; break;

		case COMB_DT(BC_DT_F_64, BC_DT_I_8):  mc.as_I_8 = mc.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_I_16): mc.as_I_16 = mc.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_I_32): mc.as_I_32 = mc.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_I_64): mc.as_I_64 = mc.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_U_8):  mc.as_U_8 = mc.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_U_16): mc.as_U_16 = mc.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_U_32): mc.as_U_32 = mc.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_U_64): mc.as_U_64 = mc.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_F_32): mc.as_F_32 = mc.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_F_64): mc.as_F_64 = mc.as_F_64; break;
		case COMB_DT(BC_DT_F_64, BC_DT_BOOL): mc.as_BOOL = mc.as_F_64; break;

		case COMB_DT(BC_DT_BOOL, BC_DT_I_8):  mc.as_I_8 = mc.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_I_16): mc.as_I_16 = mc.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_I_32): mc.as_I_32 = mc.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_I_64): mc.as_I_64 = mc.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_U_8):  mc.as_U_8 = mc.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_U_16): mc.as_U_16 = mc.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_U_32): mc.as_U_32 = mc.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_U_64): mc.as_U_64 = mc.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_F_32): mc.as_F_32 = mc.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_F_64): mc.as_F_64 = mc.as_BOOL; break;
		case COMB_DT(BC_DT_BOOL, BC_DT_BOOL): mc.as_BOOL = mc.as_BOOL; break;
		}
		#undef COMB_DT
		#pragma warning(default: 4244)
	}
}