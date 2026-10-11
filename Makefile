CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -D_GNU_SOURCE \
          -I./src -I./llama.cpp/include -I./llama.cpp/ggml/include \
          $(shell pkg-config --cflags gtk+-3.0 libcurl espeak-ng)
LDLIBS  = $(shell pkg-config --libs gtk+-3.0 libcurl espeak-ng) \
          -lstdc++ -lgomp -lm -lpthread -ldl

# llama.cpp is a build-time dependency. Its sources contain scripts and
# tooling in other languages, so instead of being vendored here it is
# fetched and compiled *inside* ./llama.cpp by this Makefile.
LLAMA_DIR   = ./llama.cpp
LLAMA_BUILD = $(LLAMA_DIR)/build
LLAMA_LIBS  = $(LLAMA_BUILD)/src/libllama.a \
              $(LLAMA_BUILD)/ggml/src/libggml.a \
              $(LLAMA_BUILD)/ggml/src/libggml-cpu.a \
              $(LLAMA_BUILD)/ggml/src/libggml-base.a
LLAMA_STAMP = $(LLAMA_BUILD)/.neon_built

SRCS   = $(wildcard src/*.c)
OBJS   = $(SRCS:.c=.o)
TARGET = neoncore

all: $(TARGET)

$(TARGET): $(OBJS) $(LLAMA_STAMP)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LLAMA_LIBS) $(LDLIBS)

# Object files include headers from llama.cpp, so that folder must be
# fetched and built first. The "|" makes it an order-only prerequisite:
# it must exist before compiling, but changing it does not force a rebuild.
%.o: %.c | $(LLAMA_STAMP)
	$(CC) $(CFLAGS) -c $< -o $@

$(LLAMA_STAMP):
	@command -v cmake >/dev/null 2>&1 || { \
		echo "error: cmake is required to build llama.cpp" >&2; exit 1; }
	@if [ ! -d "$(LLAMA_DIR)" ]; then \
		echo "Fetching llama.cpp ..."; \
		git clone --depth 1 https://github.com/ggml-org/llama.cpp.git $(LLAMA_DIR); \
	fi
	cmake -S $(LLAMA_DIR) -B $(LLAMA_BUILD) -DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SHARED_LIBS=OFF -DGGML_NATIVE=ON -DGGML_OPENMP=ON \
		-DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF \
		-DLLAMA_BUILD_SERVER=OFF -DLLAMA_CURL=OFF
	cmake --build $(LLAMA_BUILD) --target llama ggml -j
	@touch $@

clean:
	rm -f $(OBJS) $(TARGET)

clean-llama:
	rm -rf $(LLAMA_BUILD) $(LLAMA_STAMP)

.PHONY: all clean clean-llama
