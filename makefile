C_SOURCES = $(wildcard src/*/*.c) $(wildcard src/*.c)
CPP_SOURCES = $(wildcard src/*/*.cpp) $(wildcard src/*.cpp)
HEADERS = $(wildcard src/*/*.h) $(wildcard src/*.h)
OBJS = $(patsubst %.c, obj/%.o, $(notdir $(C_SOURCES))) $(patsubst %.cpp, obj/%.o, $(notdir $(CPP_SOURCES)))
SHADERS = $(wildcard src/*/*.glsl)
SPV = $(patsubst %.glsl, bin/%.spv, $(notdir $(SHADERS)))
CC = clang
CXX = clang++


C_FLAGS = -g -Wvarargs -Wall -Werror
CPP_FLAGS = -Wno-nullability-completeness -g -std=c++23
I_FLAGS = -Isrc -I$(VULKAN_SDK)/include
L_FLAGS = -lvulkan -L$(VULKAN_SDK)/lib -lX11 -lm -lstdc++
DEFINES = -D_DEBUG -DVULKAN_RENDER -D_CRT_SECURE_NO_WARNINGS

ALL: $(SPV) bin/engine

bin/engine: $(OBJS) 
	$(CC) -g $^ -o bin/engine $(L_FLAGS)

obj/%.o: src/*/%.c
	$(CC) $^ $(C_FLAGS) -c -o $@ $(DEFINES) $(I_FLAGS)

obj/%.o: src/*/%.cpp
	$(CXX) $(CPP_FLAGS) $^ -c -o $@ $(I_FLAGS)

bin/%.spv: src/*/%.glsl
	glslc $^ -o $@