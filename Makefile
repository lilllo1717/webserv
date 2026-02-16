CXX             = c++
RM              = rm -f
CFLAGS          = -Wall -Wextra -Werror
INCLUDES        = -I include

SRCS            = src/main.cpp \
                  src/server/Server.cpp \
				  src/client/Client.cpp \
				  src/manager/Manager.cpp \
				  src/Listeners.cpp \
				  src/http/HttpRequest.cpp
# 				  src/configParser/parser.cpp 


                 

OBJS            = $(SRCS:.cpp=.o)
NAME            = webserv

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
