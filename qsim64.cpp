#include <iostream>
#include <string>

#include <signal.h>

#include <qsim.h>
#include <qsim-prof.h>

#include "io.h"
#include "mmio.h"
#include "bios.h"
#include "btrace.h"
#include "itrace.h"

const unsigned CPUS = 4, QUANT = 1000;

using namespace std;
using namespace Qsim;

void btr_sigint_handler(int mask) {
  cerr << "SIGINT caught." << endl;
  print_btrace(cerr);
  exit(0);
}

void set_signal_handler(int signal, void (*func)(int)) {
  struct sigaction na, oa;

  na.sa_handler = func;
  na.sa_flags = 0;
  sigemptyset(&na.sa_mask);

  sigaction(signal, NULL, &oa);
  if (oa.sa_handler != SIG_IGN) sigaction(signal, &na, NULL);
}

int main(int argc, char** argv) {
  OSDomain osd(CPUS, "bzImage", 128);

  attach_bios(osd);
  attach_io(osd);
  attach_mmio(osd);
  attach_btrace(osd);
  set_signal_handler(SIGINT, btr_sigint_handler);
  attach_itrace(osd);

  start_prof(osd, "PROF");

  for (;;) {
    for (unsigned i = 0; i < CPUS; ++i) {
      osd.run(i, QUANT);
    }
  }

  return 0;
}
