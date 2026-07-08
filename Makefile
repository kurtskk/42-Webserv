NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iincludes

SRC_DIR = srcs
OBJ_DIR = obj

OBJ_SUBDIRS = $(OBJ_DIR) \
              $(OBJ_DIR)/config \
              $(OBJ_DIR)/server \
              $(OBJ_DIR)/http

SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/config/LocationConfig.cpp \
       $(SRC_DIR)/config/ServerConfig.cpp \
       $(SRC_DIR)/config/Config.cpp \
       $(SRC_DIR)/server/Client.cpp \
       $(SRC_DIR)/server/SocketUtils.cpp \
       $(SRC_DIR)/server/WebServ.cpp \
       $(SRC_DIR)/http/HttpRequest.cpp \
       $(SRC_DIR)/http/HttpResponse.cpp \
       $(SRC_DIR)/http/CgiHandler.cpp

OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJ_SUBDIRS) $(OBJS)
	@echo "\n"
	@echo "\033[0;32mLinking webserv...\033[0m"
	@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	@echo "\033[0;32mDone!\033[0m\n"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@printf "\033[0;33mCompiling... %-40.40s\r\033[0m" $<
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_SUBDIRS):
	@mkdir -p $@

clean:
	@echo "\033[0;33mCleaning object files...\033[0m"
	@rm -rf $(OBJ_DIR)
	@echo "\033[0;32mDone!\033[0m"

fclean: clean
	@echo "\033[0;33mRemoving executable...\033[0m"
	@rm -f $(NAME)
	@echo "\033[0;32mDone!\033[0m"

re: fclean all

.PHONY: all clean fclean re
