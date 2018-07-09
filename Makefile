PROG = dslmgr
OBJS = dslmgr.o dslmgr_nl.o main.o

PROG_CFLAGS = $(CFLAGS) -fstrict-aliasing -I./libdsl
PROG_LDFLAGS = $(LDFLAGS) -L. -L./libdsl -ldsl -pthread
PROG_LDFLAGS += -luci -lubus -lubox -lblobmsg_json -lnl-genl-3 -lnl-3

%.o: %.c
	$(CC) $(PROG_CFLAGS) $(FPIC) -c -o $@ $<

all: libdsl.so dslmgr

dslmgr: $(OBJS)
	$(CC) $(PROG_LDFLAGS) -o $@ $^

libdsl.so:
	$(MAKE) -C libdsl $@

clean:
	$(MAKE) -C libdsl clean
	rm -f *.o $(PROG)
