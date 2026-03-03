CXX				= c++
RM				= rm -f
CXXFLAGS		= -Wall -Wextra -Werror -MMD -MP

SRCS			=	tokenizer.cpp parser.cpp
					
OBJS			= $(SRCS:.cpp=.o)
DEPS			= $(OBJS:.o=.d)

NAME			= testParser

all:	$(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

-include $(DEPS)

clean:
	$(RM) $(OBJS) $(DEPS)

fclean:	clean
	$(RM) $(NAME)

re:	fclean all

.PHONY:	all clean fclean re