AR		  =  ar rvs
CC	      =  gcc -std=c99
CFLAGS	  = -g -Wall -pedantic
OPTFLAGS  = # -O2
INCLUDES  = -Iheader
LDFLAGS   = -Llib -lutils -lthreadpool -lpthread -ldict

STATICLIB =  lib/libutils.a lib/libthreadpool.a lib/libdict.a
BIN       =  bin/client bin/server bin/supervisor

.PHONY: all test debug clean cleanall
.SUFFIXES: .c .h .o .a

bin/%: src/%.c $(STATICLIB)
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

lib/lib%.a: lib/%.o header/%.h
	$(AR) $@ $<

all: $(BIN)

test: all
	./test.sh

debug: all
	./test.sh --debug

clean:
	-rm -f *~ lib/*~ lib/*.[ao] header/*~ src/*~ log/* OOB-server-*

cleanall: clean
	-rm -f $(BIN)
