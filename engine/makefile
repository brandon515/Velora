C_SOURCES = $(wildcard src/*/*.c) $(wildcard src/*.c)
CPP_SOURCES = $(wildcard src/*/*.cpp) $(wildcard src/*.cpp)
HEADERS = $(wildcard src/*/*.h) $(wildcard src/*.h)
OBJS = $(patsubst %.c, ../obj/%.o, $(notdir $(C_SOURCES))) $(patsubst %.cpp, ../obj/%.o, $(notdir $(CPP_SOURCES)))
CC = clang
CXX = clang++


C_FLAGS = -g -Wvarargs -Wall -Werror
CPP_FLAGS = -Wno-nullability-completeness -g -std=c++23
I_FLAGS = -Isrc -I$(VULKAN_SDK)/Include
L_FLAGS = -shared -luser32 -lvulkan-1 -L$(VULKAN_SDK)/Lib
DEFINES = -D_DEBUG -DVEXPORT -DVULKAN_RENDER -D_CRT_SECURE_NO_WARNINGS

../bin/engine.dll: $(OBJS)
	$(CC) -g $^ -o ../bin/engine.dll $(L_FLAGS)

../obj/%.o: src/*/%.c
	$(CC) $^ $(C_FLAGS) -c -o $@ $(DEFINES) $(I_FLAGS)

../obj/%.o: src/*/%.cpp
	$(CXX) $(CPP_FLAGS) $^ -c -o $@ $(I_FLAGS)