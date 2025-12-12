NAME = test

CPP = c++
FLAGS = -Wall -Wextra -Werror -std=c++17

SRCS = src/main.cpp src/Game.cpp src/Player.cpp
GLAD_SRC = src/thirdparty/glad/src/glad.c

OBJS = $(SRCS:.cpp=.o) $(GLAD_SRC:.c=.o)

INCLUDES = -Isrc -Isrc/thirdparty -Isrc/thirdparty/glad/include
# Use sdl3 via pkg-config (assumes SDL3 is installed)
SDL_CFLAGS := $(shell pkg-config --cflags sdl3)
SDL_LIBS := $(shell pkg-config --libs sdl3)

INCLUDES = -Isrc -Isrc/thirdparty -Isrc/thirdparty/glad/include $(SDL_CFLAGS)
LIBS = $(SDL_LIBS) -lGL -ldl

# Linux build
all: $(NAME)

$(NAME): $(OBJS)
	$(CPP) $(FLAGS) $(OBJS) -o $(NAME) $(LIBS)

%.o: %.cpp
	$(CPP) $(FLAGS) $(INCLUDES) -c $< -o $@

%.o: %.c
	$(CPP) $(FLAGS) $(INCLUDES) -c $< -o $@

# Windows cross-compile build
WIN_CPP ?= x86_64-w64-mingw32-g++
WIN_FLAGS ?= -Wall -Wextra -Werror -std=c++17
WIN_NAME ?= test.exe

# Windows (Mingw) SDL3/SDL2 include/lib locations. Users can override these if needed.
WIN_SDL_INC ?= -Isrc/thirdparty/SDL3/windows/include
# Additional include path for mingw build (can be overridden)
## If glm is installed on the host, add only its include path so mingw can find the header-only glm
WIN_INCLUDES ?= $(INCLUDES) -Isrc/thirdparty/glm
# Default win SDL lib dir; users can override
WIN_SDL_LIBDIR ?= src/thirdparty/SDL3/windows/lib

## Auto-detect SDL import libs under src/thirdparty if default dir doesn't contain them
ifneq ($(strip $(wildcard $(WIN_SDL_LIBDIR)/libSDL3*.a $(WIN_SDL_LIBDIR)/libSDL2*.a)),)
# keep default WIN_SDL_LIBDIR
else
DETECTED_WIN_SDL_LIB := $(shell find src/thirdparty -type f -name 'libSDL3*.a' -print -quit)
ifeq ($(strip $(DETECTED_WIN_SDL_LIB)),)
DETECTED_WIN_SDL_LIB := $(shell find src/thirdparty -type f -name 'libSDL2*.a' -print -quit)
endif
ifeq ($(strip $(DETECTED_WIN_SDL_LIB)),)
$(info No SDL import libs found in $(WIN_SDL_LIBDIR); keep default or override WIN_SDL_LIBDIR)
else
WIN_SDL_LIBDIR := $(dir $(DETECTED_WIN_SDL_LIB))
$(info Auto-detected WIN_SDL_LIBDIR=$(WIN_SDL_LIBDIR))
endif
endif

## Determine SDL base and whether a main import lib exists
WIN_SDL_BASE :=
WIN_SDL_MAIN :=
ifneq ($(strip $(wildcard $(WIN_SDL_LIBDIR)/libSDL3*.a)),)
WIN_SDL_BASE := SDL3
ifneq ($(strip $(wildcard $(WIN_SDL_LIBDIR)/libSDL3main.a $(WIN_SDL_LIBDIR)/libSDL3_main.a)),)
WIN_SDL_MAIN := -lSDL3main
endif
else ifneq ($(strip $(wildcard $(WIN_SDL_LIBDIR)/libSDL2*.a)),)
WIN_SDL_BASE := SDL2
ifneq ($(strip $(wildcard $(WIN_SDL_LIBDIR)/libSDL2main.a $(WIN_SDL_LIBDIR)/libSDL2_main.a)),)
WIN_SDL_MAIN := -lSDL2main
endif
endif
# Support both SDL3 and SDL2 linker names; override if needed
WIN_LIBS ?= -lmingw32 -lSDL3 -lopengl32 \
           -static-libgcc -static-libstdc++ -lwinmm -lgdi32

WIN_OBJS = $(SRCS:.cpp=.win.o) $(GLAD_SRC:.c=.win.o)

WINDOWS_DIST_DIR ?= dist/windows
PACKAGE_EXE ?= $(WIN_NAME)

windows: $(WIN_NAME)
$(WIN_NAME): $(WIN_OBJS)
	@echo "Linking Windows exe: $(WIN_NAME)"
	@# If WIN_SDL_LIBDIR doesn't contain SDL import libs, try to locate them under src/thirdparty
	@if [ -z "$(WIN_SDL_BASE)" ]; then \
		echo "ERROR: Could not find SDL3/SDL2 import libs in $(WIN_SDL_LIBDIR)"; \
		echo "Please download the Windows (MinGW) development package for SDL3 or SDL2 and place the .a libraries in $(WIN_SDL_LIBDIR)"; \
		echo "Example: download SDL from https://www.libsdl.org/download-3.0.php (SDL3) or https://www.libsdl.org/download-2.0.php (SDL2) - extract the dev build and copy include/lib directories to src/thirdparty/SDL3/windows or override WIN_SDL_LIBDIR/INC"; \
		exit 1; \
	fi
	@echo "Using SDL base: $(WIN_SDL_BASE) $(WIN_SDL_MAIN)"
	$(WIN_CPP) $(WIN_FLAGS) $(WIN_OBJS) -o $(WIN_NAME) -L"$(WIN_SDL_LIBDIR)" $(WIN_SDL_MAIN) -l$(WIN_SDL_BASE) -lmingw32 -lopengl32 -static-libgcc -static-libstdc++ -lwinmm -lgdi32

%.win.o: %.cpp
	$(WIN_CPP) $(WIN_FLAGS) $(WIN_INCLUDES) $(WIN_SDL_INC) -c $< -o $@

%.win.o: %.c
	$(WIN_CPP) $(WIN_FLAGS) $(WIN_INCLUDES) $(WIN_SDL_INC) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

clean_windows:
	rm -f $(WIN_OBJS) $(WIN_NAME)

re: fclean all

.PHONY: all clean fclean re clean_windows windows
