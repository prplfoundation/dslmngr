PROG = dslmngr
OBJS = dslmngr.o main.o

PROG_CFLAGS = $(CFLAGS) -fstrict-aliasing
PROG_LDFLAGS = $(LDFLAGS) -ldsl
PROG_LDFLAGS += -pthread -luci -lubus -lubox -lblobmsg_json -lnl-genl-3 -lnl-3

ifeq ($(TARGET_PLATFORM),INTEL)
PROG_LDFLAGS += -L/opt/intel/usr/lib -ldslfapi -lhelper -lsysfapi
endif

%.o: %.c
	$(CC) $(PROG_CFLAGS) $(FPIC) -c -o $@ $<

dslmngr: $(OBJS)
	$(CC) $(PROG_LDFLAGS) -o $@ $^

clean:
	rm -f *.o $(PROG)

.PHONY: clean
