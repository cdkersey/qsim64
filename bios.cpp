#include <iostream>
#include <iomanip>
#include <cstdlib>

#include <qsim.h>

#include "bios.h"

using namespace std;
using namespace Qsim;

const uint64_t SFI_TABLE_BASE = 0xe0000, IOAPIC_BASE = 0xfec00000;

struct bios {
  bios(OSDomain &osd);
  int int_cb(int c, uint8_t v);

  // Memory map functions
  void e820(int c);
  void e801(int c);
  void h88(int c);

  void setup_sfi_table();

  OSDomain &osd;
};

static bios *bp;
void attach_bios(OSDomain &osd) { bp = new bios(osd); }

bios::bios(OSDomain &osd): osd(osd) {
  osd.set_int_cb(this, &bios::int_cb);
  setup_sfi_table();

  cerr << "IVT:" << endl << "  ";
  for (unsigned j = 0; j < 256; ++j) {
    for (unsigned i = 0; i < 4; ++i) {
      uint8_t c;
      osd.mem_rd(c, j*4+i);
      cerr << hex << setw(2) << setfill('0') << unsigned(c) << ' ';
    }
    cerr << endl << "  ";
  }
}

uint8_t buf_checksum(uint8_t *buf, size_t count) {
  uint8_t sum(0);
  for (size_t i = 0; i < count; ++i) sum += buf[i];
  return -sum;
}

// This uses casts that make language purists cry and may be a source of bugs
// in the future.
void bios::setup_sfi_table() {
  unsigned n(osd.get_n());
  uint8_t *t(osd.get_ramdesc().mem_ptr + SFI_TABLE_BASE),
          *u(t + 0x30), *v(u + 24 + n/4*16);
  size_t sz_t(0), sz_u(0), sz_v(0);

  // Identify initial table, "SYST"-- system table
  for (unsigned i = 0; i < 4; ++i) t[sz_t++] = "SYST"[i];
  // Size of entire system table: 36 bytes
  *((uint32_t*)(&t[4])) = 36; sz_t += 4;
  // SFI revision
  t[8] = 1; sz_t += 2;
  // Vendor id: "QSIM02"
  for (unsigned i = 0; i < 6; ++i) t[sz_t++] = "QSIM64"[i];
  // OEM table ID: "system"
  for (unsigned i = 0; i < 8; ++i) t[sz_t++] = "system\0\0"[i];
  // Write the checksum.
  t[9] += buf_checksum(t, sz_t);

  // Identify next table, the I/O APIC table
  for (unsigned i = 0; i < 4; ++i) u[sz_u++] = "APIC"[i];
  // Table length.
  *((uint32_t*)(&u[4])) = 32; sz_u += 4;
  // SFI revision
  u[8] = 1; sz_u += 2;
  // Vendor ID
  for (unsigned i = 0; i < 6; ++i) u[sz_u++] = "QSIM64"[i];
  // OEM table ID: "apic";
  for (unsigned i = 0; i < 8; ++i) u[sz_u++] = "apic\0\0\0\0"[i];
  // IO APIC base address
  *((uint64_t*)(&u[24])) = IOAPIC_BASE;
  // Write the checksum
  u[9] = buf_checksum(u, sz_u);

  // Finally, the CPU table
  for (unsigned i = 0; i < 4; ++i) v[sz_v++] = "CPUS"[i];
  *((uint32_t*)(&v[4])) = 24 + 4*n; sz_v += 4;
  v[8] = 1; sz_v += 2;
  for (unsigned i = 0; i < 6; ++i) v[sz_v++] = "QSIM64"[i];
  for (unsigned i = 0; i < 8; ++i) v[sz_v++] = "cpu\0\0\0\0\0"[i];
  for (unsigned i = 0; i < n; ++i) {
    *((uint32_t*)(&v[sz_v])) = i;
    sz_v += 4;
  }
  v[9] = buf_checksum(v, sz_v);

  if (buf_checksum(t, sz_t))
    cerr << "SFI syst table checksum invalid." << endl;
  if (buf_checksum(u, sz_u))
    cerr << "SFI apic table checksum invalid." << endl;
  if (buf_checksum(v, sz_v))
    cerr << "SFI cpu table checksum invalid." << endl;
}

unsigned get_ah(OSDomain &osd, unsigned c) {
  return (osd.get_reg(c, QSIM_RAX) >> 8)&0xff;
}

unsigned get_al(OSDomain &osd, unsigned c) {
  return osd.get_reg(c, QSIM_RAX) & 0xff;
}

void clear_carry_flag(OSDomain &osd, int c) {
  const uint32_t MASK(~1u);
  osd.set_reg(c, QSIM_RFLAGS, osd.get_reg(c, QSIM_RFLAGS) & MASK);
}

void set_carry_flag(OSDomain &osd, int c) {
  osd.set_reg(c, QSIM_RFLAGS, osd.get_reg(c, QSIM_RFLAGS) | 1);
}

int bios::int_cb(int c, uint8_t v) {
  //cerr << "Interrupt, on CPU " << c << ", vec 0x" << hex << unsigned(v) << endl;
  switch (v) {
    // Exceptions
    case 0x0:  case 0x1:  case 0x2:  case 0x3:
    case 0x4:  case 0x5:  case 0x6:  case 0x7:
    case 0x8:  case 0x9:  case 0xa:  case 0xb:
    case 0xc:  case 0xd:  case 0xe:  case 0xf:
      break;

    // Video services
    case 0x10: {
      unsigned ah(get_ah(osd, c));
      if (ah == 0x0e) {
        cout << char(get_al(osd, c));
      } else {
        cerr << "Video interrupt 0x" << hex << get_ah(osd, c) << endl;
      }
      break;
    }

    // Miscallaneous
    case 0x15: {
      unsigned ah(get_ah(osd, c)), al(get_al(osd, c));
      if (ah == 0x88) {
        h88(c);
      } else if (ah == 0xc0) {
        cerr << "No we do not have an MCA bus." << endl;
        set_carry_flag(osd, c);
      } else if (ah == 0xec && al == 0x00) {
        // Do nothing. "Set target operating mode" interrupt
        cerr << "Set target operating mode to "
             << (osd.get_reg(c, QSIM_RBX) & 0xffff) << endl;
      } else if (ah == 0xe8 && al == 0x20) {
        e820(c);
      } else if (ah == 0xe8 && al == 0x01) {
        e801(c);
      } else if (ah == 0xe9 && al == 0x80) {
        // Query for Intel Speedstep. Other functions? Ignore for now.
        cerr << "Ignore IST support query." << endl;
      } else {
        cerr << "Unhandled interrupt 0x15, ah=0x" << hex << ah
             << ", al=0x" << hex << al << endl;
        exit(0);
      }
      break;
    }

    // Keyboard services
    case 0x16: {
      unsigned ah(get_ah(osd, c));
      cerr << "Keyboard interrupt 0x" << hex << ah << '(';
      if (ah == 3) cerr << "repeat rate";
      else         cerr << "unknown";
      cerr << ')' << endl;
      break;
    }

    default:
      cerr << "Unhandled interrupt 0x" << hex << unsigned(v) << endl;
      exit(0);
  }
}

void copy_to_guest(OSDomain &osd, uint64_t addr, uint8_t *buf, size_t sz) {
  for (size_t i = 0; i < sz; ++i) osd.mem_wr(addr + i, buf[i]);
}

void bios::e820(int c) {
  cout << "e820" << endl;

  // For now, let's just do one big 128MB region
  uint8_t entry[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00
  };

  // Compute address from segment/offset
  uint64_t addr((osd.get_reg(c, QSIM_ES)<<4) + osd.get_reg(c, QSIM_RDI));

  copy_to_guest(osd, addr, entry, sizeof(entry));

  osd.set_reg(c, QSIM_RAX, 0x534d4150); // ASCII "SMAP" signature
  osd.set_reg(c, QSIM_RBX, 0);          // No continuation-- only one entry.

  clear_carry_flag(osd, c);             // "no error"
}

void bios::e801(int c) {
  clear_carry_flag(osd, c);
  osd.set_reg(c, QSIM_RAX, 0x3c00);
  osd.set_reg(c, QSIM_RBX, 0x700);
  osd.set_reg(c, QSIM_RCX, 0x3c00);
  osd.set_reg(c, QSIM_RDX, 0x700);
}

void bios::h88(int c) {
  clear_carry_flag(osd, c);
  osd.set_reg(c, QSIM_RAX, 0xffff);
}
