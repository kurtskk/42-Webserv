#ifndef SOCKETUTILS_HPP
# define SOCKETUTILS_HPP

# include <sys/socket.h>
# include <netinet/in.h>
# include <fcntl.h>
# include <unistd.h>

class SocketUtils {
private:
	SocketUtils();
	SocketUtils(const SocketUtils &other);
	SocketUtils &operator=(const SocketUtils &other);
	~SocketUtils();

public:
	static int createListeningSocket(int port);
	static void setNonBlocking(int fd);
};

#endif
