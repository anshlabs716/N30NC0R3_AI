CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -D_GNU_SOURCE -I./src -I./llama.cpp/include -I./llama.cpp/ggml/include \
	$(shell pkg-config --cflags gtk+-3.0 libcurl)
LDLIBS = $(shell pkg-config --libs gtk+-3.0 libcurl) -lespeak-ng -lm -lpthread

LLAMA_DIR = ./llama.cpp
LLAMA_LIB = $(LLAMA_DIR)/build/libllama.a

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)
TARGET = neon_core

all: $(TARGET)

$(TARGET): $(OBJS) $(LLAMA_LIB)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LLAMA_LIB) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# llama.cpp is a build-time dependency and is intentionally not vendored
# in this repository. It is fetched on demand and linked directly.
$(LLAMA_LIB):
	@if [ ! -d "$(LLAMA_DIR)" ]; then \
		git clone --depth 1 https://github.com/ggml-org/llama.cpp.git $(LLAMA_DIR); \
	fi
	cd $(LLAMA_DIR) && cmake -B build -DGGML_NATIVE=1 \
		-DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF -DLLAMA_BUILD_SERVER=OFF \
		&& cmake --build build -j

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
