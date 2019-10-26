// Hyperbolic Rogue -- raycaster
// Copyright (C) 2011-2019 Zeno Rogue, see 'hyper.cpp' for details

/** \file raycaster.cpp
 *  \brief A raycaster to draw walls.
 */

#include "hyper.h"

namespace hr {

EX namespace ray {

/** texture IDs */
GLuint txConnections = 0, txWallcolor = 0, txTextureMap = 0;

EX bool in_use;
EX bool comparison_mode;

/** 0 - never use, 2 - always use, 1 = smart selection */
EX int want_use = 1;

EX ld exp_start = 1, exp_decay_exp = 4, exp_decay_poly = 10;

EX ld maxstep_sol = .02;
EX ld maxstep_nil = .1;
EX ld minstep = .001;

EX int max_iter_sol = 600, max_iter_iso = 60;

EX int max_cells = 8192;
EX bool rays_generate = true;

ld& exp_decay_current() {
  return (sol || hyperbolic) ? exp_decay_exp : exp_decay_poly;
  }

int& max_iter_current() {
  if(nonisotropic) return max_iter_sol;
  else return max_iter_iso;
  }

ld& maxstep_current() {
  if(sol) return maxstep_sol;
  else return maxstep_nil;
  }

#define IN_ODS 0

/** is the raycaster available? */
EX bool available() {
  if(WDIM == 2) return false;
  if(hyperbolic && pmodel == mdPerspective && !binarytiling)
    return true;
  if((sol || nil) && pmodel == mdGeodesic)
    return true;
  if(euclid && pmodel == mdPerspective && !binarytiling)
    return true;
  return false;
  }

/** do we want to use the raycaster? */
EX bool requested() {
  if(!want_use) return false;
  if(!available()) return false;
  if(want_use == 2) return true;
  return racing::on || quotient;
  }

struct raycaster : glhr::GLprogram {
  GLint uStart, uStartid, uM, uLength, uFovX, uFovY, uIPD;
  GLint uWallstart, uWallX, uWallY;
  GLint tConnections, tWallcolor, tTextureMap;
  GLint uBinaryWidth;
  GLint uLinearSightRange, uExpStart, uExpDecay;
  
  raycaster(string vsh, string fsh) : GLprogram(vsh, fsh) {
    println(hlog, "assigning");
    uStart = glGetUniformLocation(_program, "uStart");
    uStartid = glGetUniformLocation(_program, "uStartid");
    uM = glGetUniformLocation(_program, "uM");
    uLength = glGetUniformLocation(_program, "uLength");
    uFovX = glGetUniformLocation(_program, "uFovX");
    uFovY = glGetUniformLocation(_program, "uFovY");
    uIPD = glGetUniformLocation(_program, "uIPD");

    uWallstart = glGetUniformLocation(_program, "uWallstart");
    uWallX = glGetUniformLocation(_program, "uWallX");
    uWallY = glGetUniformLocation(_program, "uWallY");
    
    uBinaryWidth = glGetUniformLocation(_program, "uBinaryWidth");

    uLinearSightRange = glGetUniformLocation(_program, "uLinearSightRange");
    uExpDecay = glGetUniformLocation(_program, "uExpDecay");
    uExpStart = glGetUniformLocation(_program, "uExpStart");
  
    tConnections = glGetUniformLocation(_program, "tConnections");
    tWallcolor = glGetUniformLocation(_program, "tWallcolor");
    tTextureMap = glGetUniformLocation(_program, "tTextureMap");
    }
  };

shared_ptr<raycaster> our_raycaster;

void reset_raycaster() { our_raycaster = nullptr; };

void enable_raycaster() {
  if(!our_raycaster) { 
    bool use_reflect = false;

    string vsh = 
      "attribute vec4 aPosition;\n"
      "uniform float uFovX, uFovY;\n"
      "varying vec4 at;\n"
      "void main() { \n"
      "  gl_Position = aPosition; at = aPosition;\n"
  #if IN_ODS    
      "  at[0] *= PI; at[1] *= PI; \n"
  #else
      "  at[0] *= uFovX; at[1] *= uFovY; \n"
  #endif
      "  }\n";
  
    string fsh = 
    "varying vec4 at;\n"
    "uniform int uLength;\n"
    "uniform float uIPD;\n"
    "uniform mat4 uStart;\n"
    "uniform mat4 uM[80];\n"
    "uniform mat4 uTest;\n"
    "uniform vec2 uStartid;\n"
    "uniform sampler2D tConnections;\n"
    "uniform sampler2D tWallcolor;\n"
    "uniform sampler2D tTexture;\n"
    "uniform sampler2D tTextureMap;\n"
    "uniform vec4 uWallX[60];\n"
    "uniform vec4 uWallY[60];\n"
    "uniform int uWallstart[16];\n"
    "uniform float uLinearSightRange, uExpStart, uExpDecay;\n";
    
    if(IN_ODS) fsh += 

    "mat4 xpush(float x) { return mat4("
         "cosh(x), 0., 0., sinh(x),\n"
         "0., 1., 0., 0.,\n"
         "0., 0., 1., 0.,\n"
         "sinh(x), 0., 0., cosh(x)"
         ");}\n"

    "mat4 xzspin(float x) { return mat4("
         "cos(x), 0., sin(x), 0.,\n"
         "0., 1., 0., 0.,\n"
         "-sin(x), 0., cos(x), 0.,\n"
         "0., 0., 0., 1."
         ");}\n"
      
    "mat4 yzspin(float x) { return mat4("
         "1., 0., 0., 0.,\n"
         "0., cos(x), sin(x), 0.,\n"
         "0., -sin(x), cos(x), 0.,\n"
         "0., 0., 0., 1."
         ");}\n";    
    
   fsh += 
     "vec2 map_texture(vec4 pos, int which) {\n";
   if(nil) fsh += "if(which == 2 || which == 5) pos.z = 0.;\n";
   if(hyperbolic) fsh += 
       "pos /= pos.w;\n";
   fsh += 
       "int s = uWallstart[which];\n"
       "int e = uWallstart[which+1];\n"
       "for(int i=s; i<e; i++) {\n"
         "vec2 v = vec2(dot(uWallX[i], pos), dot(uWallY[i], pos));\n"
         "if(v.x >= 0. && v.y >= 0. && v.x + v.y <= 1.) return vec2(v.x+v.y, v.x-v.y);\n"
         "}\n"
       "return vec2(1, 1);\n"
       "}\n";
    
   string fmain = "void main() {\n";
    
    if(IN_ODS) fmain +=
    "  float lambda = at[0];\n" // -PI to PI
    "  float phi;\n"
    "  float eye;\n"
    "  if(at.y < 0.) { phi = at.y + PI/2.; eye = uIPD / 2.; }\n" // right
    "  else { phi = at.y - PI/2.; eye = -uIPD / 2.; }\n"
    "  mat4 vw = uStart * xzspin(-lambda) * xpush(eye) * yzspin(phi);\n"
    "  vec4 at0 = vec4(0., 0., 1., 0.);\n";
    
    else fmain += 
    "  mat4 vw = uStart;\n"
    "  vec4 at0 = at;\n"
    "  gl_FragColor = vec4(0,0,0,1);\n"
    "  float left = 1.;\n"
    "  at0.y = -at.y;\n"
    "  at0.w = 0.;\n"
    "  at0.xyz = at0.xyz / length(at0.xyz);\n";
      
    if(hyperbolic) fsh += "  float len(vec4 x) { return x[3]; }\n";
    else fsh += "  float len(vec4 x) { return length(x.xyz); }\n";
    
    if(nonisotropic) fmain += 
      "  const float maxstep = " + fts(maxstep_current()) + ";\n"
      "  const float minstep = " + fts(minstep) + ";\n"
      "  float next = maxstep;\n";
    
    fmain +=     
      "  vec4 position = vw * vec4(0., 0., 0., 1.);\n"
      "  vec4 tangent = vw * at0;\n"
      "  float go = 0.;\n"
      "  vec2 cid = uStartid;\n"
      "  for(int iter=0; iter<" + its(max_iter_current()) + "; iter++) {\n";
    
    fmain +=
      "  float dist = 100.;\n";
    
    fmain +=
      "  int which = -1;\n";
      
    if(IN_ODS) fmain += 
      "  if(go == 0.) {\n"
      "    float best = len(position);\n"
      "    for(int i=0; i<"+its(S7)+"; i++) {\n"
      "      float cand = len(uM[i] * position);\n"
      "      if(cand < best - .001) { dist = 0.; best = cand; which = i; }\n"
      "      }\n"
      "    }\n";
    
    if(!nonisotropic) {
    
      fmain +=
        "  if(which == -1) for(int i=0; i<"+its(S7)+"; i++) {\n";
      
      if(hyperbolic) fmain +=
          "    float v = ((position - uM[i] * position)[3] / (uM[i] * tangent - tangent)[3]);\n"
          "    if(v > 1. || v < -1.) continue;\n"
          "    float d = atanh(v);\n"
          "    vec4 next_tangent = position * sinh(d) + tangent * cosh(d);\n"
          "    if(next_tangent[3] < (uM[i] * next_tangent)[3]) continue;\n";
      else fmain += 
          "    float deno = dot(position, tangent) - dot(uM[i]*position, uM[i]*tangent);\n"
          "    if(deno < 1e-6  && deno > -1e-6) continue;\n"
          "    float d = (dot(uM[i]*position, uM[i]*position) - dot(position, position)) / 2. / deno;\n"
          "    if(d < 0.) continue;\n"
          "    vec4 next_position = position + d * tangent;\n"
          "    if(dot(next_position, tangent) < dot(uM[i]*next_position, uM[i]*tangent)) continue;\n";
  
      fmain += 
          "  if(d < dist) { dist = d; which = i; }\n"
            "}\n";

      fmain += 
        "  if(dist < 0.) { dist = 0.; }\n";
      
      fmain +=
        "  if(which == -1 && dist == 0.) return;";    
      }
        
    // shift d units
    
    if(hyperbolic) fmain += 
      "  float ch = cosh(dist); float sh = sinh(dist);\n"
      "  vec4 v = position * ch + tangent * sh;\n"
      "  tangent = tangent * ch + position * sh;\n"
      "  position = v;\n";
    else if(nonisotropic) {
    
      if(sol) fsh += 
        "vec4 christoffel(vec4 pos, vec4 vel, vec4 tra) {\n"
        "  return vec4(-vel.z*tra.x - vel.x*tra.z, vel.z*tra.y + vel.y * tra.z, vel.x*tra.x * exp(2.*pos.z) - vel.y * tra.y * exp(-2.*pos.z), 0.);\n"
        "  }\n";
      else fsh +=
        "vec4 christoffel(vec4 pos, vec4 vel, vec4 tra) {\n"
        "  float x = pos.x;\n"
        "  return vec4(x*vel.y*tra.y - 0.5*dot(vel.yz,tra.zy), -.5*x*dot(vel.yx,tra.xy) + .5 * dot(vel.zx,tra.xz), -.5*(x*x-1.)*dot(vel.yx,tra.xy)+.5*x*dot(vel.zx,tra.xz), 0.);\n"
//        "  return vec4(0.,0.,0.,0.);\n"
        "  }\n";
      
      if(sol) fsh += "uniform float uBinaryWidth;\n";
      
      fmain += 
        "  dist = next < minstep ? 2.*next : next;\n";

      if(nil) fsh += 
        "vec4 translate(vec4 a, vec4 b) {\n"
          "return vec4(a[0] + b[0], a[1] + b[1], a[2] + b[2] + a[0] * b[1], b[3]);\n"
          "}\n"
        "vec4 translatev(vec4 a, vec4 t) {\n"
          "return vec4(t[0], t[1], t[2] + a[0] * t[1], 0.);\n"
          "}\n"
        "vec4 itranslate(vec4 a, vec4 b) {\n"
          "return vec4(-a[0] + b[0], -a[1] + b[1], -a[2] + b[2] - a[0] * (b[1]-a[1]), b[3]);\n"
          "}\n"
        "vec4 itranslatev(vec4 a, vec4 t) {\n"
          "return vec4(t[0], t[1], t[2] - a[0] * t[1], 0.);\n"
          "}\n";
                
      if(nil) fmain += "tangent = translate(position, itranslate(position, tangent));\n";
      
      if(sol) fmain +=
        "vec4 acc = christoffel(position, tangent, tangent);\n"
        "vec4 pos2 = position + tangent * dist / 2.;\n"
        "vec4 tan2 = tangent + acc * dist / 2.;\n"
        "vec4 acc2 = christoffel(pos2, tan2, tan2);\n"
        "vec4 nposition = position + tangent * dist + acc2 / 2. * dist * dist;\n";
      
      if(use_reflect) fmain += 
        "bool reflect = false;\n";

      if(nil) {
        fmain +=
          "vec4 xp, xt;\n"
          "vec4 back = itranslatev(position, tangent);\n"
          "if(back.x == 0. && back.y == 0.) {\n"
          "  xp = vec4(0., 0., back.z*dist, 1.);\n"
          "  xt = back;\n"
          "  }\n"
          "else if(abs(back.z) == 0.) {\n"
          "  xp = vec4(back.x*dist, back.y*dist, back.x*back.y*dist*dist/2., 1.);\n"
          "  xt = vec4(back.x, back.y, dist*back.x*back.y, 0.);\n"
          "  }\n"
          "else if(abs(back.z) < 1e-1) {\n"
// we use the midpoint method here, because the formulas below cause glitches due to float precision
          "  vec4 acc = christoffel(vec4(0,0,0,1), back, back);\n"
          "  vec4 pos2 = back * dist / 2.;\n"
          "  vec4 tan2 = back + acc * dist / 2.;\n"
          "  vec4 acc2 = christoffel(pos2, tan2, tan2);\n"
          "  xp = vec4(0,0,0,1) + back * dist + acc2 / 2. * dist * dist;\n"
          "  xt = back + acc * dist;\n"
          "  }\n"
          "else {\n"
          "  float alpha = atan2(back.y, back.x);\n"
          "  float w = back.z * dist;\n"
          "  float c = length(back.xy) / back.z;\n"
          "  xp = vec4(2.*c*sin(w/2.) * cos(w/2.+alpha), 2.*c*sin(w/2.)*sin(w/2.+alpha), w*(1.+(c*c/2.)*((1.-sin(w)/w)+(1.-cos(w))/w * sin(w+2.*alpha))), 1.);\n"
          "  xt = back.z * vec4("
               "c*cos(alpha+w),"
               "c*sin(alpha+w),"
               "1. + c*c*2.*sin(w/2.)*sin(alpha+w)*cos(alpha+w/2.),"
               "0.);\n"
          "  }\n"
          "vec4 nposition = translate(position, xp);\n";
        }
      
      if(nil) fmain +=
        "float rz = (abs(nposition.x) > abs(nposition.y) ?  -nposition.x*nposition.y : 0.) + nposition.z;\n";

      fmain +=
        "if(next >= minstep) {\n";
      
      if(sol) fmain +=
          "if(abs(nposition.x) > uBinaryWidth || abs(nposition.y) > uBinaryWidth || abs(nposition.z) > log(2.)/2.) {\n";
      else fmain +=
          "if(abs(nposition.x) > .5 || abs(nposition.y) > .5 || abs(rz) > .5) {\n";
      
      fmain +=
            "next = dist / 2.; continue;\n"
            "}\n"
          "if(next < maxstep) next = next / 2.;\n"
          "}\n"
        "else {\n";
      
      if(sol) fmain +=
          "if(nposition.x > uBinaryWidth) which = 0;\n"
          "if(nposition.x <-uBinaryWidth) which = 4;\n"
          "if(nposition.y > uBinaryWidth) which = 1;\n"
          "if(nposition.y <-uBinaryWidth) which = 5;\n"
          "if(nposition.z > log(2.)/2.) which = nposition.x > 0. ? 3 : 2;\n"
          "if(nposition.z <-log(2.)/2.) which = nposition.y > 0. ? 7 : 6;\n";
      else fmain +=
          "if(nposition.x > .5) which = 3;\n"
          "if(nposition.x <-.5) which = 0;\n"
          "if(nposition.y > .5) which = 4;\n"
          "if(nposition.y <-.5) which = 1;\n"
          "if(rz > .5) which = 5;\n"
          "if(rz <-.5) which = 2;\n";
      
      fmain += 
          "next = maxstep;\n"
          "}\n";
      
      if(nil) fmain +=
        "tangent = translatev(position, xt);\n";

      fmain +=
        "position = nposition;\n";
      
      if(!nil) fmain +=
        "tangent = tangent + acc * dist;\n";
      }
    else fmain += 
      "position = position + tangent * dist;\n";
    
    fmain += "  go = go + dist;\n";
    
    fmain += "if(which == -1) continue;\n";
    
    // apply wall color
    fmain +=
      "  vec2 u = cid + vec2(float(which) / float(uLength), 0);\n"
      "  vec4 col = texture2D(tWallcolor, u);\n"
      "  if(col[3] > 0.0) {\n"
      "    vec2 inface = map_texture(position, which);\n"
      "    vec3 tmap = texture2D(tTextureMap, u).rgb;\n"
      "    if(tmap.z == 0.) col.xyz *= min(1., (1.-inface.x)/ tmap.x);\n"
      "    else {\n"
      "      vec2 inface2 = tmap.xy + tmap.z * inface;\n"
      "      col.xyz *= texture2D(tTexture, inface2).rgb;\n"
      "      }\n"
      "    col.xyz *= max(1. - go / uLinearSightRange, uExpStart * exp(-go / uExpDecay));\n";
    
    if(nil) fmain +=
      "    if(abs(abs(position.x)-abs(position.y)) < .005) col.xyz /= 2.;\n";
    
    if(use_reflect) fmain +=
      "  if(col.w == 1.) {\n"
      "    col.w = 0.9;\n"
      "    reflect = true;\n"
      "    }\n";
    
    ld vnear = glhr::vnear_default;
    ld vfar = glhr::vfar_default;

    fmain +=
      "    gl_FragColor.xyz += left * col.xyz * col.w;\n"
      "    if(col.w == 1.) {\n";
    
    if(hyperbolic) fmain +=
      "      float z = at0.z * sinh(go);\n"
      "      float w = 1.;\n";
    else fmain +=
      "      float z = at0.z * go;\n"
      "      float w = 1.;\n";
    
    fmain +=    
      "      gl_FragDepth = (-float("+fts(vnear+vfar)+")+w*float("+fts(2*vnear*vfar)+")/z)/float("+fts(vnear-vfar)+");\n"
      "      gl_FragDepth = (gl_FragDepth + 1.) / 2.;\n"
      "      return;\n"
      "      }\n"
      "    left *= (1. - col.w);\n"
      "    }\n";

    // next cell
    fmain += 
      "  vec4 connection = texture2D(tConnections, u);\n"
      "  int mid = int(connection.z * 1024.);\n"
      "  position = uM[mid] * uM[which] * position;\n"
      "  tangent = uM[mid] * uM[which] *  tangent;\n"
      "  cid = connection.xy;\n";
    
    if(use_reflect) fmain += 
      "  if(reflect) {\n"
      "    if(which == 0 || which == 4) tangent.x = -tangent.x;\n"
      "    else if(which == 1 || which == 5) tangent.y = -tangent.y;\n"
      "    else tangent.z = -tangent.z;\n"
      "    }\n";
    
    fmain += 
      "  }\n"
      "  gl_FragDepth = 1.;\n"
      "  }";

    fsh += fmain;    
 
    our_raycaster = make_shared<raycaster> (vsh, fsh);
    }
  full_enable(our_raycaster);
  }

int length, per_row, rows;

void bind_array(vector<array<float, 4>>& v, GLint t, GLuint& tx, int id) {
  if(t == -1) println(hlog, "bind to nothing");
  glUniform1i(t, id);

  if(tx == 0) glGenTextures(1, &tx);

  glActiveTexture(GL_TEXTURE0 + id);
  glBindTexture(GL_TEXTURE_2D, tx);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, length, isize(v)/length, 0, GL_RGBA, GL_FLOAT, &v[0]);  
  GLERR("bind_array");
  }

void uniform2(GLint id, array<float, 2> fl) {
  glUniform2f(id, fl[0], fl[1]);
  }

array<float, 2> enc(int i, int a) { 
  array<float, 2> res;
  res[0] = ((i%per_row) * S7 + a + .5) / length;
  res[1] = ((i / per_row) + .5) / rows;
  return res;
  };

color_t color_out_of_range = 0xFF0080FF;

EX void cast() {
  enable_raycaster();
  
  if(comparison_mode) 
    glColorMask( GL_TRUE,GL_FALSE,GL_FALSE,GL_TRUE );

  auto& o = our_raycaster;
  
  vector<glvertex> screen = {
    glhr::makevertex(-1, -1, 1),
    glhr::makevertex(-1, +1, 1),
    glhr::makevertex(+1, -1, 1),
    glhr::makevertex(-1, +1, 1),
    glhr::makevertex(+1, -1, 1),
    glhr::makevertex(+1, +1, 1)
    };
  
  auto& cd = current_display;
  glUniform1f(o->uFovX, cd->tanfov);
  glUniform1f(o->uFovY, cd->tanfov * cd->ysize / cd->xsize);
  
  length = 4096;
  per_row = length / S7;
  
  vector<cell*> lst;

  if(true) {
    manual_celllister cl;
    cl.add(viewctr.at->c7);
    for(int i=0; i<isize(cl.lst); i++) {
      cell *c = cl.lst[i];
      if(racing::on && i > 0 && c->wall == waBarrier) continue;
      forCellCM(c2, c) {
        if(rays_generate) setdist(c2, 7, c);
        cl.add(c2);
        if(isize(cl.lst) >= max_cells) goto finish;
        }
      }
    finish:
    lst = cl.lst;
    }
  
  rows = next_p2((isize(lst)+per_row-1) / per_row);
  
  map<cell*, int> ids;
  for(int i=0; i<isize(lst); i++) ids[lst[i]] = i;

  glUniform1i(o->uLength, length);
  GLERR("uniform length");
  
  // for(auto &m: reg3::spins) println(hlog, m);
  
  glUniformMatrix4fv(o->uStart, 1, 0, glhr::tmtogl_transpose(inverse(View)).as_array());
  GLERR("uniform start");
  uniform2(o->uStartid, enc(ids[viewctr.at->c7], 0));
  GLERR("uniform startid");
  glUniform1f(o->uIPD, vid.ipd);
  GLERR("uniform IPD");

  vector<transmatrix> ms;
  for(int j=0; j<S7; j++) ms.push_back(currentmap->relative_matrix(cwt.at->master, cwt.at->cmove(j)->master));

  vector<array<float, 4>> connections(length * rows);
  vector<array<float, 4>> wallcolor(length * rows);
  vector<array<float, 4>> texturemap(length * rows);

  if(1) for(cell *c: lst) {
    int id = ids[c];
    forCellIdEx(c1, i, c) { 
      int u = (id/per_row*length) + (id%per_row * S7) + i;
      if(!ids.count(c1)) {
        wallcolor[u] = glhr::acolor(color_out_of_range | 0xFF);
        texturemap[u] = glhr::makevertex(0.1,0,0);
        continue;
        }
      auto code = enc(ids[c1], 0);
      connections[u][0] = code[0];
      connections[u][1] = code[1];
      if(isWall3(c1)) {
        celldrawer dd;
        dd.cw.at = c1;
        dd.setcolors();
        transmatrix Vf;
        dd.set_land_floor(Vf);
        color_t wcol = darkena(dd.wcol, 0, 0xFF);
        int dv = get_darkval(c1, c->c.spin(i));
        float p = 1 - dv / 16.;
        wallcolor[u] = glhr::acolor(wcol);
        for(int a: {0,1,2}) wallcolor[u][a] *= p;
        if(qfi.fshape) {
          texturemap[u] = floor_texture_map[qfi.fshape->id];
          }
        else
          texturemap[u] = glhr::makevertex(0.1,0,0);
        }
      else {
        color_t col = transcolor(c, c1, winf[c->wall].color) | transcolor(c1, c, winf[c1->wall].color);
        if(col == 0)
          wallcolor[u] = glhr::acolor(0);
        else {
          int dv = get_darkval(c1, c->c.spin(i));
          float p = 1 - dv / 16.;
          wallcolor[u] = glhr::acolor(col);
          for(int a: {0,1,2}) wallcolor[u][a] *= p;
          texturemap[u] = glhr::makevertex(0.001,0,0);
          }
        }

      transmatrix T = currentmap->relative_matrix(c->master, c1->master) * inverse(ms[i]);
      for(int k=0; k<=isize(ms); k++) {
        if(k < isize(ms) && !eqmatrix(ms[k], T)) continue;
        if(k == isize(ms)) ms.push_back(T);
        connections[u][2] = (k+.5) / 1024.;
        break;
        }
      }
    }

  vector<GLint> wallstart;
  for(auto i: cgi.wallstart) wallstart.push_back(i);
  glUniform1iv(o->uWallstart, isize(wallstart), &wallstart[0]);  
  
  vector<glvertex> wallx, wally;
  for(auto& m: cgi.raywall) {
    wallx.push_back(glhr::pointtogl(m[0]));
    wally.push_back(glhr::pointtogl(m[1]));
    }
  
  glUniform4fv(o->uWallX, isize(wallx), &wallx[0][0]);
  glUniform4fv(o->uWallY, isize(wally), &wally[0][0]);

  if(o->uBinaryWidth != -1)
    glUniform1f(o->uBinaryWidth, vid.binary_width * log(2) / 2);
    
  glUniform1f(o->uLinearSightRange, sightranges[geometry]);
  glUniform1f(o->uExpDecay, exp_decay_current());
  glUniform1f(o->uExpStart, exp_start);


  vector<glhr::glmatrix> gms;
  for(auto& m: ms) gms.push_back(glhr::tmtogl_transpose(m));
  glUniformMatrix4fv(o->uM, isize(gms), 0, gms[0].as_array());

  bind_array(wallcolor, o->tWallcolor, txWallcolor, 4);
  bind_array(connections, o->tConnections, txConnections, 3);
  bind_array(texturemap, o->tTextureMap, txTextureMap, 5);
  
  glVertexAttribPointer(hr::aPosition, 4, GL_FLOAT, GL_FALSE, sizeof(glvertex), &screen[0]);
  if(ray::comparison_mode)
    glhr::set_depthtest(false);
  else {
    glhr::set_depthtest(true);
    glhr::set_depthwrite(true);
    }
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glActiveTexture(GL_TEXTURE0 + 0);
  glBindTexture(GL_TEXTURE_2D, floor_textures->renderedTexture);

  glDrawArrays(GL_TRIANGLES, 0, 6);
  GLERR("finish");
  }

EX void configure() {
  cmode = sm::SIDE | sm::MAYDARK;
  gamescreen(0);
  dialog::init(XLAT("raycasting configuration"));
  
  dialog::addBoolItem(XLAT("available in current geometry"), available(), 0);
  
  dialog::addBoolItem(XLAT("use raycasting?"), want_use == 2 ? true : in_use, 'u');
  if(want_use == 1) dialog::lastItem().value = XLAT("SMART");
  
  dialog::add_action([] {
    want_use++; want_use %= 3;
    });

  dialog::addBoolItem_action(XLAT("comparison mode"), comparison_mode, 'c');
  
  dialog::addSelItem(XLAT("exponential range"), fts(exp_decay_current()), 'r');
  dialog::add_action([&] {
    dialog::editNumber(exp_decay_current(), 0, 40, 0, 5, XLAT("exponential range"), 
      XLAT("brightness formula: max(1-d/sightrange, s*exp(-d/r))")
      );
    });

  dialog::addSelItem(XLAT("exponential start"), fts(exp_start), 's');
  dialog::add_action([&] {
    dialog::editNumber(exp_start, 0, 1, 0.1, 1, XLAT("exponential start"), 
      XLAT("brightness formula: max(1-d/sightrange, s*exp(-d/r))\n")
      );
    });

  if(nonisotropic) {
    dialog::addSelItem(XLAT("max step"), fts(maxstep_current()), 'x');
    dialog::add_action([] {
      dialog::editNumber(maxstep_current(), 1e-6, 1, 0.1, sol ? 0.03 : 0.1, XLAT("max step"), "");
      dialog::scaleLog();
      dialog::bound_low(1e-9);
      dialog::reaction = reset_raycaster;
      });

    dialog::addSelItem(XLAT("min step"), fts(minstep), 'n');
    dialog::add_action([] {
      dialog::editNumber(minstep, 1e-6, 1, 0.1, 0.001, XLAT("min step"), "");
      dialog::scaleLog();
      dialog::bound_low(1e-9);
      dialog::reaction = reset_raycaster;
      });
    }
  
  dialog::addSelItem(XLAT("iterations"), its(max_iter_current()), 's');
  dialog::add_action([&] {
    dialog::editNumber(max_iter_current(), 0, 600, 1, 60, XLAT("iterations"), "");
    dialog::reaction = reset_raycaster;
    });

  dialog::addSelItem(XLAT("max cells"), its(max_cells), 's');
  dialog::add_action([&] {
    dialog::editNumber(max_cells, 16, 131072, 0.1, 4096, XLAT("max cells"), "");
    dialog::scaleLog();
    dialog::extra_options = [] {
      dialog::addBoolItem_action("generate", rays_generate, 'G');
      dialog::addColorItem(XLAT("out-of-range color"), color_out_of_range, 'X');
      dialog::add_action([] { 
        dialog::openColorDialog(color_out_of_range); 
        dialog::dialogflags |= sm::SIDE;
        });
      };
    });
  
  dialog::addBack();
  dialog::display();
  }

#if CAP_COMMANDLINE  
int readArgs() {
  using namespace arg;
           
  if(0) ;
  else if(argis("-ray-do")) {
    PHASEFROM(2);
    want_use = 2;
    }
  else if(argis("-ray-dont")) {
    PHASEFROM(2);
    want_use = 0;
    }
  else if(argis("-ray-smart")) {
    PHASEFROM(2);
    want_use = 1;
    }
  else if(argis("-ray-cells")) {
    PHASEFROM(2); shift();
    rays_generate = true;
    max_cells = argi();
    }
  else if(argis("-ray-cells-no")) {
    PHASEFROM(2); shift();
    rays_generate = false;
    max_cells = argi();
    }
  else return 1;
  return 0;
  }

auto hook = addHook(hooks_args, 100, readArgs);
#endif

EX }
}