NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD

BUILD_DIR := ./build
SRC_DIRS := src

SRC =	$(SRC_DIRS)/main.cpp			\
		$(SRC_DIRS)/Server.cpp			\
		$(SRC_DIRS)/Client.cpp			\
		$(SRC_DIRS)/Parser.cpp			\
		$(SRC_DIRS)/Command.cpp			\
		$(SRC_DIRS)/Channel.cpp			\
		$(SRC_DIRS)/utils/utils.cpp		\
		$(SRC_DIRS)/utils/reply.cpp		\
		$(SRC_DIRS)/utils/sender.cpp	\
		$(SRC_DIRS)/utils/reciver.cpp	\
		$(SRC_DIRS)/utils/close.cpp		\
		$(SRC_DIRS)/commands/cap.cpp	\
		$(SRC_DIRS)/commands/help.cpp	\
		$(SRC_DIRS)/commands/invite.cpp	\
		$(SRC_DIRS)/commands/join.cpp	\
		$(SRC_DIRS)/commands/kick.cpp	\
		$(SRC_DIRS)/commands/mode.cpp	\
		$(SRC_DIRS)/commands/nick.cpp	\
		$(SRC_DIRS)/commands/pass.cpp	\
		$(SRC_DIRS)/commands/ping.cpp	\
		$(SRC_DIRS)/commands/pong.cpp	\
		$(SRC_DIRS)/commands/topic.cpp	\
		$(SRC_DIRS)/commands/user.cpp	\
		$(SRC_DIRS)/commands/privMsg.cpp\
		$(SRC_DIRS)/commands/away.cpp	\
		$(SRC_DIRS)/commands/quit.cpp	\
		$(SRC_DIRS)/commands/part.cpp	\


OBJ = $(SRC:%.cpp=$(BUILD_DIR)/%.o)
DEP = $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -rf $(BUILD_DIR)

fclean: clean
	-rm -f $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re