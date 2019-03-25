LIBDSL = libdsl.so

if ($(PLATFORM),INTEL)
SRCS := $(shell ls intel/*.c)
else
$(error Unknown PLATFORM: $(PLATFORM))
endif
OBJS := $(SRCS:.c=.o)

all: $(LIBDSL)

%.o: %.c
	$(CC) $(LIBDSL_CFLAGS) $(CFLAGS) -fPIC -c -o $@ $<

libdsl.so: $(OBJS)
	$(CC) $(LIBDSL_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS) -shared -o $@ $^

clean:
	rm -f *.o $(LIBDSL)

export SRCS OBJS CFLAGS LOCAL_CFLAGS
debug:
	@echo "PLATFORM = $$PLATFORM"
	@echo "SRCS = $$SRCS"
	@echo "OBJS = $$OBJS"
	@echo "CFLAGS = $$CFLAGS"

.PHONY: all clean debug
