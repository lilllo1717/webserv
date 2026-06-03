CXX             = g++
RM              = rm -f
CFLAGS          = -Wall -Wextra -Werror -std=c++17 
INCLUDES        = -I include

SRCS            = src/main.cpp \
                  src/server/Server.cpp \
				  src/client/Client.cpp \
				  src/manager/Manager.cpp \
				  src/Listeners.cpp \
				  src/http/ParseHeaders.cpp \
				  src/http/ParseBody.cpp \
				  src/http/ParseRequestLine.cpp \
				  src/http/HttpRequest.cpp \
				  src/http/HttpResponse.cpp \
				  src/http/Serializer.cpp \
				  src/http/RequestHandler.cpp \
				  src/configParser/parser.cpp \
				  src/configParser/tokenizer.cpp \
				  src/cgi/Cgi.cpp \
				  src/cgi/Envp.cpp \
				  
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
