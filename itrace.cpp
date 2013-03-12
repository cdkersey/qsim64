#include <iostream>
#include <iomanip>

#include <qsim.h>

#include "itrace.h"

using namespace Qsim;
using namespace std;

struct itracer {
  itracer(OSDomain &osd, ostream &out = cout);
  ~itracer();

  void inst_cb
    (int c, uint64_t p, uint64_t v, uint8_t s, const uint8_t *b, inst_type t);

  OSDomain &osd;
  ostream &out;
  OSDomain::inst_cb_handle_t icbh;
};

itracer *itp;
void attach_itrace(OSDomain &osd) {
  itp = new itracer(osd);
}

void detach_itrace() {
  delete itp;
}

itracer::itracer(OSDomain &osd, ostream &out): osd(osd), out(out) {
  icbh = osd.set_inst_cb(this, &itracer::inst_cb);
}

itracer::~itracer() {
  osd.unset_inst_cb(icbh);
}

void itracer::inst_cb
  (int c, uint64_t p, uint64_t v, uint8_t s, const uint8_t *b, inst_type t)
{
  uint64_t rax(osd.get_reg(c, QSIM_RAX)), rcx(osd.get_reg(c, QSIM_RCX)),
           rbx(osd.get_reg(c, QSIM_RBX)), rdx(osd.get_reg(c, QSIM_RDX)),
           rflags(osd.get_reg(c, QSIM_RFLAGS));

  //out << hex << "0x" << v << '(' << hex << rax << ',' << rcx
  //    << ',' << rbx << ',' << rdx << ',' << rflags << ')' << endl;
}
