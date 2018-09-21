#include "1987.hpp"


Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = TOSTRING(SLUG);
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->website = "";
	p->manual = "";


	p->addModel(modelrackdrums);
	p->addModel(modelrackdrums_trig);

}
