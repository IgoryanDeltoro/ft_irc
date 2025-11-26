NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD #-pedantic
SRC = main.cpp Server.cpp Client.cpp
BUILD = ./build

OBJ = $(SRC:%.cpp=$(BUILD)/%.o)
DEP = $(OBJ:.o=.d)
all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(BUILD)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -rf $(BUILD)

fclean: clean
	-rm -f $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re