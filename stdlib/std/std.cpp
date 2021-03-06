#include <PluS.h>

#include "../../MarCore/include/runtime/ExternalFunction.h"
#include "../../MarCore/include/runtime/Interpreter.h"
#include "../../MarCore/include/unused.h"

#include <iostream>
#include <thread>

class EF_PrintS : public MarC::ExternalFunction
{
public:
	using MarC::ExternalFunction::ExternalFunction;
	PLUS_FEATURE_GET_NAME(">>stdext>>prints");
	virtual void call(MarC::Interpreter& interpreter, MarC::ExFuncData& efd) override
	{
		if (efd.nParams != 1)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected 1 parameter! Got " + std::to_string(efd.nParams) + "!");
		if (efd.param[0].datatype != MarC::BC_DT_ADDR)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected parameter of type 'u64! Got '" + MarC::BC_DatatypeToString(efd.param[0].datatype) + "'!");
		const char* str = &interpreter.hostObject<char>(efd.param[0].cell.as_ADDR);
		std::cout << str;
	}
};

class EF_PrintT : public MarC::ExternalFunction
{
public:
	using MarC::ExternalFunction::ExternalFunction;
	PLUS_FEATURE_GET_NAME(">>stdext>>printt");
	virtual void call(MarC::Interpreter& interpreter, MarC::ExFuncData& efd) override
	{
		UNUSED(interpreter);
		
		if (efd.nParams != 1)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected 1 parameter! Got " + std::to_string(efd.nParams) + "!");
		switch (efd.param[0].datatype)
		{
		case MarC::BC_DT_I_8:
			std::cout << efd.param[0].cell.as_I_8;
			break;
		default:
			std::cout << MarC::BC_MemCellToString(efd.param[0].cell, efd.param[0].datatype);
		}
	}
};

class EF_ScanS : public MarC::ExternalFunction
{
public:
	using MarC::ExternalFunction::ExternalFunction;
	PLUS_FEATURE_GET_NAME(">>stdext>>scans");
	virtual void call(MarC::Interpreter& interpreter, MarC::ExFuncData& efd) override
	{
		if (efd.nParams != 1)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected 1 parameter! Got " + std::to_string(efd.nParams) + "!");
		if (efd.param[0].datatype != MarC::BC_DT_ADDR)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected parameter of type 'addr! Got '" + MarC::BC_DatatypeToString(efd.param[0].datatype) + "'!");

		char* str = &interpreter.hostObject<char>(efd.param[0].cell.as_ADDR);
		int ret = scanf("%s", str);
		UNUSED(ret);
	}
};

class EF_ScanT : public MarC::ExternalFunction
{
public:
	using MarC::ExternalFunction::ExternalFunction;
	PLUS_FEATURE_GET_NAME(">>stdext>>scant");
	virtual void call(MarC::Interpreter& interpreter, MarC::ExFuncData& efd) override
	{
		UNUSED(interpreter);

		if (efd.nParams != 0)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected 0 parameters! Got " + std::to_string(efd.nParams) + "!");
		switch (efd.retVal.datatype)
		{
		case MarC::BC_DT_NONE:     break;
		case MarC::BC_DT_UNKNOWN:  break;
		case MarC::BC_DT_U_8:  std::cin >> efd.retVal.cell.as_U_8;  break;
		case MarC::BC_DT_U_16: std::cin >> efd.retVal.cell.as_U_16; break;
		case MarC::BC_DT_U_32: std::cin >> efd.retVal.cell.as_U_32; break;
		case MarC::BC_DT_U_64: std::cin >> efd.retVal.cell.as_U_64; break;
		case MarC::BC_DT_I_8:  std::cin >> efd.retVal.cell.as_I_8;  break;
		case MarC::BC_DT_I_16: std::cin >> efd.retVal.cell.as_I_16; break;
		case MarC::BC_DT_I_32: std::cin >> efd.retVal.cell.as_I_32; break;
		case MarC::BC_DT_I_64: std::cin >> efd.retVal.cell.as_I_64; break;
		case MarC::BC_DT_F_32: std::cin >> efd.retVal.cell.as_F_32; break;
		case MarC::BC_DT_F_64: std::cin >> efd.retVal.cell.as_F_64; break;
		case MarC::BC_DT_ADDR: std::cin >> efd.retVal.cell.as_U_64; break;
		case MarC::BC_DT_DATATYPE: break;
		}
	}
};

class EF_SleepMS : public MarC::ExternalFunction
{
public:
	using MarC::ExternalFunction::ExternalFunction;
	PLUS_FEATURE_GET_NAME(">>stdext>>sleepms");
	virtual void call(MarC::Interpreter& interpreter, MarC::ExFuncData& efd) override
	{
		UNUSED(interpreter);
		if (efd.nParams != 1)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected 1 parameter! Got " + std::to_string(efd.nParams) + "!");
		if (efd.param[0].datatype != MarC::BC_DT_U_64)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected parameter of type 'BC_DT_U_64! Got '" + MarC::BC_DatatypeToString(efd.param[0].datatype) + "'!");
		std::this_thread::sleep_for(std::chrono::milliseconds(efd.param[0].cell.as_U_64));
	}
};

PLUS_PERPLUGIN_DEFINE_EXTERNALS("STD-EXTENSION");

void PluS::PerPlugin::initPlugin()
{
	pPlugin->registerFeatureFactory(FeatureFactory::create<EF_PrintS>());
	pPlugin->registerFeatureFactory(FeatureFactory::create<EF_PrintT>());
	pPlugin->registerFeatureFactory(FeatureFactory::create<EF_ScanS>());
	pPlugin->registerFeatureFactory(FeatureFactory::create<EF_ScanT>());
	pPlugin->registerFeatureFactory(FeatureFactory::create<EF_SleepMS>());
}

void PluS::PerPlugin::shutdownPlugin()
{
}
