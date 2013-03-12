#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cctype>

#include <stdint.h>

#include <qsim.h>

#include "mmio.h"

using namespace Qsim;
using namespace std;

static const uint64_t IOAPIC_BASE = 0xfec00000;

struct mmio {
  mmio(OSDomain &osd);
  void mem_cb(int c, uint64_t v, uint64_t p, uint8_t s, int w);

  OSDomain &osd;
};

mmio *mmiop;
void attach_mmio(OSDomain &osd) {
  mmiop = new mmio(osd);
}

mmio::mmio(OSDomain &osd): osd(osd) {
  osd.set_mem_cb(this, &mmio::mem_cb);
}

void mmio::mem_cb(int c, uint64_t v, uint64_t p, uint8_t s, int w) {
#if 0
  // Right now the early printks do not seem to be going to VGA memory. This
  // allows these messages to be read (but also prints a ton of unhelpful noise)
  if (w && s == 1) {
    uint8_t c;
    osd.mem_rd(c, p);
    if (isgraph(c) || isspace(c)) cout << c;
  }
#else
  if (w && p >= 0xa0000 && p <= 0xbffff) {
    for (unsigned i = 0; i < s; ++i) {
      uint8_t c;
      osd.mem_rd(c, p + i);
      cout << c;
    }
  }
#endif

  if (p >= IOAPIC_BASE && p < IOAPIC_BASE + 0x20) {
    static uint32_t apicaddr;
    if (w && p == IOAPIC_BASE) osd.mem_rd(apicaddr, p);
    else if (w) {
      cerr << "IOAPIC READ 0x" << hex << apicaddr << endl;
    } else {
      cerr << "IOAPIC WRITE 0x" << hex << apicaddr << endl;
    }
  }

}
