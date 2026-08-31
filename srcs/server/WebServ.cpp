#include "WebServ.hpp"
#include "SocketUtils.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "CgiHandler.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>
#include <signal.h>
#include <errno.h>

bool WebServ::isRunning = true;

void WebServ::signalHandler(int) {
	isRunning = false;
}

WebServ::WebServ(Config &c) : config(c) {}

WebServ::~WebServ() {
	for (std::map<int, Client *>::iterator it = clients.begin(); it != clients.end(); ++it) {
		close(it->first);
		delete it->second;
	}
	for (std::map<int, int>::iterator it = serverSockets.begin(); it != serverSockets.end(); ++it) {
		close(it->first);
	}
}

void WebServ::setPollEvent(int fd, short event) {
	for (size_t i = 0; i < pollfds.size(); ++i) {
		if (pollfds[i].fd == fd) {
			pollfds[i].events = event;
			return;
		}
	}
}

void WebServ::addPollFd(int fd, short events) {
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;
	pollfds.push_back(pfd);
}

void WebServ::removePollFd(int fd) {
	for (std::vector<struct pollfd>::iterator it = pollfds.begin(); it != pollfds.end(); ++it) {
		if (it->fd == fd) {
			it->fd = -1;
			return;
		}
	}
}

void WebServ::setupServers() {
	const std::vector<ServerConfig> &servers = config.getServers();
	for (size_t i = 0; i < servers.size(); ++i) {
		int port = servers[i].getListenPort();
		try {
			int listenFd = SocketUtils::createListeningSocket(port);
			serverSockets[listenFd] = port;
			addPollFd(listenFd, POLLIN);
			std::cout << "Server listening on port " << port << std::endl;
		}
		catch(const std::exception &e) {
			std::cerr << "Error setting up server on port " << port << ": " << e.what() << std::endl;
		}
	}
}

void WebServ::run() {
	while (isRunning) {
		for (std::vector<struct pollfd>::iterator it = pollfds.begin(); it != pollfds.end(); ) {
			if (it->fd == -1)
				it = pollfds.erase(it);
			else
				++it;
		}
		int ret = poll(pollfds.empty() ? NULL : &pollfds[0], pollfds.size(), 1000);
		if (ret < 0) {
			break;
		}
		for (std::map<int, Client *>::iterator it = clients.begin(); it != clients.end(); ++it) {
			Client *client = it->second;
			if (client->cgiRunning && client->isCgiTimedOut()) {
				int clientFd = it->first;
				kill(client->cgiPid, SIGKILL);
				int status;
				waitpid(client->cgiPid, &status, 0);
				HttpResponse res;
				client->setResponse(res.getErrorResponse(504));
				setPollEvent(clientFd, POLLOUT);
				client->cgiRunning = false;
				client->cgiPid = -1;
				if (client->cgiPipeOut != -1) { close(client->cgiPipeOut); client->cgiPipeOut = -1; }
				if (client->cgiTmpFd != -1) { close(client->cgiTmpFd); client->cgiTmpFd = -1; }
				std::map<int, int>::iterator pipeIt = cgiPipeToClient.begin();
				while (pipeIt != cgiPipeToClient.end()) {
					if (pipeIt->second == clientFd) {
						removePollFd(pipeIt->first);
						close(pipeIt->first);
						cgiPipeToClient.erase(pipeIt++);
					}
					else {
						++pipeIt;
					}
				}
			}
		}
		size_t size = pollfds.size();
		for (size_t i = 0; i < size; ++i) {
			if (pollfds[i].revents == 0 || pollfds[i].fd == -1)
				continue;
			int fd = pollfds[i].fd;
			if (serverSockets.find(fd) != serverSockets.end()) {
				if (pollfds[i].revents & POLLIN)
					acceptNewConnection(fd);
			}
			else if (cgiPipeToClient.find(fd) != cgiPipeToClient.end()) {
				int clientFd = cgiPipeToClient[fd];
				if (clients.find(clientFd) == clients.end()) {
					cgiPipeToClient.erase(fd);
					removePollFd(fd);
					close(fd);
					continue;
				}
				Client *client = clients[clientFd];
				if (fd == client->cgiPipeOut) {
					if (pollfds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
						handleCgiRead(fd);
					}
				}
			}
			else {
				if (pollfds[i].revents & POLLIN) {
					handleClientRead(fd);
				}
				else if (pollfds[i].revents & POLLOUT) {
					handleClientWrite(fd);
				}
				else if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
					removeClient(fd);
				}
			}
		}
	}
}

void WebServ::acceptNewConnection(int serverFd) {
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	int clientFd = accept(serverFd, reinterpret_cast<struct sockaddr *>(&clientAddr), &clientLen);
	if (clientFd >= 0) {
		SocketUtils::setNonBlocking(clientFd);
		int bufferSize = 1024 * 1024;
		setsockopt(clientFd, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
		setsockopt(clientFd, SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize));
		clients[clientFd] = new Client(clientFd, serverFd);
		addPollFd(clientFd, POLLIN);
	}
}

void WebServ::handleClientRead(int clientFd) {
	char buffer[65536];
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesRead > 0) {
		Client *client = clients[clientFd];
		client->appendRequest(buffer, bytesRead);
		client->isRequestComplete();
		size_t maxBody = config.getServers()[0].getClientMaxBodySize();
		if (client->isRequestTooLarge(maxBody)) {
			HttpResponse res;
			client->setResponse(res.getErrorResponse(413));
			setPollEvent(clientFd, POLLOUT);
			return;
		}
		if (!client->isRequestComplete() && client->isHeadersParsed() && client->getNeeds100Continue() && !client->getHasSent100Continue()) {
			std::string continueMsg = "HTTP/1.1 100 Continue\r\n\r\n";
			client->setResponse(continueMsg);
			client->sending100Continue = true;
			client->setHasSent100Continue(true);
			setPollEvent(clientFd, POLLIN | POLLOUT);
			return;
		}
		if (client->isRequestComplete()) {
			HttpRequest req(client->getRequestBuffer());
			client->clearRequestBuffer();
			if (!req.isValid()) {
				HttpResponse res;
				client->setResponse(res.getErrorResponse(400));
				setPollEvent(clientFd, POLLOUT);
				return;
			}
			HttpResponse res;
			const ServerConfig &server = getServerForRequest(req, client->getServerFd());
			std::string reqBodyPathStr = client->getReqTmpPath() ? client->getReqTmpPath() : "";
			res.generateResponse(req, server, this->sessions, reqBodyPathStr, client->getReqBodyWritten());
			if (res.needsCgiExec()) {
				size_t bodySize = client->getReqBodyWritten();
				std::string reqBodyPath = client->getReqTmpPath() ? client->getReqTmpPath() : "";
				CgiProcess proc = CgiHandler::startCgi(res.getCgiExecPath(), res.getCgiScriptPath(), req, reqBodyPath, bodySize);
				if (!proc.valid) {
					HttpResponse errRes;
					client->setResponse(errRes.getErrorResponse(500));
					setPollEvent(clientFd, POLLOUT);
					return;
				}
				client->cgiRunning = true;
				client->cgiPid = proc.pid;
				client->cgiPipeOut = proc.pipeOut;
				client->cgiTmpFd = proc.tmpFd;
				client->cgiStartTime = SocketUtils::getCurrentTime();
				std::memcpy(client->cgiTmpPath, proc.tmpPath, sizeof(proc.tmpPath));
				client->cgiTotalOutput = 0;
				cgiPipeToClient[client->cgiPipeOut] = clientFd;
				addPollFd(client->cgiPipeOut, POLLIN);
			}
			else {
				std::string responseString = res.buildResponseString();
				if (res.getIsFileResponse()) {
					std::string header = res.buildResponseHeader();
					client->setFileResponse(header, res.getFilePath(), res.getFileSize(), 0, false);
				}
				else
					client->setResponse(responseString);
				setPollEvent(clientFd, POLLOUT);
			}
		}
	}
	else if (bytesRead == 0)
		removeClient(clientFd);
	else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
		removeClient(clientFd);
}



void WebServ::handleCgiRead(int pipeFd) {
	int clientFd = cgiPipeToClient[pipeFd];
	Client *client = clients[clientFd];
	char buffer[65536];
	ssize_t bytesRead = read(pipeFd, buffer, sizeof(buffer));
	if (bytesRead > 0) {
		ssize_t w = write(client->cgiTmpFd, buffer, bytesRead);
		if (w > 0)
			client->cgiTotalOutput += w;
	}
	else if (bytesRead == 0 || (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
		cgiPipeToClient.erase(pipeFd);
		removePollFd(pipeFd);
		close(pipeFd);
		client->cgiPipeOut = -1;
		finishCgi(clientFd);
	}
}

void WebServ::finishCgi(int clientFd) {
	Client *client = clients[clientFd];
	int status;
	waitpid(client->cgiPid, &status, 0);
	client->cgiPid = -1;

	bool cgiCrashed = (WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) != 0));

	CgiResult result = CgiHandler::finishCgi(client->cgiTmpFd, client->cgiTmpPath, client->cgiTotalOutput);
	client->cgiTmpFd = -1;
	client->cgiTmpPath[0] = '\0';
	client->cgiRunning = false;
	
	if (cgiCrashed) {
		HttpResponse errRes;
		client->setResponse(errRes.getErrorResponse(500));
	}
	else if (result.success && result.useFile) {
		std::ostringstream header;
		header << "HTTP/1.1 " << result.statusCode << " " << result.statusMessage << "\r\n";
		header << "Content-Type: " << result.contentType << "\r\n";
		header << "Content-Length: " << result.bodySize << "\r\n";
		header << "Connection: close\r\n";
		header << "\r\n";
		client->setFileResponse(header.str(), result.tmpFilePath, result.bodySize, result.fileStartOffset, true);
	}
	else {
		HttpResponse errRes;
		client->setResponse(errRes.getErrorResponse(500));
	}
	setPollEvent(clientFd, POLLOUT);
}

void WebServ::handleClientWrite(int clientFd) {
	Client *client = clients[clientFd];
	size_t remaining = client->getResponseRemaining();
	if (remaining == 0) {
		if (client->sending100Continue) {
			client->sending100Continue = false;
			client->setResponse("");
			setPollEvent(clientFd, POLLIN);
			return;
		}
		removeClient(clientFd);
		return;
	}
	char sendBuf[65536];
	size_t toRead = remaining < sizeof(sendBuf) ? remaining : sizeof(sendBuf);
	ssize_t dataLen = client->getResponseData(sendBuf, toRead);
	if (dataLen <= 0) {
		removeClient(clientFd);
		return;
	}
	ssize_t bytesSent = send(clientFd, sendBuf, dataLen, 0);
	if (bytesSent > 0) {
		client->advanceResponseOffset(bytesSent);
		if (client->isResponseComplete()) {
			if (client->sending100Continue) {
				client->sending100Continue = false;
				client->setResponse("");
				setPollEvent(clientFd, POLLIN);
			}
			else
				removeClient(clientFd);
		}
	}
	else if (bytesSent < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
		removeClient(clientFd);
}

void WebServ::removeClient(int clientFd) {
	std::map<int, Client *>::iterator it = clients.find(clientFd);
	if (it != clients.end()) {
		Client *client = it->second;
		if (client->cgiPipeOut >= 0) {
			cgiPipeToClient.erase(client->cgiPipeOut);
			removePollFd(client->cgiPipeOut);
		}
		if (client->cgiPid > 0) {
			int status;
			kill(client->cgiPid, SIGKILL);
			waitpid(client->cgiPid, &status, 0);
		}
		delete client;
		clients.erase(it);
	}
	removePollFd(clientFd);
	close(clientFd);
}

const ServerConfig &WebServ::getServerForRequest(const HttpRequest &req, int serverFd) {
	int port = 0;
	std::map<int, int>::const_iterator sIt = serverSockets.find(serverFd);
	if (sIt != serverSockets.end())
		port = sIt->second;

	const std::map<std::string, std::string> &headers = req.getHeaders();
	std::string host = "";
	std::map<std::string, std::string>::const_iterator hostIt = headers.find("Host");
	if (hostIt != headers.end()) {
		host = hostIt->second;
		size_t colonPos = host.find(':');
		if (colonPos != std::string::npos)
			host = host.substr(0, colonPos);
	}

	const std::vector<ServerConfig> &servers = config.getServers();
	int defaultServerIdx = -1;
	for (size_t i = 0; i < servers.size(); ++i) {
		if (servers[i].getListenPort() == port) {
			if (defaultServerIdx == -1)
				defaultServerIdx = i;
			if (!host.empty() && servers[i].getServerName() == host) {
				return (servers[i]);
			}
		}
	}
	if (defaultServerIdx != -1)
		return (servers[defaultServerIdx]);
	return (servers[0]);
}
