QSIM_PREFIX ?= /usr/local
LDFLAGS = -I$(QSIM_PREFIX)/include -L$(QSIM_PREFIX)/lib \
          -ldl -lstdc++ -lpthread -lqsim
CXXFLAGS ?= -O0 -g

qsim64: qsim64.o bios.o io.o mmio.o btrace.o itrace.o

qsim64.o: qsim64.cpp bios.h io.h mmio.h btrace.h itrace.h
bios.o: bios.cpp bios.h
io.o: io.cpp io.h
mmio.o: mmio.cpp mmio.h
btrace.o: btrace.cpp btrace.h
itrace.o: itrace.cpp itrace.h

clean:
	rm -f qsim64 *.o
