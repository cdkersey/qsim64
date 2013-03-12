#ifndef __ITRACE_H
#define __ITRACE_H

#include <iostream>

#include <qsim.h>

void attach_itrace(Qsim::OSDomain &osd);
void detach_itrace();

#endif
