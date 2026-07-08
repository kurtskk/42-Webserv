#include "ServerConfig.hpp"

ServerConfig::ServerConfig() :
	listenPort(8080),
	serverName(""),
	clientMaxBodySize(1048576),
	root(""), index("")
	{}

ServerConfig::ServerConfig(const ServerConfig &other) {
	*this = other;
}

ServerConfig &ServerConfig::operator=(const ServerConfig &other) {
	if (this != &other) {
		listenPort = other.listenPort;
		serverName = other.serverName;
		errorPages = other.errorPages;
		clientMaxBodySize = other.clientMaxBodySize;
		root = other.root;
		index = other.index;
		locations = other.locations;
	}
	return (*this);
}

ServerConfig::~ServerConfig() {}

int ServerConfig::getListenPort() const {
	return (listenPort);
}

const std::string &ServerConfig::getServerName() const {
	return (serverName);
}

const std::map<int, std::string> &ServerConfig::getErrorPages() const {
	return (errorPages);
}

size_t ServerConfig::getClientMaxBodySize() const {
	return (clientMaxBodySize);
}

const std::string &ServerConfig::getRoot() const {
	return (root);
}

const std::string &ServerConfig::getIndex() const {
	return (index);
}

const std::vector<LocationConfig> &ServerConfig::getLocations() const {
	return (locations);
}

void ServerConfig::setListenPort(int port) {
	listenPort = port;
}

void ServerConfig::setServerName(const std::string &name) {
	serverName = name;
}

void ServerConfig::addErrorPage(int code, const std::string &path) {
	errorPages[code] = path;
}

void ServerConfig::setClientMaxBodySize(size_t size) {
	clientMaxBodySize = size;
}

void ServerConfig::setRoot(const std::string &rootDir) {
	root = rootDir;
}

void ServerConfig::setIndex(const std::string &indexFile) {
	index = indexFile;
}

void ServerConfig::addLocation(const LocationConfig &location) {
	locations.push_back(location);
}
