LIBDSL = libdsl.so
OBJS = dsl.o

ifneq ($(INTELCPE),)
  OBJS += intel.o
  LIBDSL_CFLAGS += -DINCLUDE_DSL_CPE_API_VRX
endif

all: $(LIBDSL)

%.o: %.c
	$(CC) $(LIBDSL_CFLAGS) $(CFLAGS) -fPIC -c -o $@ $<

libdsl.so: $(OBJS)
	$(CC) $(LIBDSL_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS) -shared -o $@ $^

clean:
	rm -f *.o $(LIBDSL)
