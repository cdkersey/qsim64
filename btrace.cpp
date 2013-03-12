#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <string>

#include <signal.h>

#include <qsim.h>

#include "btrace.h"

using namespace Qsim;
using namespace std;

struct tracer {
  tracer(OSDomain &osd, string smfile);
  ~tracer();

  void dumptrace(ostream &out);

  void inst_cb
    (int cpu, uint64_t va, uint64_t pa, uint8_t sz, const uint8_t *buf,
     inst_type t);

  OSDomain &osd;

  map<uint64_t, string> sysmap;
  vector<vector<uint64_t> > cstack;
  vector<bool> call;
};

tracer *tp;
void attach_btrace(OSDomain &osd) {
  tp = new tracer(osd, "System.map");
}

void print_btrace(ostream &out) {
  tp->dumptrace(out);
}

tracer::tracer(OSDomain &osd, string smfile):
  osd(osd), cstack(osd.get_n()), call(osd.get_n())
{
  osd.set_inst_cb(this, &tracer::inst_cb);

  ifstream smap(smfile.c_str());
  for (;;) {
    uint64_t addr;
    string sym, t;
    smap >> hex >> addr >> t >> sym;
    if (!smap) break;
    sysmap[addr] = sym;
    //cerr << "Added 0x" << hex << addr << ": " << sym << endl;
  }
}

tracer::~tracer() { dumptrace(cerr); }

void tracer::dumptrace(ostream &out) {
  for (unsigned i = 0; i < osd.get_n(); ++i) {
    if (!cstack[i].empty()) {
      out << "CPU " << i << ':' << endl;
      for (unsigned j = 0; j < cstack[i].size(); ++j) {
        for (unsigned k = 0; k <= cstack[i].size(); ++k) out << "  ";
        out << hex << cstack[i][j] << ": " << sysmap[cstack[i][j]] << endl;
      }
    }
  }
}

void tracer::inst_cb
  (int i, uint64_t va, uint64_t pa, uint8_t sz, const uint8_t *buf, inst_type t)
{
  if (call[i]) {
    call[i] = false;
    cstack[i].push_back(va);
  }

  if (t == QSIM_INST_CALL)
    call[i] = true;
  else if (t == QSIM_INST_RET && !cstack[i].empty())
    cstack[i].pop_back();
}
