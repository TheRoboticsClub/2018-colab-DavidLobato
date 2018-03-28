PROJ = spi_slave_controller
DEPS = rom8x16.v
PIN_DEF = iCE40UP5K-B-EVN.pcf
DEVICE = up5k

ARACHNE = arachne-pnr 
ARACHNE_ARGS = 
ICEPACK = icepack
ICETIME = icetime
ICEPROG = iceprog

CXX ?= clang++
CC ?= clang
PKG_CONFIG ?= pkg-config
CFLAGS = -g
LDLIBS = 
LDLIBS += $(shell $(PKG_CONFIG) --libs libftdi)
CFLAGS += $(shell $(PKG_CONFIG) --cflags libftdi)


all: $(PROJ).bin spi_send_command

%.blif: %.v $(DEPS)
	yosys -p 'synth_ice40 -blif $@' $^

%.asc: $(PIN_DEF) %.blif
	$(ARACHNE) $(ARACHNE_ARGS) -d $(subst up,,$(subst hx,,$(subst lp,,$(DEVICE)))) -o $@ -p $^

%.bin: %.asc
	$(ICEPACK) $< $@

%.rpt: %.asc
	$(ICETIME) -d $(DEVICE) -mtr $@ $<

%_tb: %_tb.v %.v $(DEPS)
	iverilog -o $@ $^

%_tb.vcd: %_tb
	vvp -N $< +vcd=$@

prog: $(PROJ).bin
	$(ICEPROG) -S $<

sudo-prog: $(PROJ).bin
	@echo 'Executing prog as root!!!'
	sudo $(ICEPROG) -S $<

spi_send_command: spi_send_command.o
	$(CC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

clean:
	rm -f $(PROJ).blif $(PROJ).asc $(PROJ).rpt $(PROJ).bin $(PROJ)_tb $(PROJ)_tb.vcd spi_send_command

.SECONDARY:
.PHONY: all prog clean
