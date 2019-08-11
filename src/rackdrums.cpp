#include "plugin.hpp"
#include "osdialog.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include <vector>
#include "cmath"

using namespace std;


struct rackdrums : Module {
	enum ParamIds {
		COPY_PARAM,
		PASTE_PARAM,
		CLEAR_PARAM,
		INFO_PARAM,
		LAPIN_PARAM,
		TORTUE_PARAM,
		PLAY_PARAM,
		SONG_PARAM,
		GROUP_PARAM = SONG_PARAM + 5,
		BM_PARAM = GROUP_PARAM + 4,
		END_PARAM = BM_PARAM + 16,
		ON_PARAM = END_PARAM + 16,
		LOAD_PARAM = ON_PARAM + 256,
		NUM_PARAMS = LOAD_PARAM +16
	};
	enum InputIds {
		RST_INPUT,
		UP_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS = OUT_OUTPUT+4
	};
	enum LightIds {		
		NUM_LIGHTS
	};

int pas = 0;
int measure=0;
int group =0;
int affichnum = 128 ;
int stepper = 0 ;
float toc_phase = 0.f;
uint32_t toc = 0u;
float end = 15;
float m_visible = 0;
float play_visible = 0;
bool loop_state = false ;
bool ledState[16384] = {};
bool copyState[256] = {};
bool ON_STATE = false;
int tempState[16] = {};
int song_state = 0 ;
int song_pos = 0 ;
int song_end = 0 ;
int song_m[1000] = {};
int song_g[1000] = {};
dsp::SchmittTrigger rstTrigger, upTrigger;
dsp::SchmittTrigger ledTrigger[256] ={};
dsp::SchmittTrigger BMTrigger[16] ={};
dsp::SchmittTrigger groupTrigger[4] ={};
dsp::SchmittTrigger endTrigger[16] ={};
dsp::SchmittTrigger copyTrigger, pastTrigger, clearTrigger;
dsp::SchmittTrigger songTrigger[5]={};
dsp::SchmittTrigger playTrigger;
dsp::SchmittTrigger lapinTrigger, tortueTrigger;
int tempi[30] = {30,31,32,33,34,36,37,39,40,42,45,27,50,52,56,60,64,69,75,81,90,100,112,128,150,180,225,300,450,900};
int tempi_ind = 23 ;
bool lig[256] ;
bool measd[16], groud[4], stepd[16], songd[5] ; 
bool play[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} ;

std::string lastPath[16]={"","","","","","","","","","","","","","","",""};
vector<vector<float>> rdrBuffer;
unsigned int channels[16];
unsigned int sampleRate[16];
drwav_uint64 totalSampleCount[16];
float samplePos[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
std::string fileDesc[16]={"","","","","","","","","","","","","","","",""};
bool fileLoaded[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
std::string pathM;
dsp::SchmittTrigger loadsampleTrigger[16];
dsp::SchmittTrigger samplay[16];
int loader = -1;

rackdrums() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 256; i++) {
			configParam(ON_PARAM+i, 0.0, 1.0, 0.0, "On");}
		for (int i = 0; i < 16; i++) {
			configParam(BM_PARAM+i, 0.0, 1.0, 0.0, "Bm");}
		for (int i = 0; i < 16; i++) {
			configParam(END_PARAM+i, 0.0, 1.0, 0.0, "End");}
		for (int i = 0; i < 4; i++) {
			configParam(GROUP_PARAM+i, 0.0, 1.0, 0.0, "Group");}
		for (int i = 0; i < 5; i++) {
			configParam(SONG_PARAM+i, 0.0, 1.0, 0.0, "Song");}
		configParam(LAPIN_PARAM, 0.0, 1.0, 0.0, "Lapin");
		configParam(TORTUE_PARAM, 0.0, 1.0, 0.0, "Tortue");
		configParam(COPY_PARAM, 0.0, 1.0, 0.0, "Copy");
		configParam(PASTE_PARAM, 0.0, 1.0, 0.0, "Past");
		configParam(CLEAR_PARAM, 0.0, 1.0, 0.0, "clear");
		configParam(INFO_PARAM, 0.0, 1.0, 0.0, "Info");
		configParam(PLAY_PARAM, 0.0, 1.0, 0.0, "Play");
		for (int i = 0; i < 16; i++) {
			configParam(LOAD_PARAM+i, 0.0, 1.0, 0.0, "Load");}
		rdrBuffer.resize(16);
for (int i = 0; i < 16; i++) {
		rdrBuffer[i].resize(0); }
	//	onReset();
}

	// persistence
/////////////////////////////////////////////////////////////////////JSON write	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		// lastPath
		std::string paths ="";
		for (int i=0;i<16;i++) {
			paths = paths + lastPath[i] + ";";}
		json_object_set_new(rootJ, "Paths", json_string(paths.c_str()));

		// tempi_ind
		json_object_set_new(rootJ, "temp", json_integer(tempi_ind));

		// measure
		json_object_set_new(rootJ, "meas", json_integer(measure));

		// group
		json_object_set_new(rootJ, "grou", json_integer(group));

		// end
		json_object_set_new(rootJ, "end", json_integer(end));

		// song_end
		json_object_set_new(rootJ, "send", json_integer(song_end));

		// loop_state
		json_object_set_new(rootJ, "loop", json_integer(loop_state));

		// song_state
		json_object_set_new(rootJ, "song", json_integer(song_state));

		// ON_STATE
		json_object_set_new(rootJ, "on", json_integer(ON_STATE));

		// leds
		json_t *ledsJ = json_array();
		for (int i = 0; i < 16384; i++) {
			json_t *ledJ = json_integer((int) ledState[i]);
			json_array_append_new(ledsJ, ledJ);
		}
		json_object_set_new(rootJ, "leds", ledsJ);

		// song_m
		json_t *msJ = json_array();
		for (int i = 0; i < song_end; i++) {
			json_t *mJ = json_integer((int) song_m[i]);
			json_array_append_new(msJ, mJ);
		}
		json_object_set_new(rootJ, "ms", msJ);

		// song_g
		json_t *gsJ = json_array();
		for (int i = 0; i < song_end; i++) {
			json_t *gJ = json_integer((int) song_g[i]);
			json_array_append_new(gsJ, gJ);
		}
		json_object_set_new(rootJ, "gs", gsJ);

		return rootJ;
		
	}
/////////////////////////////////////////////////////////////////////JSON read
	void dataFromJson(json_t *rootJ) override {
		// lastPath
		json_t *PathsJ = json_object_get(rootJ, "Paths"); 

		if (PathsJ) {
			std::string ppp = json_string_value(PathsJ);
			int j=0;
			for (int i=0; i<10000 ; i++){
					if ((ppp[i]!= ';') & (j<16)) 
						lastPath[j]=lastPath[j]+ppp[i];
						else j=j+1;
					if (j>15) i =10000;
					};
			for (int k=0; k<16 ; k++){
					//loader=k; 
					loadSample(lastPath[k],k);
					};
				
		};



		//loader=-1; songd[2]=0;songd[3]=0;

		// tempi_ind
		json_t *tempJ = json_object_get(rootJ, "temp");
		if (tempJ)
			tempi_ind = json_integer_value(tempJ);

		// measure
		json_t *measJ = json_object_get(rootJ, "meas");
		if (measJ)
			measure = json_integer_value(measJ);

		// groupe
		json_t *grouJ = json_object_get(rootJ, "grou");
		if (grouJ)
			group = json_integer_value(grouJ);

		// end
		json_t *endJ = json_object_get(rootJ, "end");
		if (endJ)
			end = json_integer_value(endJ);

		// song_end
		json_t *sendJ = json_object_get(rootJ, "send");
		if (sendJ)
			song_end = json_integer_value(sendJ);

		// loop_state
		json_t *loopJ = json_object_get(rootJ, "loop");
		if (loopJ)
			loop_state = json_integer_value(loopJ);

		// song_state
		json_t *songJ = json_object_get(rootJ, "song");
		if (songJ)
			song_state = json_integer_value(songJ);
		if (song_state == 2) {
			song_pos=0;pas=0;
			measure = song_m[song_pos];
			group = song_g[song_pos];
			}

		// ON_STATE
		json_t *onJ = json_object_get(rootJ, "on");
		if (onJ)
			ON_STATE = json_integer_value(onJ);

		// leds
		json_t *ledsJ = json_object_get(rootJ, "leds");
		if (ledsJ) {
			for (int i = 0; i < 16384; i++) {
				json_t *ledJ = json_array_get(ledsJ, i);
				if (ledJ)
					ledState[i] = !!json_integer_value(ledJ);
			}
		}

		// song_ms
		json_t *msJ = json_object_get(rootJ, "ms");
		if (msJ) {
			for (int i = 0; i < song_end; i++) {
				json_t *mJ = json_array_get(msJ, i);
				if (mJ)
					song_m[i] = json_integer_value(mJ);
			}
		}

		// song_gs
		json_t *gsJ = json_object_get(rootJ, "gs");
		if (gsJ) {
			for (int i = 0; i < song_end; i++) {
				json_t *gJ = json_array_get(gsJ, i);
				if (gJ)
					song_g[i] = json_integer_value(gJ);
			}
		}
	}


	void onReset() override {
		for (int i = 0; i < 16384; i++) {
			ledState[i] = false;
		}
		song_end = 0;
		end = 15;
		song_state=0;
		loop_state=0;
		tempi_ind=23;
	}

	void onRandomize() override {
		//for (int i = 0; i < 16384; i++) {
		//	ledState[i] = (randomUniform() > 0.5);
		//}
	}




/////////////////////////////////////////////////////////////////rackdrums loadSample
void loadSample(std::string xpath, int loa) {

		unsigned int c;
  		unsigned int sr;
  		drwav_uint64 sc;

		float *pSampleData;
		pSampleData = drwav_open_and_read_file_f32(xpath.c_str(), &c, &sr, &sc);

	if (pSampleData != NULL) {
		channels[loa] = c;
		sampleRate[loa] = sr;
		rdrBuffer[loa].clear();
		for (unsigned int i=0; i < sc; i = i + c) {
			rdrBuffer[loa].push_back(pSampleData[i]);
			}
		
		drwav_free(pSampleData);

		totalSampleCount[loa] = rdrBuffer[loa].size();
		fileLoaded[loa] = true;
		fileDesc[loa] = rack::string::filename(xpath);
//		lastPath[loa] = xpath;
	}

	else {
		
		fileLoaded[loa] = false;
	}
}; 

//////////////////////////////////////////////////////////////////////////rackdrums step
void process(const ProcessArgs &args) override {
/////////////////////////////////////////////////////////////////////CLOCK
if (inputs[UP_INPUT].active!=true) {

	float bpm = tempi[tempi_ind]; ;
	bool toced = false;

	if (ON_STATE) {
		toc_phase += ((bpm / 60.f) * args.sampleTime) * 12.f;
		
		if(toc_phase >= 1.f) {
			toced = true;
			toc = (toc+1u) % 48u ;
			toc_phase -= 1.f;
			}

		if(toced) {
			if (toc % 3u ? 0 : 1) stepper = 1;
			}


	} else {toc = 47u;toc_phase = 1.f;stepper=0;}

};

/////////////////////////////////////////////////////////////////////INPUTS
	if (rstTrigger.process(inputs[RST_INPUT].value))
		{
		pas = 0;
		if (song_state==2) {
			song_pos=0;
			measure = song_m[song_pos];
			group = song_g[song_pos];
			}
		toc = 47u;toc_phase = 1.f;
		}

	if (upTrigger.process(inputs[UP_INPUT].value) or stepper!=0)
		{ 	
		stepper =0;

		for (int i = 0; i < 16; i++) {
			if (ledState[i+pas*16+group*4096+measure*256]>0) tempState [i] = 50;
		}

		if (pas <end) pas = pas+1; else {
			pas =0;
			if (song_state==2) {
				if (song_pos<song_end-1) {
					song_pos+=1;
					measure = song_m[song_pos];
					group = song_g[song_pos];
					} else {
					if (loop_state == false) {
						ON_STATE=0;
						song_state=0; 
					} else {
						song_pos=0;
						measure = song_m[song_pos];
						group = song_g[song_pos];
					}
				};
			}
		};
	}


//////////steps buttons
	for (int i = 0; i < 256; i++) {
		if (ledTrigger[i].process(params[ON_PARAM +i].value)) {
			if (ledState[i+group*4096+measure*256]>0) {
				ledState[i+group*4096+measure*256]=0;
			} else {
				for (int j = 0; j < 4; j++) {
					ledState[int(i/4)*4+j+group*4096+measure*256]=0;
				};
				ledState[i+group*4096+measure*256]=1;
			};  
		};
	}		
//////////pattern buttons
	for (int i = 0; i < 16; i++) {
		if (BMTrigger[i].process(params[BM_PARAM +i].value)) {
			measure = i ;  
			};}
	for (int i = 0; i < 4; i++) {
		if (groupTrigger[i].process(params[GROUP_PARAM +i].value)) {
			group = i ;  
			};}
	for (int i = 0; i < 16; i++) {
		if (endTrigger[i].process(params[END_PARAM +i].value)) {
			end = i ;  
			};}

//////////menu buttons
	if (copyTrigger.process(params[COPY_PARAM].value)) {
			for (int i = 0; i < 256; i++) {copyState[i]=ledState[i+group*4096+measure*256];}  
			};
	if (pastTrigger.process(params[PASTE_PARAM].value)) {
			for (int i = 0; i < 256; i++) {ledState[i+group*4096+measure*256]=copyState[i];}  
			};
	if (clearTrigger.process(params[CLEAR_PARAM].value)) {
			for (int i = 0; i < 256; i++) {ledState[i+group*4096+measure*256]=false;}  
			};

//////////song buttons
			
	if (songTrigger[0].process(params[SONG_PARAM+0].value)) {
			if (song_state !=1) song_state=1; else song_state=0;
			};

	if (songTrigger[1].process(params[SONG_PARAM+1].value)) {
		if (song_end!=0) {
			if (song_state !=2) {
				if ((ON_STATE==0) & (inputs[UP_INPUT].active!=true)) {
					pas=0;
					song_pos=0;
					measure = song_m[song_pos];
					group = song_g[song_pos];
				} else song_pos=-1;
				song_state=2;
				ON_STATE=1;
			} else {song_state=0;ON_STATE=0;};
		};
	};

	if (song_state==1) {
		if (songTrigger[2].process(params[SONG_PARAM+2].value)) {
			song_m[song_end] = measure;
			song_g[song_end] = group;
			if (song_end<999) song_end+=1 ;  
			};
		if (songTrigger[3].process(params[SONG_PARAM+3].value)) {
			if (song_end>0) song_end-=1 ;  
			};
				}
	if (songTrigger[4].process(params[SONG_PARAM+4].value)) {
		if (loop_state == true) loop_state = false ; else loop_state = true ;
		};

//////////clock buttons

	if (playTrigger.process(params[PLAY_PARAM].value)) {
		if (ON_STATE == 0) {ON_STATE = 1;pas=0;} 
		else {if (song_state==2) song_state=0;else ON_STATE = 0;};
	};
	if (lapinTrigger.process(params[LAPIN_PARAM].value)) {
		if (tempi_ind<29) tempi_ind+=1 ;  
		};
	if (tortueTrigger.process(params[TORTUE_PARAM].value)) {
		if (tempi_ind>0) tempi_ind-=1 ;
		};



///////////////////////////////////////affichage
	for (int i = 0; i < 256; i++) {lig[i]=ledState[i+group*4096+measure*256];}

	for (int i = 0; i < 16; i++) {if ((i==pas) and ((ON_STATE) or (inputs[UP_INPUT].active))) stepd[i]=1; else stepd[i]=0;}

	for (int i = 0; i < 16; i++) {if (i==measure) measd[i]=1;else measd[i]=0;}

	for (int i = 0; i < 4; i++) {if (i==group) groud[i]=1;else groud[i]=0;}

	for (int i = 0; i < 2; i++) {if (song_state == i+1) songd[i]=1;else songd[i]=0;}
					
	songd[4]=loop_state;
	if (ON_STATE==0) play_visible =0 ; else play_visible =1 ;
	if (song_state==0) m_visible =0 ; else m_visible =1 ;
	if (song_state==0) affichnum = tempi[tempi_ind]; ;
	if (inputs[UP_INPUT].active==true) affichnum = 0;
	if (song_state==1) affichnum = song_end ;
	if (song_state==2) affichnum = song_pos ;

////////////////////////////////////////////////////////////////// LOAD SAMPLE
for (int i=0; i<16; i++) {
	if (loadsampleTrigger[i].process(params[LOAD_PARAM+i].value))
		{
		lastPath[i] = pathM;
		loader=i;
		};

	if (!fileLoaded[i]) fileDesc[i] = "load sample";
}


if(loader!=-1)
	{
	loadSample(lastPath[loader] ,loader);
	loader=-1;
	};	
////////////////////////////////////////////////////////////////// PLAY SAMPLE


for (int i=0; i<16; i++) {

		if (tempState [i]>0)
			{
			//play[i]=true;
			for (int k =0; k<4; k++) {
				if (int(i/4)*4+k==i) play[i] = true; else play[int(i/4)*4+k]=false;
				}
				samplePos[i] = 0;
				tempState[i] = 0;

			}

    
	if ((play[i]) && fileLoaded[i] && ((floor(samplePos[i]) < totalSampleCount[i]) && (floor(samplePos[i]) >= 0))) 
		{
		int j = floor(i/4);
		outputs[OUT_OUTPUT +j].value =5 * (rdrBuffer[i][floor(samplePos[i])]);
		samplePos[i] = samplePos[i]+1;
		}
		else
		{ 
		if ( (play[i]) && (floor(samplePos[i]) >= totalSampleCount[i]) ) {
			int j = floor(i/4);
			outputs[OUT_OUTPUT +j].value =0;}
		play[i] = false;
		}
	}

};
};
//////////////////////////////////////////////////////rackdrumsWidget step

//void rackdrums::rackdrumsWidget::step() {
//	rackdrums *macdr = dynamic_cast<rackdrums*>(module);
//	widgetToMove->box.pos.x = -200 + 210* int(macdr->play_visible); 
//	if(macdr->loader!=-1)
//		{
		//macdr->loader=macdr->loader-1;
		//std::string dir = macdr->lastPath[macdr->loader].empty() ? assetLocal("") : stringDirectory(macdr->lastPath[macdr->loader]);
//		char *path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);//dir.c_str()
//		if (path) {
//			macdr->play[macdr->loader] = false;
//			macdr->loadSample(path, macdr->loader);
//			macdr->samplePos[macdr->loader] = 0;
//			macdr->lastPath[macdr->loader] = path;
//			free(path);
//			};
//		macdr->loader=-1;
//		};
//};





////////////////////////////////////////////////////////////////////////////////STRUCTS


//////////////////////////////////////////////////////////////////LED
struct LButton : app::SvgSwitch {
	LButton() {
		momentary = true;
		shadow->visible = false;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/L.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Ld.svg")));
	}
};
//////////////////////////////////////////////////////////////////loop end
struct ENDButton : app::SvgSwitch {
	ENDButton() {
		momentary = true;
		shadow->visible = false;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/end.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/end.svg")));
	}
};
//////////////////////////////////////////////////////////////////menu 
struct COPYButton : app::SvgSwitch {
	COPYButton() {
		momentary = true;
		shadow->visible = false;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/copy.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/copyd.svg")));
	}
};
struct PASTEButton : app::SvgSwitch {
	PASTEButton() {
		momentary = true;
		shadow->visible = false;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/paste.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/pasted.svg")));
	}
};
struct CLEARButton : app::SvgSwitch {
	CLEARButton() {
		momentary = true;
		shadow->visible = false;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/clear.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/cleard.svg")));
	}
};
struct INFOButton : app::SvgSwitch {
	INFOButton() {
		momentary = true;
		shadow->visible = false;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/info.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/infod.svg")));
	}
};
//////////////////////////////////////////////////////////////////tempo
struct LAPINButton : app::SvgSwitch {
	LAPINButton() {
		momentary = true;
		shadow->visible = false;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/lapin.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/lapind.svg")));
	}
};
struct TORTUEButton : app::SvgSwitch {
	TORTUEButton() {
		momentary = true;
		shadow->visible = false;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/tortue.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/tortued.svg")));
	}
};
//////////////////////////////////////////////////////////////////play
struct PLAYButton : app::SvgSwitch {
	PLAYButton() {
		momentary = true;
		shadow->visible = false;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/play.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/playd.svg")));

	}
};


////////////////////////////////////////////////////////////////////////////////////////loadsample
////////////////////////////////////////////////////////////////////////////////////////loadsample
////////////////////////////////////////////////////////////////////////////////////////loadsample
//static void selectPath(rackdrums *module)
//{
//	char *path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);
//	if (path)
//	{
//		module->lastPath[module->loader] = std::string(path);
//		free(path);
//	}
//}


struct LOADsampleButton : app::SvgSwitch {

	LOADsampleButton() {
		momentary = true;
		shadow->visible = false;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/loads.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/loadsd.svg")));

	}

	void onDragStart(const event::DragStart &e)
	{
		rackdrums *module = dynamic_cast<rackdrums *>(paramQuantity->module);

		if (module) 
			//selectPath(module);
			{
			char *path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);
			if (path)
				{
				module->pathM = path;
				//module->loader=1;
				free(path);
				}
			}
		app::SvgSwitch::onDragStart(e);
	}
};

////////////////////////////////////////////////////////////////////////////////DISPLAYS
struct GRIDDisplay : TransparentWidget {
	rackdrums *module;

	GRIDDisplay() {
		
	}
	
	void draw(const DrawArgs &args) override {
if (module) {
	for (int i=0; i<256; i++){
		if (module->lig[i]){
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, 0+int(i/16)*18,0+ (i%16)*18, 4.8);
			nvgFillColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xff));
			nvgFill(args.vg);
		}
		}

	}
}
};

struct MEASDisplay : TransparentWidget {
	rackdrums *module;

	MEASDisplay() {
		
	}
	
	void draw(const DrawArgs &args) override {
if (module) {
	for (int i=0; i<16; i++){
		if (module->measd[i]){
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, 0+i*18,0, 4.8);
			nvgFillColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xff));
			nvgFill(args.vg);
		}
		}

	}
}
};

struct GROUDisplay : TransparentWidget {
	rackdrums *module;

	GROUDisplay() {
		
	}
	
	void draw(const DrawArgs &args) override {
if (module) {
	for (int i=0; i<4; i++){
		if (module->groud[i]){
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, 0+i*24,0, 4.8);
			nvgFillColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xff));
			nvgFill(args.vg);
		}
		}

	}
}
};

struct SONGDisplay : TransparentWidget {
	rackdrums *module;

	SONGDisplay() {
		
	}
	
	void draw(const DrawArgs &args) override {
if (module) {
	for (int i=0; i<5; i++){
	  if ((i!=2) & (i!=3)){
		if (module->songd[i]){
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, 0,0+i*15, 4.8);
			nvgFillColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xff));
			nvgFill(args.vg);
		}}
		}

	}
}
};

struct STEPDisplay : TransparentWidget {
	rackdrums *module;

	STEPDisplay() {
		
	}
	
	void draw(const DrawArgs &args) override {
if (module) {
	for (int i=0; i<16; i++){
		if (module->stepd[i]){
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, 0+i*18,0, 4.8);
			nvgFillColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xff));
			nvgFill(args.vg);
		}
		}

	}
}
};

struct NumDisplayWidget : TransparentWidget {
	rackdrums *module;
  int *value;
  std::shared_ptr<Font> font;

  NumDisplayWidget() {
        font = APP->window->loadFont(asset::plugin(pluginInstance, "res/Segment7Standard.ttf"));
  };

	void draw(const DrawArgs &args) override {
if (module) {

    nvgFontSize(args.vg, 24);
    nvgFontFaceId(args.vg, font->handle);
    nvgTextLetterSpacing(args.vg, 0);

    std::string to_display = std::to_string(*value);

    while(to_display.length()<3) to_display = '0' + to_display;


    Vec textPos = Vec(6.0f, 17.0f);

    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);

    nvgFillColor(args.vg, textColor);
    nvgText(args.vg, textPos.x, textPos.y, to_display.c_str(), NULL);};
}
  
};

struct ARROWEDDisplay : TransparentWidget {
	rackdrums *module;
	float *enda ;

	ARROWEDDisplay() {
		
	}
	
	void draw(const DrawArgs &args) override {
if (module) {


		
			nvgBeginPath(args.vg);
	
		
			nvgStrokeWidth(args.vg,2);
			nvgStrokeColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xff));
			{
				nvgBeginPath(args.vg);
				nvgMoveTo(args.vg, 0+*enda*18,-2);
				nvgLineTo(args.vg, 0+*enda*18,5);
				nvgLineTo(args.vg, 0+*enda*18-5,5);
				nvgLineTo(args.vg, 0+*enda*18-3,2);
			}
			nvgStroke(args.vg);
		
}
	}
};

struct MNDisplay : TransparentWidget {
	rackdrums *module;
	float *visible ;

	MNDisplay() {
		
	}
	
	void draw(const DrawArgs &args) override {
if (module) {
			if (*visible!=0) {
		    	NVGcolor backgroundColor = nvgRGB(0xff, 0xff, 0xff);
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, -4, 2, 20, -24, 2.0);
			nvgFillColor(args.vg, backgroundColor);
			nvgFill(args.vg);
		
			nvgBeginPath(args.vg);
	

			nvgStrokeWidth(args.vg,3);
			nvgStrokeColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xff));
			{
				nvgBeginPath(args.vg);
				nvgMoveTo(args.vg, 0,-1);
				nvgLineTo(args.vg, 0,-17);
				nvgLineTo(args.vg, 6,-13);
				nvgLineTo(args.vg, 12,-17);
				nvgLineTo(args.vg, 12,-1);
			}
			nvgStroke(args.vg);
			}
		

	}
}
};



struct rackdrumsDisplay : TransparentWidget {
	rackdrums *module;

	int frame = 0;
	shared_ptr<Font> font;

	rackdrumsDisplay() {
font = APP->window->loadFont(asset::plugin(pluginInstance, "res/Silom.ttf"));
	}
	
	void draw(const DrawArgs &args) override {
if (module) {
		nvgFontSize(args.vg, 14);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, 0);
		nvgFillColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xff));
		for (int i=0 ; i<16 ; i++) {	
			nvgTextBox(args.vg, 5, 5+i*18,500, module->fileDesc[i].c_str(), NULL);
		};
	}
}
};


/////////////////////////////////////////////////////////////////////rackdrumsWidget

struct rackdrumsWidget : ModuleWidget {
	rackdrumsWidget(rackdrums *module) {
	
	setModule(module);

	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/rackdrums.svg")));


	addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createWidget<ScrewSilver>(Vec(15, 365)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));
	
	for (int i = 0; i < 16; i++) {
	for (int j = 0; j < 16; j++) {
     		addParam(createParam<LButton>(Vec(136+i*18-1.0,j*18+30-1), module, rackdrums::ON_PARAM + (i*16+j)));
	}}

	for (int i = 0; i < 16; i++) {
     		addParam(createParam<LButton>(Vec(136+i*18-1.0,338-1), module, rackdrums::BM_PARAM + (i)));
	}

	for (int i = 0; i < 16; i++) {
     		addParam(createParam<ENDButton>(Vec(136+i*18-1.0,321-1), module, rackdrums::END_PARAM + (i)));
	}

	for (int i = 0; i < 4; i++) {
     		addParam(createParam<LButton>(Vec(i*24-1.0+25,338-1), module, rackdrums::GROUP_PARAM + (i)));
	}

	for (int i = 0; i < 5; i++) {
     		addParam(createParam<LButton>(Vec(72-1,i*15-1.0+143), module, rackdrums::SONG_PARAM + (i)));
	}


	addParam(createParam<LAPINButton>(Vec(55,74), module, rackdrums::LAPIN_PARAM));
	addParam(createParam<TORTUEButton>(Vec(15,74), module, rackdrums::TORTUE_PARAM));


	{
		ARROWEDDisplay *pdisplay = new ARROWEDDisplay();
		pdisplay->box.pos = Vec(143, 324);
		pdisplay->enda = &module->end;
		pdisplay->module = module;
		addChild(pdisplay);
	}
	{
		MNDisplay *mdisplay = new MNDisplay();
		mdisplay->box.pos = Vec(18, 57);
		mdisplay->visible = &module->m_visible;
		mdisplay->module = module;
		addChild(mdisplay);
	}
	{
		GRIDDisplay *grdisplay = new GRIDDisplay();
		grdisplay->box.pos = Vec(140.7, 34.2);
		grdisplay->module = module;
		addChild(grdisplay);
	}
	{
		MEASDisplay *medisplay = new MEASDisplay();
		medisplay->box.pos = Vec(140.7, 342.7);
		medisplay->module = module;
		addChild(medisplay);
	}
	{
		GROUDisplay *groudisplay = new GROUDisplay();
		groudisplay->box.pos = Vec(29.7, 342.7);
		groudisplay->module = module;
		addChild(groudisplay);
	}
	{
		STEPDisplay *stepdisplay = new STEPDisplay();
		stepdisplay->box.pos = Vec(140.7, 342.7);
		stepdisplay->module = module;
		addChild(stepdisplay);
	}
	{
		SONGDisplay *songdisplay = new SONGDisplay();
		songdisplay->box.pos = Vec(76.7, 147.7);
		songdisplay->module = module;
		addChild(songdisplay);
	}

	addParam(createParam<COPYButton>(Vec(37,2), module, rackdrums::COPY_PARAM));
	addParam(createParam<PASTEButton>(Vec(80,2), module, rackdrums::PASTE_PARAM));
	addParam(createParam<CLEARButton>(Vec(127,2), module, rackdrums::CLEAR_PARAM));
	addParam(createParam<INFOButton>(Vec(187,2), module, rackdrums::INFO_PARAM));

	addParam(createParam<PLAYButton>(Vec(10,235), module, rackdrums::PLAY_PARAM));
	
	{
		NumDisplayWidget *display = new NumDisplayWidget();
		display->box.pos = Vec(39,39);
		display->box.size = Vec(50, 20);
		display->value = &module->affichnum;
		display->module = module;
		addChild(display);
	}

	addInput(createInput<PJ301MPort>(Vec(25, 100), module, rackdrums::RST_INPUT));
	addInput(createInput<PJ301MPort>(Vec(67, 100), module, rackdrums::UP_INPUT));  

	{
		rackdrumsDisplay *gdisplay = new rackdrumsDisplay();
		gdisplay->box.pos = Vec(420, 36);
		gdisplay->box.size = Vec(130, 250);
		gdisplay->module = module;
		addChild(gdisplay);
	}



for (int i=0; i<16; i++) {
	addParam(createParam<LOADsampleButton>(Vec(420,27+i*18), module, rackdrums::LOAD_PARAM+i));

	}

for (int i=0; i<4; i++) {
	addOutput(createOutput<PJ301MPort>(Vec(442+i%2*40, 317+int(i/2)*25), module, rackdrums::OUT_OUTPUT +i));
	}


}

};
Model *modelrackdrums = createModel<rackdrums, rackdrumsWidget>("rackdrums");
