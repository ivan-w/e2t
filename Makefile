CC390=s390x-linux-gnu-gcc
ARCHS= \
	z900 \
	z10 \
	z13

C390FLAGS=-O3 -m31 -fno-asynchronous-unwind-tables -fomit-frame-pointer
# C390FLAGS=-O3 -m31 -fPIC -fPIE -fno-asynchronous-unwind-tables -fomit-frame-pointer
# C390FLAGS=-O3 -m31
CFLAGS=-O3

OBJS= \
	e2t_main.o  \
	e2t_asm.o  \
	e2t_elf.o 

all: docomp.done e2t_sample e2t

docomp.done: e2t_sample.c
	./docomp $(CC390) "$(C390FLAGS)" $(ARCHS)

e2t_sample: e2t_sample.c
	$(CC) $(CFLAGS) e2t_sample.c -o e2t_sample

clean:
	rm -rf e2t_sample_s390.o
	rm -rf e2t_sample
	rm -f docomp.done
	rm -f $(OBJS)
	rm -f e2t
	rm -f e2t_sample_*

e2t: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -lelf -lcrypto -lssl -o $@

.c.o:
	$(CC) $(CFLAGS) $*.c -c -o $*.o
