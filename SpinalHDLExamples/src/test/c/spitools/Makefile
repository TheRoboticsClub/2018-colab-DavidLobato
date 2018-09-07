CXX ?= clang++
CC ?= clang
PKG_CONFIG ?= pkg-config

MPSSE_LDLIBS ?= -lmpsse
MPSSE_CFLAGS ?= -I /usr/local/include

CFLAGS ?= -g
LDLIBS ?= 
LDLIBS += $(shell $(PKG_CONFIG) --libs libftdi1) $(MPSSE_LDLIBS)
CFLAGS += $(shell $(PKG_CONFIG) --cflags libftdi1) $(MPSSE_CFLAGS)



all: check spi_send

spi_send: spi_send.o
	$(CC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

check:
	@$(PKG_CONFIG) --print-errors --exists libftdi1

clean:
	rm -f spi_send *.o

.SECONDARY:
.PHONY: all clean check