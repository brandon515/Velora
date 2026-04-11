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
L_LINUX_FLAGS = -lvulkan -L$(VULKAN_SDK)/lib -lX11 -lm -lstdc++
L_WINDOWS_FLAGS = -L$(VULKAN_SDK)/lib -lvulkan-1 -L"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64" -luser32
DEFINES = -D_DEBUG -DVULKAN_RENDER -D_CRT_SECURE_NO_WARNINGS

.PHONY: Linux
Linux: $(SPV) bin/engine

.PHONY: Windows
Windows: $(SPV) bin/engine.exe

bin/engine: $(OBJS) 
	$(CC) -g $^ -o bin/engine $(L_LINUX_FLAGS)

bin/engine.exe: $(OBJS)
	$(CC) -g $^ -o bin/engine.exe $(L_WINDOWS_FLAGS)

obj/%.o: src/*/%.c
	$(CC) $^ $(C_FLAGS) -c -o $@ $(DEFINES) $(I_FLAGS)

obj/%.o: src/*/%.cpp
	$(CXX) $(CPP_FLAGS) $^ -c -o $@ $(I_FLAGS)

bin/%.spv: src/*/%.glsl
	glslc $^ -o $@