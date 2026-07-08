#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include "LocationConfig.hpp"
# include <string>
# include <vector>
# include <map>

class ServerConfig {
public:
	ServerConfig();
	ServerConfig(const ServerConfig &other);
	ServerConfig &operator=(const ServerConfig &other);
	~ServerConfig();

	int getListenPort() const;
	const std::string &getServerName() const;
	const std::map<int, std::string> &getErrorPages() const;
	size_t getClientMaxBodySize() const;
	const std::string &getRoot() const;
	const std::string &getIndex() const;
	const std::vector<LocationConfig> &getLocations() const;

	void setListenPort(int port);
	void setServerName(const std::string &name);
	void addErrorPage(int code, const std::string &path);
	void setClientMaxBodySize(size_t size);
	void setRoot(const std::string &rootDir);
	void setIndex(const std::string &indexFile);
	void addLocation(const LocationConfig &location);

private:
	int listenPort;
	std::string serverName;
	std::map<int, std::string> errorPages;
	size_t clientMaxBodySize;
	std::string root;
	std::string index;
	std::vector<LocationConfig> locations;
};

#endif
