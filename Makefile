NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD #-pedantic

BUILD_DIR := ./build
SRC_DIRS := src

MODE ?= release

ifeq ($(MODE),debug)
    CXXFLAGS += -DDEBUG=1
endif

SRC =	$(SRC_DIRS)/main.cpp			\
		$(SRC_DIRS)/Server.cpp			\
		$(SRC_DIRS)/Client.cpp			\
		$(SRC_DIRS)/Parser.cpp			\
		$(SRC_DIRS)/Command.cpp			\
		$(SRC_DIRS)/Channel.cpp			\
		$(SRC_DIRS)/Utils.cpp			\
		$(SRC_DIRS)/reply.cpp			\
		$(SRC_DIRS)/commands/Cap.cpp	\
		$(SRC_DIRS)/commands/Help.cpp	\
		$(SRC_DIRS)/commands/Invite.cpp	\
		$(SRC_DIRS)/commands/Join.cpp	\
		$(SRC_DIRS)/commands/Kick.cpp	\
		$(SRC_DIRS)/commands/Mode.cpp	\
		$(SRC_DIRS)/commands/Nick.cpp	\
		$(SRC_DIRS)/commands/Pass.cpp	\
		$(SRC_DIRS)/commands/Ping.cpp	\
		$(SRC_DIRS)/commands/Topic.cpp	\
		$(SRC_DIRS)/commands/User.cpp	\
		$(SRC_DIRS)/commands/PrivMsg.cpp\


OBJ = $(SRC:%.cpp=$(BUILD_DIR)/%.o)
DEP = $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -DDEBUG=1" all

clean:
	-rm -rf $(BUILD_DIR)

fclean: clean
	-rm -f $(NAME)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re