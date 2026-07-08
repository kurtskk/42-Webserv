#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "ServerConfig.hpp"
# include <string>
# include <vector>

class Config {
public:
	Config();
	~Config();

	void load(const std::string &filename);
	const std::vector<ServerConfig> &getServers() const;

private:
	std::vector<ServerConfig> servers;
	void stripComment(std::string &line);
	std::vector<std::string> tokenize(const std::string &line);
	void parseServerBlock(std::ifstream &file, ServerConfig &server);
	void parseLocationBlock(std::ifstream &file, LocationConfig &location);
	static size_t parseBodySize(const std::string &value);
	static int parsePort(const std::string &value);
	static int parseErrorCode(const std::string &value);

	Config(const Config &other);
	Config &operator=(const Config &other);
};

#endif
