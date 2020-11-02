// Hyperbolic Rogue -- language support
// Copyright (C) 2011-2019 Zeno Rogue, see 'hyper.cpp' for details

/** \file language.cpp
 *  \brief localization support
 */

// #define CHECKTRANS

#include "hyper.h"
namespace hr {

EX const string dnameof(eMonster m) { return m >= 0 && m < motypes ? minf[m].name : format("[MONSTER %d]", m); }
EX const string dnameof(eLand l) { return l >= 0 && l < landtypes ? linf[l].name : format("[LAND %d]", l); }
EX const string dnameof(eWall w) { return w >= 0 && w < walltypes ? winf[w].name : format("[WALL %d]", w); }
EX const string dnameof(eItem i) { return i >= 0 && i < ittypes ? iinf[i].name : format("[ITEM %d]", i); }

#if HDR
#define NUMLAN 7

struct stringpar {
  string v;
  stringpar(string s) : v(s) { }
  stringpar(const char* s) : v(s) { }
  stringpar(eMonster m) { v= minf[m].name; }
  stringpar(eLand l) { v= linf[l].name; }
  stringpar(eWall w) { v= winf[w].name; }
  stringpar(eItem i) { v= iinf[i].name; }  
  };
#endif

void rep(string& pattern, string what, string to) {
  while(true) {
    size_t at = pattern.find(what);
    if(at == string::npos) break;
      pattern = pattern.replace(at, what.size(), to);
    }
  }

void reponce(string& pattern, string what, string to) {
  size_t at = pattern.find(what);
  if(at != string::npos)
    pattern = pattern.replace(at, what.size(), to);
  }

typedef unsigned hashcode;

struct sentence {
  hashcode langhash;
  const char* xlat[NUMLAN-1];
  };

struct noun {
  int genus;
  const char *nom, *nomp, *acc, *abl;
  };

struct fullnoun {
  hashcode langhash;
  int english_grammar_flags;
  noun n[NUMLAN-1];
  };

#if !CAP_TRANS
#if HDR
#define NUMEXTRA 12
extern const char* natchars[NUMEXTRA];
#endif
const char* natchars[NUMEXTRA] = {"°","é","á", "²", "½", "Θ", "δ", "π", "ϕ", "ᵈ", "∞", "⌫"};
#endif

#if CAP_TRANS
#include "language-data.cpp"

hashcode langhash(const string& s) {
  hashcode r = 0;
  for(int i=0; i<isize(s); i++) r = hashval * r + s[i];
  return r;
  }

template<class T> const T* findInHashTableS(string s, const T *table, int size) {
  int b = 0, e = size;
  hashcode h = langhash(s);
  while(b!=e) {
    int m = (b+e)>>1;
    // printf("b=%d e=%d m=%d h=%x s=%x\n", b, e, m, table[m].langhash, h);
    if(table[m].langhash >= h) e = m;
    else b = m+1;
    }
  if(e != size && table[e].langhash == h)
    return &table[e];
  return NULL;
  }

#define findInHashTable(s,t) findInHashTableS(s, t, sizeof(t) / sizeof(t[0]))
#endif

string choose3(int g, string a, string b, string c) {
  if(g == GEN_M || g == GEN_O) return a;
  if(g == GEN_F) return b;
  if(g == GEN_N) return c;
  return "unknown genus";
  }

string choose4(int g, string a, string b, string c, string d) {
  if(g == GEN_M) return a;
  if(g == GEN_F) return b;
  if(g == GEN_N) return c;
  if(g == GEN_O) return d;
  return "unknown genus";
  }

set<string> warnshown;

void basicrep(string& x) {

#if CAP_TRANS
  const sentence *s = findInHashTable(x, all_sentences);
  if(!s && !warnshown.count(x)) {
    printf("WARNING: no translations for '%s'\n", x.c_str());
    warnshown.insert(x);
    }

  int l = lang();
  if(l) {
    const sentence *s = findInHashTable(x, all_sentences);
    if(s) x = s->xlat[l-1];
    }
  
  if(l == 1) {
    rep(x, "%łeś0", choose3(playergender(), "łeś", "łaś", "łoś"));
    rep(x, "%ąłeś0", choose3(playergender(), "ąłeś", "ęłaś", "ęłoś"));
    rep(x, "%ógł0", choose3(playergender(), "ógł", "ogła", "ogło"));
    rep(x, "%ł0", choose3(playergender(), "ł", "ła", "ło"));
    rep(x, "%y0", choose3(playergender(), "y", "a", "e"));
    }
  
  if(l == 3) {
    rep(x, "%l0", choose3(playergender(), "l", "la", "lo"));
    rep(x, "%d0", choose3(playergender(), "", "a", "o"));
    }

  if(l == 4) {
    rep(x, "%E0", choose3(playergender(), "", "а", "о"));
    rep(x, "%A0", choose3(playergender(), "ый", "ая", "ое"));
    rep(x, "%c0", choose3(playergender(), "ся", "ась", ""));
    rep(x, "%y0", choose3(playergender(), "ый", "ая", "ое"));
    }

#endif
  }

void parrep(string& x, string w, stringpar p) {
#if CAP_TRANS
  int l = lang();
  const fullnoun *N = findInHashTable(p.v, all_nouns);
  if(l == 1) {
    if(N) {
      rep(x, "%"+w, N->n[0].nom);
      rep(x, "%P"+w, N->n[0].nomp);
      rep(x, "%a"+w, N->n[0].acc);
      rep(x, "%abl"+w, N->n[0].abl);
      rep(x, "%ł"+w, choose3(N->n[0].genus, "ł", "ła", "ło"));
      rep(x, "%łem"+w, choose3(N->n[0].genus, "łem", "łam", "łom"));
      rep(x, "%łeś"+w, choose3(N->n[0].genus, "łeś", "łaś", "łoś"));
      rep(x, "%ął"+w, choose3(N->n[0].genus, "ął", "ęła", "ęło"));
      rep(x, "%ya"+w, choose3(N->n[0].genus, "y", "a", "e"));
      rep(x, "%yą"+w, choose4(N->n[0].genus, "ego", "ą", "e", "y"));
      rep(x, "%oa"+w, choose3(N->n[0].genus, "", "a", "o"));
      rep(x, "%ymą"+w, choose3(N->n[0].genus, "ym", "ą", "ym"));
      rep(x, "%go"+w, choose3(N->n[0].genus, "go", "ją", "je"));
      rep(x, "%aka"+w, choose3(N->n[0].genus, "a", "ka", "a"));
      }
    else {
      rep(x,"%"+w, p.v);
      rep(x, "%P"+w, p.v);
      rep(x, "%a"+w, p.v);
      rep(x, "%abl"+w, p.v);
      rep(x, "%ł"+w, choose3(0, "ł", "ła", "ło"));
      }
    }
  if(l == 2) {
    if(N) {
      rep(x, "%"+w, N->n[1].nom);
      rep(x, "%P"+w, N->n[1].nomp);
      rep(x, "%a"+w, N->n[1].acc);
      rep(x, "%abl"+w, N->n[1].abl);
      }
    else {
      rep(x,"%"+w,p.v);
      rep(x, "%P"+w, p.v);
      rep(x, "%a"+w, p.v);
      rep(x, "%abl"+w, p.v);
      }
    }
  if(l == 3) {
    if(N) {
      rep(x, "%"+w, N->n[2].nom);
      rep(x, "%P"+w, N->n[2].nomp);
      rep(x, "%a"+w, N->n[2].acc);
      rep(x, "%abl"+w, N->n[2].abl);
      rep(x, "%ý"+w, choose3(N->n[2].genus, "ý", "á", "é"));
      rep(x, "%l"+w, choose3(N->n[2].genus, "l", "la", "lo"));
      rep(x, "%el"+w, choose3(N->n[2].genus, "el", "la", "lo"));
      rep(x, "%ůj"+w, choose4(N->n[2].genus, "ého", "ou", "é", "ůj"));
      rep(x, "%ým"+w, choose3(N->n[2].genus, "ým", "ou", "ým"));
      rep(x, "%ho"+w, choose3(N->n[2].genus, "ho", "ji", "ho"));
      rep(x, "%ého"+w, choose3(N->n[2].genus, "ého", "ou", "ého"));

      if(p.v == "Mirror Image")
        rep(x, "%s"+w, "se");
      if(p.v == "Mirage")
        rep(x, "%s"+w, "s");
      }
    else {
      rep(x,"%"+w,p.v);
      rep(x, "%P"+w, p.v);
      rep(x, "%a"+w, p.v);
      rep(x, "%abl"+w, p.v);
      }
    }
  if(l == 4) {
    if(N) {
      rep(x, "%"+w, N->n[3].nom);
      rep(x, "%P"+w, N->n[3].nomp);
      rep(x, "%a"+w, N->n[3].acc);
      rep(x, "%abl"+w, N->n[3].abl);
      rep(x, "%E"+w, choose3(N->n[3].genus, "", "а", "о"));
      rep(x, "%A"+w, choose3(N->n[3].genus, "ый", "ая", "ое"));
      rep(x, "%c"+w, choose3(N->n[3].genus, "ся", "ась", ""));
      rep(x, "%y"+w, choose3(N->n[3].genus, "ый", "ая", "ое"));
      }
    else {
      rep(x,"%"+w,p.v);
      rep(x, "%P"+w, p.v);
      rep(x, "%a"+w, p.v);
      rep(x, "%abl"+w, p.v);
      }
    }
  if(l == 5) {
    if(N) {
      rep(x, "%"+w, N->n[4].nom);
      rep(x, "%P"+w, N->n[4].nomp);
      rep(x, "%a"+w, N->n[4].acc);
      rep(x, "%abl"+w, N->n[4].abl);
      rep(x, "%d"+w, N->n[4].abl); // Dativ (which equals Ablative in German)
      rep(x, "%Der"+w, choose3(N->n[4].genus, "Der", "Die", "Das"));
      rep(x, "%der"+w, choose3(N->n[4].genus, "der", "die", "das"));
      rep(x, "%den"+w, choose3(N->n[4].genus, "den", "die", "das"));
      rep(x, "%dem"+w, choose3(N->n[4].genus, "dem", "der", "dem"));
      }
    else {
      rep(x,"%"+w,p.v);
      rep(x, "%P"+w, p.v);
      rep(x, "%a"+w, p.v);
      rep(x, "%abl"+w, p.v);
      rep(x, "%Der"+w, "The");
      rep(x, "%der"+w, "the");
      rep(x, "%den"+w, "the");
      }
    }
  if(l == 6) {
    if(N) {
      rep(x, "%"+w, N->n[5].nom);
      rep(x, "%P"+w, N->n[5].nomp);
      rep(x, "%na"+w, choose4(N->n[5].genus, "o", "a", "os", "as") + " " + N->n[5].nom);
      rep(x, "%Na"+w, choose4(N->n[5].genus, "O", "A", "Os", "As") + " " + N->n[5].nom);
      rep(x, "%oa"+w, choose4(N->n[5].genus, "o", "a", "os", "as"));
      rep(x, "%g"+w, choose4(N->n[5].genus, "do", "da", "dos", "das")+ " " + N->n[5].nom);
      rep(x, "%d"+w, choose4(N->n[5].genus, "ao", "à", "aos", "às")+ " " + N->n[5].nom);
      rep(x, "%l"+w, choose4(N->n[5].genus, "no", "na", "nos", "nas")+ " " + N->n[5].nom);
      rep(x, "%abl"+w, choose4(N->n[5].genus, "pelo", "pela", "pelos", "pelas")+ " " + N->n[5].nom);
      rep(x, "%seu"+w, choose4(N->n[5].genus, "seu", "sua", "seus", "suas"));
      }
    else {
      rep(x, "%"+w,p.v);
      rep(x, "%P"+w, p.v);
      rep(x, "%na"+w, p.v);
      rep(x, "%g"+w, p.v);
      rep(x, "%d"+w, p.v);
      rep(x, "%l"+w, p.v);
      rep(x, "%abl"+w, p.v);
      }
    }
#endif
  if(true) {
    // proper names (R'Lyeh)
    rep(x,"%"+w,p.v);
#if !CAP_TRANS
    int flags = 0;
    if(p.v == "R'Lyeh" || p.v == "Camelot")  flags = 1;
    if(p.v == "Crossroads" || p.v == "Crossroads II" ||
      p.v == "Crossroads III" || p.v == "Crossroads IV" ||
      p.v == "Kraken Depths" || p.v == "Elemental Planes")
      flags = 2;
#else
    int flags = N ? N->english_grammar_flags : 0;
#endif
    if(flags & 1) {
      rep(x,"%the"+w, p.v);
      rep(x,"%The"+w, p.v);
      }
    else {
      rep(x,"%the"+w, "the " + p.v);
      rep(x,"%The"+w, "The " + p.v);
      rep(x,"%him"+w, princessgender() ? "her" : "him");
      rep(x,"%his"+w, princessgender() ? "her" : "his");
      }
    // plural names (Crossroads)
    if(flags & 2)
      rep(x,"%s"+w, "");
    else 
      rep(x,"%s"+w, "s");
    }
  return;
  }

void postrep(string& s) {
  }

/** translate the string @x */
EX string XLAT(string x) { 
  basicrep(x);
  postrep(x);
  return x;
  }
EX string XLAT(string x, stringpar p1) { 
  basicrep(x);
  parrep(x,"1",p1.v);
  postrep(x);
  return x;
  }
EX string XLAT(string x, stringpar p1, stringpar p2) { 
  basicrep(x);
  parrep(x,"1",p1.v);
  parrep(x,"2",p2.v);
  postrep(x);
  return x;
  }
EX string XLAT(string x, stringpar p1, stringpar p2, stringpar p3) { 
  basicrep(x);
  parrep(x,"1",p1.v);
  parrep(x,"2",p2.v);
  parrep(x,"3",p3.v);
  postrep(x);
  return x;
  }
EX string XLAT(string x, stringpar p1, stringpar p2, stringpar p3, stringpar p4) { 
  basicrep(x);
  parrep(x,"1",p1.v);
  parrep(x,"2",p2.v);
  parrep(x,"3",p3.v);
  parrep(x,"4",p4.v);
  postrep(x);
  return x;
  }
EX string XLAT(string x, stringpar p1, stringpar p2, stringpar p3, stringpar p4, stringpar p5) { 
  basicrep(x);
  parrep(x,"1",p1.v);
  parrep(x,"2",p2.v);
  parrep(x,"3",p3.v);
  parrep(x,"4",p4.v);
  parrep(x,"5",p5.v);
  postrep(x);
  return x;
  }


EX string XLATN(string x) { 
#if CAP_TRANS
  if(lang()) {
    const fullnoun *N = findInHashTable(x, all_nouns);
    if(N) return N->n[lang()-1].nomp;
    }
#endif
  return x;
  }

EX string XLAT1(string x) {
#if CAP_TRANS
  if(lang()) {
    const fullnoun *N = findInHashTable(x, all_nouns);
    if(N) return N->n[lang()-1].nom;
    }
#endif
  return x;
  }

EX string XLATT1(stringpar p) { return XLAT1(p.v); }

EX bool shapename_available(int id) { return id >= 3 && id < 9; }

EX string shapename(int id) {
  if(!shapename_available(id)) return "";
  string english[9] = {"", "", "digon", "triangle", "square", "pentagon", "hexagon", "heptagon", "octagon"};
  return english[id];
  }

}
