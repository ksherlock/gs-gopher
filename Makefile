MAKEFLAGS += --no-builtin-rules
.SUFFIXES:


CC = occ
AS = occ
CHTYP = iix chtyp
COPYFORK = iix copyfork
CFLAGS  =  -v -a0 -w-1
ASFLAGS = 
LDFLAGS =
LDLIBS =


OBJS = o/gopher.o o/q.o o/hierarchic.o

ROBJ = o/gopher.r

gopher: $(OBJS) $(ROBJ)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	$(COPYFORK) $(ROBJ) $@ -r
	$(CHTYP) -t s16 -a 56071 $@

o:
	mkdir o

o/gopher.o : gopher.c defines.h gopher.h hierarchic.h
o/index.o: index.c defines.h
o/url.o: url.c url.h
o/gopher-url.o: gopher-url.c gopher.h
o/hierarchic.o: hierarchic.c hierarchic.h


o/gopher.r: gopher.rez defines.h

o/q.o: q.c q.h

o/%.r : %.rez | o
	$(CC) -c -o $@ $<
	iix rlint $@


o/%.o : %.c | o
	$(CC) -c $(CFLAGS) -o $@ $<

