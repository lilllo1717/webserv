#include "webserv.hpp"

int main(int argc, char **argv)
{
	if (argc != 2)
		return (1);

		
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		std::cerr << "Socket creation failed.\n";
		return (1);
	}
	return (0);

}