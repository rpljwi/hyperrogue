// Hyperbolic Rogue
// This is the main file when the online version of HyperRogue is compiled with Emscripten.
// Copyright (C) 2011-2018 Zeno Rogue, see 'hyper.cpp' for details

#define ISWEB 1
#define ISMINI 0
#define CAP_AUDIO 0
#define CAP_SDLGFX 0
#define CAP_PNG 0
#define CAP_TOUR 1
#define CAP_SDLTTF 0
#define CAP_SHMUP 0
#define CAP_RUG 1
#define CAP_INV 0
#define CAP_URL 1
#define GLES_ONLY
#define CAP_COMPLEX2 0
#define CAP_EDIT 1
#define CAP_COMPLEX2 1

#if CAP_ROGUEVIZ
#define MAXMDIM 4
#else
#define MAXMDIM 4
#endif

// we want newconformist, but we don't want CAP_GD there
#define CAP_NCONF 1
#define CAP_GD 0

#ifndef EMSCRIPTEN
#define EMSCRIPTEN
#endif

#ifdef FAKEWEB
namespace hr { void mainloopiter(); }
template<class A, class B, class C> void emscripten_set_main_loop(A a, B b, C c) { while(true) mainloopiter(); }
#else
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <string>

namespace hr { 
  void initweb();
  void emscripten_get_commandline();

  void loadCompressedChar(int &otwidth, int &otheight, int *tpix);

  const char *wheresounds;

  std::string get_value(std::string name);
  }

#include "hyper.cpp"

namespace hr {

string get_value(string name) {
  char *str = (char*)EM_ASM_INT({
    var name = UTF8ToString($0, $1);
    var value = document.getElementById(name).value;
    var lengthBytes = lengthBytesUTF8(value)+1;
    var stringOnWasmHeap = _malloc(lengthBytes);
    stringToUTF8(value, stringOnWasmHeap, lengthBytes);
    return stringOnWasmHeap;
    }, name.c_str(), int(name.size())
    );
  string res = str;
  free(str);
  return res;
  }

// -- demo --

#if CAP_URL
EX void open_url(string s) {
  EM_ASM_({
    window.open(UTF8ToString($0, 1000));
    }, s.c_str());
  }
#endif

//    window.open(Pointer_stringify($0));
bool demoanim;

void toggleanim(bool v) {
  demoanim = v;
  if(v) {
    sightrange_bonus = -3;
    vid.wallmode = 5;
    vid.particles = true;
    vid.sspeed = -1;
    vid.mspeed = -1;
    vid.highdetail = vid.middetail = 5;
    }
  else {
    sightrange_bonus = 0;
    vid.sspeed = 5;
    vid.mspeed = 5;
    vid.particles = false;
    vid.wallmode = 3;
    vid.highdetail = vid.middetail = -1;
    }
  }

void showDemo() {
  gamescreen(2);

  getcstat = ' ';
  
  dialog::init(XLAT("HyperRogue %1: online demo", VER), 0xC00000, 200, 100);
  dialog::addBreak(50);

  dialog::addItem(XLAT("play the game"), 'f');
  dialog::addItem(XLAT("learn about hyperbolic geometry"), 'T');
  dialog::addHelp(); 
  // dialog::addItem(XLAT("toggle high detail"), 'a');
  dialog::addBreak(100);

  dialog::addTitle("highlights", 0xC00000, 120);
  dialog::addItem(XLAT("Temple of Cthulhu"), 't');
  dialog::addItem(XLAT("Land of Storms"), 'l');
  dialog::addItem(XLAT("Burial Grounds"), 'b');

  dialog::display();
  
  keyhandler = [] (int sym, int uni) {
    dialog::handleNavigation(sym, uni);
    if(sym == SDLK_F1 || uni == 'h') gotoHelp(help);
    else if(uni == 'a') {
      toggleanim(!demoanim);
      popScreen();
      }
    else if(uni == 'f') {
      firstland = laIce;
      restart_game(tactic::on ? rg::tactic : rg::nothing);
      }
#if CAP_TOUR
    else if(uni == 'T') {
      firstland = laIce;
      if(!tour::on) tour::start();
      }
#endif
    else if(uni == 't') {
      firstland = laTemple;
      restart_game(tactic::on ? rg::tactic : rg::nothing);
      }
    else if(uni == 'l') {
      firstland = laStorms;
      restart_game(tactic::on ? rg::tactic : rg::nothing);
      }
    else if(uni == 'b') {
      firstland = laBurial;
      restart_game(tactic::on ? rg::tactic : rg::nothing);
      items[itOrbSword] = 60;
      }
    };
  }

int bak_xres, bak_yres;

EM_BOOL fsc_callback(int eventType, const EmscriptenFullscreenChangeEvent *fullscreenChangeEvent, void *userData) {
  if(fullscreenChangeEvent->isFullscreen) {
    bak_xres = vid.xres;
    bak_yres = vid.yres;
    vid.xres = vid.xscr = fullscreenChangeEvent->screenWidth;
    vid.yres = vid.yscr = fullscreenChangeEvent->screenHeight;
    vid.full = true;
    printf("set to %d x %d\n", vid.xres, vid.yres);
    setvideomode();
    }
  else {
    vid.xres = vid.xscr = bak_xres;
    vid.yres = vid.yscr = bak_yres;
    vid.full = true;
    printf("reset to %d x %d\n", vid.xres, vid.yres);
    setvideomode();
    }
  return true;
  }

void initweb() {
  // toggleanim(false);
  emscripten_set_fullscreenchange_callback(0, NULL, false, fsc_callback);
  printf("showstartmenu = %d\n", showstartmenu);
  if(showstartmenu) pushScreen(showDemo);
  }

#if CAP_ORIENTATION
transmatrix getOrientation() {
  ld alpha, beta, gamma;
  alpha = EM_ASM_DOUBLE({ return rotation_alpha; });
  beta = EM_ASM_DOUBLE({ return rotation_beta; });
  gamma = EM_ASM_DOUBLE({ return rotation_gamma; });
  return 
    cspin(0, 1, alpha * degree) *
    cspin(1, 2, beta * degree) *
    cspin(0, 2, gamma * degree);
  }
#endif

void emscripten_get_commandline() {
#ifdef EMSCRIPTEN_FIXED_ARG
  string s = EMSCRIPTEN_FIXED_ARG;
#else
  char *str = (char*)EM_ASM_INT({
    var jsString = document.location.href;
    if (typeof(default_arg) != 'undefined' && jsString.indexOf('?') == -1)
      jsString = default_arg;
    var lengthBytes = lengthBytesUTF8(jsString)+1;
    var stringOnWasmHeap = _malloc(lengthBytes);
    stringToUTF8(jsString, stringOnWasmHeap, lengthBytes+1);
    return stringOnWasmHeap;
    });
  string s = str;
#endif
  s += "+xxxxxx";
  for(int i=0; i<isize(s); i++) if(s[i] == '?') {
#ifndef EMSCRIPTEN_FIXED_ARG
    printf("HREF: %s\n", str);
#endif
    arg::argument.push_back("hyperweb"); arg::lshift();
    string next = ""; i += 3;
    for(; i<isize(s); i++)  {
      if(s[i] == '+') {
        arg::argument.push_back(next);
        next = "";
        }
      else if(s[i] == '%') {
        string s2 = "";
        s2 += s[i+1];
        s2 += s[i+2];
        i += 2;
        next += strtol(s2.c_str(), NULL, 16);
        }
      else if(s[i] == '&') { 
        arg::argument.push_back(next); break; 
        }
      else next += s[i];
      }
    printf("Arguments:"); for(string s: arg::argument) printf(" %s", s.c_str()); printf("\n");
    break;
    }

#ifndef EMSCRIPTEN_FIXED_ARG
  free(str);
#endif
  }
}

