CXX ?= clang++
PKG_CONFIG ?= pkg-config

GEMMLOWP_PATH ?= ~/src/gemmlowp

CXXFLAGS ?= -g --std=c++11 -I $(GEMMLOWP_PATH)/public $(shell $(PKG_CONFIG) --cflags jsoncpp)
LDLIBS ?= -lpthread $(shell $(PKG_CONFIG) --libs jsoncpp)

all: xor_mlp_inference_float xor_mlp_inference_quantized

%.o: %cc
	$(CXX) -c -o $@ $< $(CXXFLAGS)

xor_mlp_inference_float: xor_mlp_inference_float.o json_ndarray.o
	$(CXX) -o $@ $(CXXFLAGS) $^ $(LDLIBS)

xor_mlp_inference_quantized: xor_mlp_inference_quantized.o json_ndarray.o
	$(CXX) -o $@ $(CXXFLAGS) $^ $(LDLIBS)

check:
	@$(PKG_CONFIG) --print-errors --exists jsoncpp

clean:
	rm -f *.o xor_mlp_inference_float xor_mlp_inference_quantized

.PHONY: all clean check