#include "plugin.hpp"


Plugin *pluginInstance;

void init(rack::Plugin *p) {
	pluginInstance = p;

	p->addModel(modelrackdrums);	
	p->addModel(modelrackdrums_trig);
}
