#ifndef __BTRACE_H
#define __BTRACE_H

#include <iostream>

#include <qsim.h>

void attach_btrace(Qsim::OSDomain &osd);
void print_btrace(std::ostream &out = std::cout);

#endif
