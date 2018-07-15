CXX ?= clang++
CC ?= clang
PKG_CONFIG ?= pkg-config
CFLAGS ?= -g
LDLIBS ?= 
LDLIBS += $(shell $(PKG_CONFIG) --libs libftdi1)
CFLAGS += $(shell $(PKG_CONFIG) --cflags libftdi1)


all: check spi_send

spi_send: spi_send.o
	$(CC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

check:
	@$(PKG_CONFIG) --print-errors --exists libftdi1


clean:
	rm -f spi_send *.o

.SECONDARY:
.PHONY: all clean check