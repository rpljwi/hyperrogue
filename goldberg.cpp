namespace hr { namespace gp {
  bool on;
  loc param(1, 0);

  hyperpoint next;
  ld scale;
  ld alpha;
  int area;
  
  int length(loc p) {
    return eudist(p.first, p.second);
    }
  
  loc operator+(loc e1, loc e2) {
    return make_pair(e1.first+e2.first, e1.second+e2.second);
    }
  
  loc operator-(loc e1, loc e2) {
    return make_pair(e1.first-e2.first, e1.second-e2.second);
    }
  
  loc operator*(loc e1, loc e2) {
    return make_pair(e1.first*e2.first-e1.second*e2.second, 
      e1.first*e2.second + e2.first*e1.second + (S3 == 3 ? e1.second*e2.second : 0));
    }

  loc operator*(loc e1, int i) {
    return loc(e1.first*i, e1.second*i);
    }
  
  struct goldberg_mapping_t {
    cellwalker cw;
    signed char rdir;
    signed char mindir;
    loc start;
    };

  loc eudir(int d) {
    if(S3 == 3) {
      d %= 6; if (d < 0) d += 6;
      switch(d) {
        case 0: return make_pair(1, 0);
        case 1: return make_pair(0, 1);
        case 2: return make_pair(-1, 1);
        case 3: return make_pair(-1, 0);
        case 4: return make_pair(0, -1);
        case 5: return make_pair(1, -1);
        default: return make_pair(0, 0);
        }
      }
    else switch(d&3) {
      case 0: return make_pair(1, 0);
      case 1: return make_pair(0, 1);
      case 2: return make_pair(-1, 0);
      case 3: return make_pair(0, -1);
      default: return make_pair(0, 0);
      }
    }
  
#define SG6 (S3==3?6:4)
#define SG3 (S3==3?3:2)
#define SG2 (S3==3?2:1)

  int fixg6(int x) { return (x + MODFIXER) % SG6; }
  
#define WHD(x) // x

  int get_code(const local_info& li) {
    return 
      ((li.relative.first & 15) << 0) +
      ((li.relative.second & 15) << 4) +
      ((fixg6(li.total_dir)) << 8) +
      ((li.last_dir & 15) << 12);
    }
  
  local_info get_local_info(cell *c) {
    local_info li;
    if(c == c->master->c7) {
      li.relative = loc(0,0);
      li.first_dir = -1;
      li.last_dir = -1;
      li.total_dir = -1;
      }
    else {
      vector<int> dirs;
      while(c != c->master->c7) {
        dirs.push_back(c->c.spin(0));
        c = c->move(0);
        }
      li.first_dir = dirs[0];
      li.last_dir = dirs.back();

      loc at(0,0);
      int dir = 0;
      at = at + eudir(dir);
      dirs.pop_back();
      while(dirs.size()) {
        dir += dirs.back() + SG3;
        dirs.pop_back();
        at = at + eudir(dir);
        }
      li.relative = at;
      li.total_dir = dir + SG3;
      }
    return li;
    }
    
  int last_dir(cell *c) {
    return get_local_info(c).last_dir;
    }

  loc get_coord(cell *c) {
    return get_local_info(c).relative;
    }
  
  int pseudohept_val(cell *c) {
    loc v = get_coord(c);
    return (v.first - v.second + MODFIXER)%3;
    }
  
  // mapping of the local equilateral triangle
  // goldberg_map[y][x].cw is the cellwalker in this triangle at position (x,y)
  // facing local direction 0  
  
  goldberg_mapping_t goldberg_map[32][32];
  void clear_mapping() {
    for(int y=0; y<32; y++) for(int x=0; x<32; x++) {
      goldberg_map[y][x].cw.at = NULL;
      goldberg_map[y][x].rdir = -1;
      goldberg_map[y][x].mindir = 0;
      }
    }
  
  goldberg_mapping_t& get_mapping(loc c) {
    return goldberg_map[c.second&31][c.first&31];
    }

  const char *disp(loc at) { 
    static char bufs[16][16];
    static int bufid;
    bufid++; bufid %= 16;
    snprintf(bufs[bufid], 16, "[%2d,%2d]", at.first, at.second);
    return bufs[bufid];
    }

  const char *dcw(cellwalker cw) { 
    static char bufs[16][32];
    static int bufid;
    bufid++; bufid %= 16;
    snprintf(bufs[bufid], 32, "[%p/%2d:%d:%d]", cw.at, cw.at?cw.at->type:-1, cw.spin, cw.mirrored);
    return bufs[bufid];
    }

  int spawn;

  cell*& peek(cellwalker cw) {
    return cw.at->move(cw.spin);
    }

  cellwalker get_localwalk(const goldberg_mapping_t& wc, int dir) {
    if(dir < wc.mindir) dir += SG6;
    if(dir >= wc.mindir + SG6) dir -= SG6;
    return wc.cw + dir;
    }

  void set_localwalk(goldberg_mapping_t& wc, int dir, const cellwalker& cw) {
    if(dir < wc.mindir) dir += SG6;
    if(dir >= wc.mindir + SG6) dir -= SG6;
    wc.cw = cw - dir;
    }

  bool pull(loc at, int dir) {
    auto& wc = get_mapping(at);
    auto at1 = at + eudir(dir);
    int dir1 = fixg6(dir+SG3);
    cellwalker wcw = get_localwalk(wc, dir);
    auto& wc1= get_mapping(at1);
    if(wc1.cw.at) {
      if(peek(wcw)) {
        auto wcw1 = get_localwalk(wc1, dir1);
        if(wcw + wstep != wcw1) {
          WHD( Xprintf("%s : %s / %s (pull error from %s :: %s)\n", disp(at1), dcw(wcw+wstep), dcw(wcw1), disp(at), dcw(wcw)); )
          exit(1);
          }
        }
      return false;
      }
    if(peek(wcw)) {
      set_localwalk(wc1, dir1, wcw + wstep);
      WHD( Xprintf("%s : %s (pulled from %s :: %s)\n", disp(at1), dcw(wcw + wstep), disp(at), dcw(wcw)); )
      return true;
      }
    return false;
    }

  void conn1(loc at, int dir, int dir1) {
    auto& wc = get_mapping(at);
    auto wcw = get_localwalk(wc, dir);
    auto& wc1 = get_mapping(at + eudir(dir));
    WHD( Xprintf("  md:%02d s:%d", wc.mindir, wc.cw.spin); )
    WHD( Xprintf("  connection %s/%d %s=%s ~ %s/%d ", disp(at), dir, dcw(wc.cw+dir), dcw(wcw), disp(at+eudir(dir)), dir1); )
    if(!wc1.cw.at) {
      wc1.start = wc.start;
      if(peek(wcw)) {
        WHD( Xprintf("(pulled) "); )
        set_localwalk(wc1, dir1, wcw + wstep);
        }
      else {
        peek(wcw) = newCell(SG6, wc.cw.at->master);
        wcw.at->c.setspin(wcw.spin, 0, false);
        set_localwalk(wc1, dir1, wcw + wstep);
        spawn++;
        WHD( Xprintf("(created) "); )
        }
      }
    WHD( Xprintf("%s ", dcw(wc1.cw+dir1)); )
    auto wcw1 = get_localwalk(wc1, dir1);
    if(peek(wcw)) {
      if(wcw+wstep != wcw1) {
        WHD( Xprintf("FAIL: %s / %s\n", dcw(wcw), dcw(wcw1)); exit(1); )
        }
      else {
        WHD(Xprintf("(was there)\n");)
        }
      }
    else {
      WHD(Xprintf("ok\n"); )
      peek(wcw) = wcw1.at;
      wcw.at->c.setspin(wcw.spin, wcw1.spin, wcw.mirrored != wcw1.mirrored);
      if(wcw+wstep != wcw1) {
        Xprintf("assertion failed\n");
        exit(1);
        }
      }
    }

  void conn(loc at, int dir) {
    conn1(at, fixg6(dir), fixg6(dir+SG3));
    conn1(at + eudir(dir), fixg6(dir+SG3), fixg6(dir));
    }
  
  goldberg_mapping_t& set_heptspin(loc at, heptspin hs) {
    auto& ac0 = get_mapping(at);
    ac0.cw = cellwalker(hs.at->c7, hs.spin, hs.mirrored);
    ac0.start = at;
    WHD( Xprintf("%s : %s\n", disp(at), dcw(ac0.cw)); )
    return ac0;
    }

  void extend_map(cell *c, int d) {
    WHD( Xprintf("EXTEND %p %d\n", c, d); )
    if(c->master->c7 != c) {
      while(c->master->c7 != c) {
        WHD( Xprintf("%p direction 0 corresponds to %p direction %d\n", c, c->move(0), c->c.spin(0)); )
        d = c->c.spin(0);
        c = c->move(0);
        }
      // c move 0 equals c' move spin(0)
      extend_map(c, d);
      extend_map(c, fixdir(d-1, c));
      extend_map(c, fixdir(d+1, c));
      if(S3 == 4 && !c->move(d))
        for(int i=0; i<S7; i++)
        for(int j=0; j<S7; j++)
          extend_map(createStep(c->master, i)->c7, j);
      return;
      }

    if(S3 == 4 && param.first <= param.second) { d--; if(d<0) d += S7; }
    clear_mapping();

    // we generate a local map from an Euclidean grid to the
    // hyperbolic grid we build.
    
    // we fill the equilateral triangle with the following vertices:

    loc vc[4];
    vc[0] = loc(0,0);
    vc[1] = param;
    if(S3 == 3)
      vc[2] = param * loc(0,1);
    else 
      vc[2] = param * loc(1,1),
      vc[3] = param * loc(0,1);
    
    heptspin hs(c->master, d, false);
    
    auto& ac0 = set_heptspin(vc[0], hs);
    ac0.mindir = -1;
    auto& ac1 = set_heptspin(vc[1], hs + wstep - SG3);
    ac1.mindir = 0;
    auto& ac2 = set_heptspin(vc[S3-1], S3 == 3 ? hs + 1 + wstep - 4 : hs + 1 + wstep + 1);
    ac2.mindir = S3 == 3 ? 1 : -2;
    if(S3 == 4) {
      set_heptspin(vc[2], hs + wstep - 1 + wstep + 1).mindir = -3;
      }
    
    if(S3 == 4 && param == loc(1,1)) {
      conn(loc(0,0), 1);
      conn(loc(0,1), 0);
      conn(loc(0,1), 1);
      conn(loc(0,1), 2);
      conn(loc(0,1), 3);
      return;
      }
    
    if(nonorientable && param.first == param.second) {
      int x = param.first;
      if(ac1.cw.mirrored != hs.mirrored) ac1.cw--;
      if(ac2.cw.mirrored != hs.mirrored) ac2.cw--;
      
      for(int d=0; d<3; d++) for(int k=0; k<3; k++) 
      for(int i=0; i<x; i++) {
        int dd = (2*d+k);
        loc cx = vc[d] + eudir(dd) * i;
        if(!pull(cx, dd)) break;
        }
      
      for(int i=0; i<=2*x; i++)
      for(int d=0; d<3; d++) {
        loc cx = vc[d] + eudir(1+2*d) * i;
        if(i < 2*x) conn(cx, 1+2*d);
        int jmax = x-i, drev = 2*d;
        if(jmax < 0) drev += 3, jmax = -jmax;
        for(int j=0; j<jmax; j++) {
          loc cy = cx + eudir(drev) * j;
          conn(cy, drev);
          conn(cy, drev+1);
          conn(cy, drev+2);
          }
        }
      
      return;
      }

    // then we set the edges of our big equilateral triangle (in a symmetric way)
    for(int i=0; i<S3; i++) {
      loc start = vc[i];
      loc end = vc[(i+1)%S3];
      WHD( Xprintf("from %s to %s\n", disp(start), disp(end)); )
      loc rel = param;
      auto build = [&] (loc& at, int dx, bool forward) {
        int dx1 = dx + SG2*i;
        WHD( Xprintf("%s %d .. %s %d\n", disp(at), dx1, disp(at + eudir(dx1)), fixg6(dx1+SG3)); )
        conn(at, dx1);        
        if(forward) get_mapping(at).rdir = fixg6(dx1);
        else get_mapping(at+eudir(dx1)).rdir = fixg6(dx1+SG3);
        at = at + eudir(dx1);
        };
      while(rel.first >= 2 && (S3 == 3 ? rel.first >= 2 - rel.second : true)) {
        build(start, 0, true);
        build(end, SG3, false);
        rel.first -= 2;
        }
      while(rel.second >= 2) {
        build(start, 1, true);
        build(end, 1+SG3, false);
        rel.second -= 2;
        }
      while(rel.second <= -2 && S3 == 3) {
        build(start, 5, true);
        build(end, 2, false);
        rel.second += 2;
        rel.first -= 2;
        }
      if(S3 == 3) while((rel.first>0 && rel.second > 0) | (rel.first > 1 && rel.second < 0)) {
        build(start, 0, true);
        build(end, 3, false);
        rel.first -= 2;
        }
      if(S3 == 4 && rel == loc(1,1)) {
        if(param == loc(3,1) || param == loc(5,1)) {
          build(start, 1, true);
          build(end, 2, false);
          rel.first--;
          rel.second--;
          }
        else {
          build(start, 0, true);
          build(end, 3, false);
          rel.first--;
          rel.second--;
          }
        }
      for(int k=0; k<SG6; k++)
        if(start + eudir(k+SG2*i) == end)
          build(start, k, true);                         
      if(start != end) { Xprintf("assertion failed: start %s == end %s\n", disp(start), disp(end)); exit(1); }
      }

    // now we can fill the interior of our big equilateral triangle
    loc at = vc[0];
    int maxstep = 3000;
    while(true) {
      maxstep--; if(maxstep < 0) { printf("maxstep exceeded\n"); exit(1); }
      auto& wc = get_mapping(at);
      int dx = wc.rdir;
      auto at1 = at + eudir(dx);
      auto& wc1 = get_mapping(at1);
      WHD( Xprintf("%s (%d) %s (%d)\n", disp(at), dx, disp(at1), wc1.rdir); )
      int df = wc1.rdir - dx;
      if(df < 0) df += SG6;
      if(df == SG3) break;
      if(S3 == 3) switch(df) {
        case 0:
        case 4:
        case 5:
          at = at1;
          continue;
        case 2: {
          conn(at, dx+1);
          wc.rdir = (dx+1) % 6;
          break;
          }
        case 1: {
          auto at2 = at + eudir(dx+1);
          auto& wc2 = get_mapping(at2);
          if(wc2.cw.at) { at = at1; continue; }
          wc.rdir = (dx+1) % 6;
          conn(at, (dx+1) % 6);
          conn(at1, (dx+2) % 6);
          conn(at2, (dx+0) % 6);
          wc1.rdir = -1;
          wc2.rdir = dx; 
          break;
          }
        default:
          Xprintf("case unhandled %d\n", df);
          exit(1);
        }
      else switch(df) {
        case 0: 
        case 3:
          at = at1;
          continue;
        case 1:
          auto at2 = at + eudir(dx+1);
          auto& wc2 = get_mapping(at2);
          if(wc2.cw.at) {
            auto at3 = at1 + eudir(wc1.rdir);
            auto& wc3 = get_mapping(at3);
            auto at4 = at3 + eudir(wc3.rdir);
            if(at4 == at2) {
              wc.rdir = (dx+1)%4;
              wc1.rdir = -1;
              wc3.rdir = -1;
              conn(at, (dx+1)%4);
              }
            else { 
              at = at1;
              }
            }
          else {
            wc.rdir = (dx+1)%4;
            wc1.rdir = -1;
            wc2.rdir = dx%4;
            int bdir = -1;
            int bdist = 100;
            for(int d=0; d<4; d++) {
              auto &wcm = get_mapping(at2 + eudir(d));
              if(wcm.cw.at && length(wcm.start - at2) < bdist)
                bdist = length(wcm.start - at2), bdir = d;
              }
            if(bdir != -1) conn(at2 + eudir(bdir), bdir ^ 2);
            conn(at, (dx+1)%4);
            conn(at2, dx%4);
            
            at = param * loc(1,0) + at * loc(0, 1);
            }
          break;
        }
      }

    WHD( Xprintf("DONE\n\n"); )    
    }
  
  hyperpoint loctoh_ort(loc at) {
    return hpxyz(at.first, at.second, 1);
    }

  hyperpoint corner_coords6[7] = {
    hpxyz(2, -1, 0),
    hpxyz(1, 1, 0),
    hpxyz(-1, 2, 0),
    hpxyz(-2, 1, 0),
    hpxyz(-1, -1, 0),
    hpxyz(1, -2, 0),
    hpxyz(0, 0, 0) // center, not a corner
    };

  hyperpoint corner_coords4[7] = {
    hpxyz(1.5, -1.5, 0),
//    hpxyz(1, 0, 0),
    hpxyz(1.5, 1.5, 0),
//    hpxyz(0, 1, 0),
    hpxyz(-1.5, 1.5, 0),
//    hpxyz(-1, 0, 0),
    hpxyz(-1.5, -1.5, 0),
//    hpxyz(0, -1, 0),
    hpxyz(0, 0, 0),
    hpxyz(0, 0, 0),
    hpxyz(0, 0, 0)
    };

  #define corner_coords (S3==3 ? corner_coords6 : corner_coords4)
  
  hyperpoint cornmul(const transmatrix& corners, const hyperpoint& c) {
    if(sphere) {
      ld cmin = c[0] * c[1] * c[2] * (6 - S7);
      return corners * hpxyz(c[0] + cmin, c[1] + cmin, c[2] + cmin);
      }
    else return corners * c;
    }
    
  hyperpoint atz(const transmatrix& T, const transmatrix& corners, loc at, int cornerid = 6, ld cf = 3) {
    int sp = 0;
    again:
    auto corner = corners * hyperpoint_vec::operator+ (loctoh_ort(at), hyperpoint_vec::operator/ (corner_coords[cornerid], cf));
    if(corner[1] < -1e-6 || corner[2] < -1e-6) {
      at = at * eudir(1);
      if(cornerid < SG6) cornerid = (1 + cornerid) % SG6;
      sp++;
      goto again;
      }
    if(sp>SG3) sp -= SG6;

    return normalize(spin(2*M_PI*sp/S7) * cornmul(T, corner));
    }
  
  transmatrix Tf[8][32][32][6];
  
  transmatrix corners;
  
  transmatrix dir_matrix(int i) {
    cell cc; cc.type = S7;
    return spin(-alpha) * build_matrix(
      C0, 
      ddspin(&cc, i) * xpush0(tessf),
      ddspin(&cc, i+1) * xpush0(tessf)
      );
    }
  
  void prepare_matrices() {
    corners = inverse(build_matrix(
      loctoh_ort(loc(0,0)),
      loctoh_ort(param),
      loctoh_ort(param * loc(0,1))
      ));
    for(int i=0; i<S7; i++) {
      transmatrix T = dir_matrix(i);
      for(int x=-16; x<16; x++)
      for(int y=-16; y<16; y++)
      for(int d=0; d<(S3==3?6:4); d++) {
        loc at = loc(x, y);
        
        hyperpoint h = atz(T, corners, at, 6);
        hyperpoint hl = atz(T, corners, at + eudir(d), 6);
        Tf[i][x&31][y&31][d] = rgpushxto0(h) * rspintox(gpushxto0(h) * hl) * spin(M_PI);
        }
      }       
    }

  hyperpoint get_corner_position(const local_info& li, int cid, ld cf = 3) {
    int i = li.last_dir;
    if(i == -1) 
      return atz(dir_matrix(cid), corners, li.relative, 0, cf);
    else {
      auto& cellmatrix = Tf[i][li.relative.first&31][li.relative.second&31][fixg6(li.total_dir)];
      return inverse(cellmatrix) * atz(dir_matrix(i), corners, li.relative, fixg6(cid + li.total_dir), cf);
      }
    }
  
  hyperpoint get_corner_position(cell *c, int cid, ld cf = 3) {
    return get_corner_position(get_local_info(c), cid, cf);
    }
    
  map<pair<int, int>, loc> center_locs;
  
  void compute_geometry() {
    center_locs.clear();
    if(on) {
      int x = param.first;
      int y = param.second;
      area = ((2*x+y) * (2*x+y) + y*y*3) / 4;
      next = hpxyz(x+y/2., -y * sqrt(3) / 2, 0);
      scale = 1 / hypot2(next);
      crossf *= scale;
      hepvdist *= scale;
      rhexf *= scale;
//    spin = spintox(next);
//    ispin = rspintox(next);
      alpha = -atan2(next[1], next[0]) * 6 / S7;
      if(S3 == 3)
        base_distlimit = (base_distlimit + log(scale) / log(2.618)) / scale;
      else
        base_distlimit = 3 * max(param.first, param.second) + 2 * min(param.first, param.second);
      if(base_distlimit > 30)
        base_distlimit = 30;
      prepare_matrices();
      if(debug_geometry)
        Xprintf("scale = " LDF "\n", scale);
      }
    else {
      scale = 1;
      alpha = 0;
      }
    }

  loc config;
  
  loc internal_representation(loc v) {
    int& x = v.first, &y = v.second;
    while(x < 0 || y < 0 || (x == 0 && y > 0))
      v = v * loc(0, 1);
    if(x > 8) x = 8;
    if(y > 8) y = 8;
    if(S3 == 3 && y > x) v = v * loc(1, -1);
    return v;
    }
  
  loc human_representation(loc v) {
    int& x = v.first, &y = v.second;
    if(S3 == 3) while(x < 0 || y < 0 || (x == 0 && y > 0))
      v = v * loc(0, 1);
    return v;
    }
  
  string operation_name() {
    if(!gp::on) {
      if(irr::on) return XLAT("irregular");
      else if(nonbitrunc) return XLAT("OFF");
      else return XLAT("bitruncated");
      }
    else if(param == loc(1, 0))
      return XLAT("OFF");
    else if(param == loc(1, 1) && S3 == 3)
      return XLAT("bitruncated");
    else if(param == loc(1, 1) && S3 == 4)
      return XLAT("rectified");
    else if(param == loc(2, 0))
      return S3 == 3 ? XLAT("chamfered") : XLAT("expanded");
    else if(param == loc(3, 0) && S3 == 3)
      return XLAT("2x bitruncated");
    else {
      auto p = human_representation(param);
      return "GP(" + its(p.first) + "," + its(p.second) + ")";
      }
    }
  
  void whirl_set(loc xy, bool texture_remap) {
#if CAP_TEXTURE
    auto old_tstate = texture::config.tstate;
    auto old_tstate_max = texture::config.tstate_max;
#endif
    xy = internal_representation(xy);
    if(xy.second && xy.second != xy.first && nonorientable) {
      addMessage(XLAT("This does not work in non-orientable geometries"));
      xy.second = 0;
      }
    config = human_representation(xy);
    auto g = screens;
    if(xy.first == 0 && xy.second == 0) xy.first = 1;
    if(xy.first == 1 && xy.second == 0) {
      if(irr::on) stop_game_and_switch_mode(rg::bitrunc);
      if(gp::on) stop_game_and_switch_mode(rg::bitrunc);
      if(!nonbitrunc) stop_game_and_switch_mode(rg::bitrunc);
      }
    else if(xy.first == 1 && xy.second == 1 && S3 == 3) {
      if(gp::on) stop_game_and_switch_mode(rg::bitrunc);
      if(nonbitrunc) stop_game_and_switch_mode(rg::bitrunc);
      }
    else {
      if(nonbitrunc) stop_game_and_switch_mode(rg::bitrunc);
      param = xy;
      stop_game_and_switch_mode(rg::gp);
      }
    start_game();
#if CAP_TEXTURE
    if(texture_remap)
      texture::config.remap(old_tstate, old_tstate_max);
#endif
    screens = g;
    }

  string helptext() {
    return XLAT(
      "Goldberg polyhedra are obtained by adding extra hexagons to a dodecahedron. "
      "GP(x,y) means that, to get to a nearest non-hex from any non-hex, you should move x "
      "cells in any direction, turn right 60 degrees, and move y cells. "
      "HyperRogue generalizes this to any tesselation with 3 faces per vertex. "
      "By default HyperRogue uses bitruncation, which corresponds to GP(1,1)."
      );
    }  

  void show(bool texture_remap) {
    cmode = sm::SIDE;
    gamescreen(0);  
    dialog::init(XLAT("variations"));
    
    bool show_nonthree = !(texture_remap && (S7&1));
    bool show_bitrunc  = !(texture_remap && !(S7&1));
    bool show_irregular = true;
    if(texture_remap) {
      if(patterns::cgroup == cpSingle)
        show_nonthree = true, show_bitrunc = false, show_irregular = true;
      }
    
    if(show_nonthree) {
      dialog::addBoolItem(XLAT("OFF"), param == loc(1,0) && !irr::on, 'a');
      dialog::lastItem().value = "GP(1,0)";
      }

    if(show_bitrunc) {
      dialog::addBoolItem(XLAT("bitruncated"), param == loc(1,1) && !nonbitrunc, 'b');  
      dialog::lastItem().value = S3 == 3 ? "GP(1,1)" : XLAT(nonbitrunc ? "OFF" : "ON");
      }

    if(show_nonthree) {
      dialog::addBoolItem(XLAT(S3 == 3 ? "chamfered" : "expanded"), param == loc(2,0), 'c');
      dialog::lastItem().value = "GP(2,0)";
      }

    if(S3 == 3) {
      dialog::addBoolItem(XLAT("2x bitruncated"), param == loc(3,0), 'd');
      dialog::lastItem().value = "GP(3,0)";
      }
    else {
      dialog::addBoolItem(XLAT("rectified"), param == loc(1,1) && nonbitrunc, 'd');
      dialog::lastItem().value = "GP(1,1)";
      }

    dialog::addBreak(100);
    dialog::addSelItem("x", its(config.first), 'x');
    dialog::addSelItem("y", its(config.second), 'y');
    
    if(config.second && config.second != config.first && nonorientable) {
      dialog::addInfo(XLAT("This does not work in non-orientable geometries"));
      }
    else if((config.first-config.second)%3 && !show_nonthree)
      dialog::addInfo(XLAT("This pattern needs x-y divisible by 3"));
    else if(config == loc(1,1) && !show_bitrunc)
      dialog::addInfo(XLAT("Select bitruncated from the previous menu"));
    else    
      dialog::addBoolItem(XLAT("select"), param == internal_representation(config) && !irr::on, 'f');
      
    if(show_irregular && irr::supports(geometry)) {
      dialog::addBoolItem(XLAT("irregular"), irr::on, 'i');
      dialog::add_action([=] () { 
        if(texture_remap && !show_nonthree && !irr::bitruncations_requested) irr::bitruncations_requested++;
        if(!irr::on) irr::visual_creator(); 
        });
      }
    
    dialog::addBreak(100);
    dialog::addHelp();
    dialog::addBack();
    dialog::display();

    keyhandler = [show_nonthree, show_bitrunc, texture_remap] (int sym, int uni) {
      dialog::handleNavigation(sym, uni);
      if(uni == 'a' && show_nonthree) 
        whirl_set(loc(1, 0), texture_remap);
      else if(uni == 'b' && show_bitrunc) {
        if(S3 == 4) {
          if(nonbitrunc || gp::on)
            restart_game(rg::bitrunc);
          }
        else 
          whirl_set(loc(1, 1), texture_remap);
        }
      else if(uni == 'c' && show_nonthree)
        whirl_set(loc(2, 0), texture_remap);
      else if(uni == 'd')
        whirl_set(S3 == 3 ? loc(3, 0) : loc(1,1), texture_remap);
      else if(uni == 'f' && (config == loc(1,1) ? show_bitrunc : (show_nonthree || (config.first-config.second)%3 == 0)))
        whirl_set(config, texture_remap);
      else if(uni == 'x')
        dialog::editNumber(config.first, 0, 8, 1, 1, "x", helptext());
      else if(uni == 'y')
        dialog::editNumber(config.second, 0, 8, 1, 1, "y", helptext());
      else if(uni == 'z')
        swap(config.first, config.second);
      else if(uni == '?' || sym == SDLK_F1 || uni == 'h' || uni == '2')
        gotoHelp(helptext());
      else if(doexiton(sym, uni))
        popScreen();
      };
    }
  
  loc univ_param() {
    if(on) return param;
    else if(nonbitrunc) return loc(1,0);
    else return loc(1,1);
    }
  
  void configure(bool texture_remap = false) {
    auto l = univ_param();
    param = l;
    config = human_representation(l);
    pushScreen([texture_remap] () { gp::show(texture_remap); });
    }
  
  void be_in_triangle(local_info& li) {
    int sp = 0;
    auto& at = li.relative;
    again:
    auto corner = corners * loctoh_ort(at);
    if(corner[1] < -1e-6 || corner[2] < -1e-6) {
      at = at * eudir(1);
      sp++;
      goto again;
      }
    if(sp>SG3) sp -= SG6;
    li.last_dir = fix7(li.last_dir - sp);
    }

  // from some point X, (0,0) is in distance dmain, param is in distance d0, and param*z is in distance d1
  // what is the distance of at from X?

  int solve_triangle(int dmain, int d0, int d1, loc at) {
    loc centerloc(0, 0);
    auto rel = make_pair(d0-dmain, d1-dmain);
    if(center_locs.count(rel))
      centerloc = center_locs[rel];
    else {
      bool found = false;
      for(int y=-20; y<=20; y++)
      for(int x=-20; x<=20; x++) {
        loc c(x, y);
        int cc = length(c);
        int c0 = length(c - param);
        int c1 = length(c - param*loc(0,1));
        if(c0-cc == d0-dmain && c1-cc == d1-dmain)
          found = true, centerloc = c;
        }
      if(!found && !quotient) {
        Xprintf("Warning: centerloc not found: %d,%d,%d\n", dmain, d0, d1);
        }
      center_locs[rel] = centerloc;
      }
    
    return dmain + length(centerloc-at) - length(centerloc);
    }

  int solve_quad(int dmain, int d0, int d1, int dx, loc at) {
    loc centerloc(0, 0);
    auto rel = make_pair(d0-dmain, (d1-dmain) + 1000 * (dx-dmain) + 1000000);
    if(center_locs.count(rel))
      centerloc = center_locs[rel];
    else {
      bool found = false;
      for(int y=-20; y<=20; y++)
      for(int x=-20; x<=20; x++) {
        loc c(x, y);
        int cc = length(c);
        int c0 = length(c - param);
        int c1 = length(c - param*loc(0,1));
        int c2 = length(c - param*loc(1,1));
        if(c0-cc == d0-dmain && c1-cc == d1-dmain && c2-cc == dx-dmain)
          found = true, centerloc = c;
        }
      if(!found && !quotient) {
        Xprintf("Warning: centerloc not found: %d,%d,%d,%d\n", dmain, d0, d1, dx);
        }
      center_locs[rel] = centerloc;
      }
    
    return dmain + length(centerloc-at) - length(centerloc);
    }
  
  array<heptagon*, 3> get_masters(cell *c) {
    if(gp::on) {
      auto li = get_local_info(c);
      be_in_triangle(li);      
      auto cm = c->master;
      int i = li.last_dir;
      return make_array(cm, createStep(cm, i), createStep(cm, fix7(i+1)));
      }
    else if(irr::on)
      return irr::get_masters(c);
    else
      return make_array(c->move(0)->master, c->move(2)->master, c->move(4)->master);
    }
  
  int compute_dist(cell *c, int master_function(cell*)) {
    auto li = get_local_info(c);
    be_in_triangle(li);
    
    cell *cm = c->master->c7;
    
    int i = li.last_dir;
    auto at = li.relative;

    auto dmain = master_function(cm);
    auto d0 = master_function(createStep(cm->master, i)->c7);
    auto d1 = master_function(createStep(cm->master, fixdir(i+1, cm))->c7);
    
    if(S3 == 4) {
      heptspin hs(cm->master, i);
      hs += wstep; hs+=-1; hs += wstep;
      auto d2 = master_function(hs.at->c7);
      return solve_quad(dmain, d0, d1, d2, at);
      }
    
    return solve_triangle(dmain, d0, d1, at);
    }
  
  int dist_2() {
    return length(univ_param());
    }

  int dist_3() {
    return length(univ_param() * loc(1,1));
    }
  
  int dist_1() {
    return dist_3() - dist_2();
    }

  }}
