#include <iostream>
#include <iomanip>
#include <cstdlib>

#include <qsim.h>

#include "io.h"

using namespace Qsim;
using namespace std;

struct io {
  io(OSDomain &osd);
  uint32_t *io_cb(int c, uint64_t p, uint8_t s, int w, uint32_t v);
  void cmos_wr(uint8_t addr, uint8_t val);
  uint8_t cmos_rd(uint8_t addr);

  OSDomain &osd;
};

io *iop;
void attach_io(OSDomain &osd) {
  iop = new io(osd);
}

io::io(OSDomain &osd): osd(osd) {
  osd.set_io_cb(this, &io::io_cb);
}

uint32_t *io::io_cb(int c, uint64_t p, uint8_t s, int w, uint32_t v) {
  static uint8_t cmosaddr(0x80);
  static uint32_t pciaddr, rval;
  bool return_value(false);

  // Writes to I/O ports
  if (w) switch(p) {
    case 0x20: // (old-school) PIC 1 control register 0
      cerr << "PIC 1 reg 0: 0x" << hex << v << endl;
      break;
    case 0x21: // (old-school) PIC 1 control register 1
      cerr << "PIC 1 reg 1: 0x" << hex << v << endl;
      break;
    case 0x70: // CMOS address
      cmosaddr = v&0x7f;
      break;
    case 0x71: // CMOS data
      cmos_wr(cmosaddr, v);
      break;
    case 0x80: // Diagnostic port. Used by Linux kernel for delay.
      break;
    case 0xa0: // (old-school) PIC 1 control register
      cerr << "PIC 1 reg 0: 0x" << hex << v << endl;
      break;
    case 0xa1: // (old-school) PIC 2 control register
      cerr << "PIC 2 reg 1: 0x" << hex << v << endl;
      break;
    case 0xf0: // FPU clear busy latch
      cerr << "FPU clear busy latch." << endl;
      break;
    case 0xf1: // FPU reset
      cerr << "FPU reset." << endl;
      break;
    case 0x3d4: // CGA index register
      cerr << "CGA index: " << dec << v << endl;
      break;
    case 0x3d5: // CGA data register
      cerr << "CGA data: " << dec << v << endl;
      break;
    case 0xcf8: // PCI configuration space address
      cerr << "PCI configuration space address set to 0x" << hex << v << endl;
      pciaddr = v;
      break;
    case 0xcfc: // PCI configuration space data
      cerr << "PCI configuration space write 0x" << hex << v
           << " to 0x" << hex << pciaddr << endl;
      break;
    default:
      cerr << "Unhandled " << dec << s*8 << "-bit write to port 0x"
           << hex << p << endl;
      exit(0);

  // Reads from I/O ports
  } else switch (p) {
    case 0x71:
      return_value = true;
      rval = cmos_rd(cmosaddr);
    case 0xcfc: // PCI configuration space data
      cerr << "PCI configuration space read at 0x" << hex << pciaddr << endl;
      // Return all 1s.
      return_value = true;
      rval = 0xffffffff;
      break;
    case 0xcfe: // PCI configuration space data: upper half.
      return_value = true;
      rval = 0xffff;
      break;
    default:
      cerr << "Unhandled " << dec << s*8 << "-bit read from port 0x"
           << hex << p << endl;
      exit(0);
  }

  if (return_value) return &rval;
  else return NULL;
}

void io::cmos_wr(uint8_t addr, uint8_t val) {
  switch(addr) {
    default:
      cerr << "Unhandled write 0x" << hex << unsigned(val)
           << " to CMOS address 0x" << hex << unsigned(addr) << endl;
      exit(0);
  }
}

uint8_t io::cmos_rd(uint8_t addr) {
  switch(addr) {
    case 0x0a: // RTC status A
      cerr << "Query RTC status A from CMOS." << endl;
      return 0x26;
    default:
      cerr << "Unhandled read from CMOS address 0x"
           << hex << unsigned(addr) << endl;
      exit(0);
  }

  return 0;
}
