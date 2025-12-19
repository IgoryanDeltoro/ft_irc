NAME = irc_bot

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD 

BOT_SRC = bot.cpp Bot.cpp 

BOT_OBJ = $(BOT_SRC:%.cpp=%.o)

BOT_DEP = $(BOT_OBJ:.o=.d)

all: $(NAME)

$(NAME): $(BOT_OBJ)
	$(CXX) $(CXXFLAGS) $(BOT_OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -rf $(BOT_OBJ) $(BOT_DEP)

fclean: clean
	-rm -f $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re