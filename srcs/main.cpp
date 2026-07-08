#include "Config.hpp"
#include "WebServ.hpp"
#include <iostream>
#include <exception>
#include <csignal>

int main(int argc, char **argv) {
	signal(SIGINT, WebServ::signalHandler);
	signal(SIGPIPE, SIG_IGN);
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return (1);
	}
	try {
		Config config;
		config.load(argv[1]);
		WebServ webserv(config);
		webserv.setupServers();
		std::cout << "WebServ is running..." << std::endl;
		webserv.run();
	}
	catch(const std::exception &e) {
		std::cerr << "Fatal Error: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
