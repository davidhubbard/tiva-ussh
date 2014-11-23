#
# tiva-ussh libti2cit Makefile Copyright (c) 2014 David Hubbard github.com/davidhubbard
# Licensed under the GNU LGPL v3. See README.md for more information.
#

.PHONY: all clean lm4flash

PART=TM4C1294NCPDT
IPATH=../../tivaware
include makedefs

TARGET=example-main.elf

all: $(TARGET)
clean:
	rm -rf $(TARGET) *.o

lm4flash: all
	@echo "Programming device with: $(TARGET:.elf=.bin)"
	lm4flash $(TARGET:.elf=.bin)

$(TARGET): $(TARGET:.elf=.o) example-poll.o example-isrnofifo.o example-isr.o \
	libti2cit.o startup_${COMPILER}.o project.ld

libti2cit.o: libti2cit.c libti2cit.h
example-poll.o: example-main.h example-poll.c
example-isr.o: example-main.h example-isr.c

SCATTERgcc_example-main=project.ld
ENTRY_example-main=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C129_RA1 -ggdb -Wall
