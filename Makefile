NAME = test

CPP = c++
FLAGS = -Wall -Wextra -Werror -std=c++17

SRCS = src/main.cpp src/Game.cpp
GLAD_SRC = src/thirdparty/glad/src/glad.c

OBJS = $(SRCS:.cpp=.o) $(GLAD_SRC:.c=.o)
INCLUDES = -Isrc -Isrc/thirdparty -Isrc/thirdparty/glad/include
LIBS = -lSDL2 -lGL -ldl

all: $(NAME)

$(NAME): $(OBJS)
	$(CPP) $(FLAGS) $(OBJS) -o $(NAME) $(LIBS)

# Compile C++ sources
%.o: %.cpp
	$(CPP) $(FLAGS) $(INCLUDES) -c $< -o $@

# Compile C sources
%.o: %.c
	$(CPP) $(FLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
