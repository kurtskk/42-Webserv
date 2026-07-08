#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include "Config.hpp"
# include "Client.hpp"
# include <vector>
# include <map>
# include <poll.h>

class HttpRequest;

class WebServ {
public:
	WebServ(Config &config);
	~WebServ();

	static bool isRunning;
	static void signalHandler(int signum);

	void setupServers();
	void run();

private:
	Config &config;
	std::vector<struct pollfd> pollfds;
	std::map<int, int> serverSockets;
	std::map<int, Client *> clients;
	std::map<int, int> cgiPipeToClient;
	std::map<std::string, std::map<std::string, std::string> > sessions;

	void acceptNewConnection(int serverFd);
	void handleClientRead(int clientFd);
	void handleClientWrite(int clientFd);
	void handleCgiRead(int pipeFd);
	void handleCgiWrite(int pipeFd);
	void finishCgi(int clientFd);
	void removeClient(int clientFd);
	void setPollEvent(int fd, short event);
	void addPollFd(int fd, short events);
	void removePollFd(int fd);

	WebServ(const WebServ &other);
	WebServ &operator=(const WebServ &other);

	const ServerConfig &getServerForRequest(const HttpRequest &req, int serverFd);
};

#endif
