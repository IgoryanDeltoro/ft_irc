NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -pedantic
SRC = main.cpp

OBJ = $(SRC:.cpp=.o)
DEP = $(OBJ:.o=.d)
all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -f $(OBJ) $(DEP)

fclean: clean
	-rm -f $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re