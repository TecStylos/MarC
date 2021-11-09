#include <PluS.h>

#include "../../MarCore/include/ExternalFunction.h"
#include "../../MarCore/include/Interpreter.h"

#include <iostream>

class EF_PrintS : public MarC::ExternalFunction
{
public:
	using MarC::ExternalFunction::ExternalFunction;
	PLUS_FEATURE_GET_NAME(">>std>>prints");
	virtual void call(MarC::Interpreter& interpreter, MarC::ExFuncData& efd) override
	{
		if (efd.nParams != 1)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected 1 parameter! Got " + std::to_string(efd.nParams) + "!");
		if (efd.param[0].datatype != MarC::BC_DT_U_64)
			throw MarC::InterpreterError(MarC::IntErrCode::WrongExtCallParamCount, "Expected parameter of type 'BC_DT_U_64! Got '" + MarC::BC_DatatypeToString(efd.param[0].datatype) + "'!");
		const char* str = &interpreter.hostObject<char>(efd.param[0].cell.as_ADDR);
		std::cout << str;
	}
};

PLUS_PERPLUGIN_DEFINE_EXTERNALS("STD-EXTENSION");

void PluS::PerPlugin::initPlugin()
{
	pPlugin->registerFeatureFactory(FeatureFactory::create<EF_PrintS>());
}

void PluS::PerPlugin::shutdownPlugin()
{
}