#include <PluS.h>

#include "../../MarCore/include/ExternalFunction.h"

#include <iostream>

class EF_PrintS : public MarC::ExternalFunction
{
public:
	using MarC::ExternalFunction::ExternalFunction;
	PLUS_FEATURE_GET_NAME(">>std>>prints");
	virtual void call() override { std::cout << "Hello world!" << std::endl; }
};

PLUS_PERPLUGIN_DEFINE_EXTERNALS("STD-EXTENSION");

void PluS::PerPlugin::initPlugin()
{
	pPlugin->registerFeatureFactory(FeatureFactory::create<EF_PrintS>());
}

void PluS::PerPlugin::shutdownPlugin()
{
}