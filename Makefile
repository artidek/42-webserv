CPP = c++
CPPFLAGS = -std=c++98 -Wall -Wextra -Werror -g

SRCPATH = src/
UTILSPATH = utils/

SRC = $(SRCPATH)configParser.cpp $(SRCPATH)serverConfig.cpp $(SRCPATH)configHandler.cpp $(SRCPATH)server.cpp \
	  $(SRCPATH)requestHandler.cpp
UTILS = $(UTILSPATH)errorHandler.cpp $(UTILSPATH)configUtils.cpp

SRCOBJS = $(patsubst %.cpp, ./objs/%.o, $(notdir $(SRC)))
UTILSOBJS = $(patsubst %.cpp, ./objs/%.o, $(notdir $(UTILS)))
OBJS = $(SRCOBJS) $(UTILSOBJS)

LIBR = ./objs/lftwebserv.a
NAME = webserv

./objs/%.o: $(SRCPATH)%.cpp
	@mkdir -p objs
	@$(CPP) $(CPPFLAGS) -c $< -o $@

./objs/%.o: $(UTILSPATH)%.cpp
	@mkdir -p objs
	@$(CPP) $(CPPFLAGS) -c $< -o $@

$(NAME): $(LIBR)
	@$(CPP) $(CPPFLAGS) main.cpp $(LIBR) -o $(NAME)
	@echo "Build complete"

all: $(NAME)

$(LIBR): $(OBJS)
	@ar rcs $(LIBR) $(OBJS)

clean:
	@rm -f $(OBJS)
	@echo "clean complete"

fclean:
	@rm -rf ./objs
	@rm -f $(NAME)
	@echo "full clean complete"

re: fclean $(NAME)

.PHONY: all clean fclean re
