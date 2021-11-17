#include <PluS.h>

#include "../../MarCore/include/ExternalFunction.h"
#include "../../MarCore/include/Interpreter.h"

#include <iostream>
#include <thread>

class EF_PrintS : public MarC::ExternalFunction
{
public:
	using MarC::ExternalFunction::ExternalFunction;
	PLUS_FEATURE_GET_NAME(">>std>>ext>>prints");
	virtual void call(MarC::Interpreter& interpreter, MarC::ExFuncData& efd) override
	{
		if (efd.nParams != 1)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected 1 parameter! Got " + std::to_string(efd.nParams) + "!");
		if (efd.param[0].datatype != MarC::BC_DT_U_64)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected parameter of type 'u64! Got '" + MarC::BC_DatatypeToString(efd.param[0].datatype) + "'!");
		const char* str = &interpreter.hostObject<char>(efd.param[0].cell.as_ADDR);
		std::cout << str;
	}
};

class EF_PrintT : public MarC::ExternalFunction
{
public:
	using MarC::ExternalFunction::ExternalFunction;
	PLUS_FEATURE_GET_NAME(">>std>>ext>>printt");
	virtual void call(MarC::Interpreter& interpreter, MarC::ExFuncData& efd) override
	{
		if (efd.nParams != 1)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected 1 parameter! Got " + std::to_string(efd.nParams) + "!");
		switch (efd.param[0].datatype)
		{
		case MarC::BC_DT_I_8:  std::cout << efd.param[0].cell.as_I_8;  break;
		case MarC::BC_DT_I_16: std::cout << efd.param[0].cell.as_I_16; break;
		case MarC::BC_DT_I_32: std::cout << efd.param[0].cell.as_I_32; break;
		case MarC::BC_DT_I_64: std::cout << efd.param[0].cell.as_I_64; break;
		case MarC::BC_DT_U_8:  std::cout << efd.param[0].cell.as_U_8;  break;
		case MarC::BC_DT_U_16: std::cout << efd.param[0].cell.as_U_16; break;
		case MarC::BC_DT_U_32: std::cout << efd.param[0].cell.as_U_32; break;
		case MarC::BC_DT_U_64: std::cout << efd.param[0].cell.as_U_64; break;
		case MarC::BC_DT_F_32: std::cout << efd.param[0].cell.as_F_32; break;
		case MarC::BC_DT_F_64: std::cout << efd.param[0].cell.as_F_64; break;
		default:
			throw MarC::InterpreterError(MarC::IntErrCode::InvalidDatatype, "Invalid datatype specified for extFunc '>>std>>printt'!"); break;
		}
	}
};

class EF_ScanS : public MarC::ExternalFunction
{
public:
	using MarC::ExternalFunction::ExternalFunction;
	PLUS_FEATURE_GET_NAME(">>std>>ext>>scans");
	virtual void call(MarC::Interpreter& interpreter, MarC::ExFuncData& efd) override
	{
		if (efd.nParams != 1)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected 2 parameters! Got " + std::to_string(efd.nParams) + "!");
		if (efd.param[0].datatype != MarC::BC_DT_U_64)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected parameter of type 'addr! Got '" + MarC::BC_DatatypeToString(efd.param[0].datatype) + "'!");

		char* str = &interpreter.hostObject<char>(efd.param[0].cell.as_ADDR);
		int ret = scanf("%s", str);
	}
};

class EF_SleepMS : public MarC::ExternalFunction
{
public:
	using MarC::ExternalFunction::ExternalFunction;
	PLUS_FEATURE_GET_NAME(">>std>>ext>>sleepms");
	virtual void call(MarC::Interpreter& interpreter, MarC::ExFuncData& efd) override
	{
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
	pPlugin->registerFeatureFactory(FeatureFactory::create<EF_SleepMS>());
}

void PluS::PerPlugin::shutdownPlugin()
{
}
