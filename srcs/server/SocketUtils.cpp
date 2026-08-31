#include "SocketUtils.hpp"
#include <stdexcept>
#include <cstring>
#include <iostream>

SocketUtils::SocketUtils() {}
SocketUtils::SocketUtils(const SocketUtils &) {}
SocketUtils &SocketUtils::operator=(const SocketUtils &) { return *this; }
SocketUtils::~SocketUtils() {}

int SocketUtils::createListeningSocket(int port) {
	int listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd < 0)
		throw std::runtime_error("Failed to create socket");
	int opt = 1;
	if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(listenFd);
		throw std::runtime_error("Failed to setsockopt SO_REUSEADDR");
	}
	int bufferSize = 1024 * 1024;
	if (setsockopt(listenFd, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize)) < 0)
		std::cerr << "Warning: Failed to increase SO_RCVBUF" << std::endl;
	if (setsockopt(listenFd, SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize)) < 0)
		std::cerr << "Warning: Failed to increase SO_SNDBUF" << std::endl;
	struct sockaddr_in serverAddr;
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);
	if (bind(listenFd, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
		close(listenFd);
		throw std::runtime_error("Failed to bind socket");
	}
	if (listen(listenFd, SOMAXCONN) < 0) {
		close(listenFd);
		throw std::runtime_error("Failed to listen on socket");
	}
	setNonBlocking(listenFd);
	return (listenFd);
}

void SocketUtils::setNonBlocking(int fd) {
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("Failed to set non-blocking flag");
}

time_t SocketUtils::getCurrentTime() {
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec;
}
