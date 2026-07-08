#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>

Config::Config() {}

Config::~Config() {}

void Config::stripComment(std::string &line) {
	size_t pos = line.find('#');
	if (pos != std::string::npos)
		line.erase(pos);
}

std::vector<std::string> Config::tokenize(const std::string &line) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(line);
	while (tokenStream >> token) {
		if (!token.empty() && token[token.length() - 1] == ';') {
			token.erase(token.length() - 1);
		}
		if (!token.empty()) {
			tokens.push_back(token);
		}
	}
	return (tokens);
}

void Config::load(const std::string &filename) {
	std::ifstream file(filename.c_str());
	if (!file.is_open()) {
		throw std::runtime_error("Could not open config file: " + filename);
	}
	std::string line;
	while (std::getline(file, line)) {
		stripComment(line);
		std::vector<std::string> tokens = tokenize(line);
		if (tokens.empty())
			continue;
		if (tokens[0] == "server" && tokens.size() == 2 && tokens[1] == "{") {
			ServerConfig server;
			parseServerBlock(file, server);
			servers.push_back(server);
		}
		else {
			throw std::runtime_error("Invalid config file format. Expected 'server {'");
		}
	}
}

void Config::parseServerBlock(std::ifstream &file, ServerConfig &server) {
	std::string line;
	while (std::getline(file, line)) {
		stripComment(line);
		std::vector<std::string> tokens = tokenize(line);
		if (tokens.empty())
			continue;
		if (tokens[0] == "}") {
			return;
		}
		else if (tokens[0] == "listen" && tokens.size() == 2) {
			int port = parsePort(tokens[1]);
			server.setListenPort(port);
		}
		else if (tokens[0] == "server_name" && tokens.size() == 2) {
			server.setServerName(tokens[1]);
		}
		else if (tokens[0] == "client_max_body_size" && tokens.size() == 2) {
			server.setClientMaxBodySize(parseBodySize(tokens[1]));
		}
		else if (tokens[0] == "root" && tokens.size() == 2) {
			server.setRoot(tokens[1]);
		}
		else if (tokens[0] == "index" && tokens.size() == 2) {
			server.setIndex(tokens[1]);
		}
		else if (tokens[0] == "error_page" && tokens.size() == 3) {
			server.addErrorPage(parseErrorCode(tokens[1]), tokens[2]);
		}
		else if (tokens[0] == "location" && tokens.size() == 3 && tokens[2] == "{") {
			LocationConfig location(tokens[1]);
			parseLocationBlock(file, location);
			server.addLocation(location);
		}
	}
	throw std::runtime_error("Server block not closed properly");
}

void Config::parseLocationBlock(std::ifstream &file, LocationConfig &location) {
	std::string line;
	while (std::getline(file, line)) {
		stripComment(line);
		std::vector<std::string> tokens = tokenize(line);
		if (tokens.empty())
			continue;
		if (tokens[0] == "}") {
			return;
		}
		else if (tokens[0] == "root" && tokens.size() == 2) {
			location.setRoot(tokens[1]);
		}
		else if (tokens[0] == "index" && tokens.size() == 2) {
			location.setIndex(tokens[1]);
		}
		else if (tokens[0] == "autoindex" && tokens.size() == 2) {
			location.setAutoindex(tokens[1] == "on");
		}
		else if (tokens[0] == "allow_methods") {
			for (size_t i = 1; i < tokens.size(); ++i) {
				location.addAllowedMethod(tokens[i]);
			}
		}
		else if (tokens[0] == "cgi_ext" && tokens.size() == 2) {
			location.setCgiExtension(tokens[1]);
		}
		else if (tokens[0] == "cgi_path" && tokens.size() == 2) {
			location.setCgiPath(tokens[1]);
		}
		else if (tokens[0] == "client_max_body_size" && tokens.size() == 2) {
			location.setClientMaxBodySize(parseBodySize(tokens[1]));
		}
		else if (tokens[0] == "upload_store" && tokens.size() == 2) {
			location.setUploadStore(tokens[1]);
		}
		else if (tokens[0] == "return" && tokens.size() == 3) {
			std::istringstream issCode(tokens[1]);
			int code;
			char extra;
			if (!(issCode >> code) || issCode.get(extra))
				throw std::runtime_error("Invalid redirect code: '" + tokens[1] + "'");
			location.setRedirectCode(code);
			location.setRedirectUrl(tokens[2]);
		}
	}
	throw std::runtime_error("Location block not closed properly");
}

size_t Config::parseBodySize(const std::string &value) {
	if (value.empty()) {
		throw std::runtime_error("Invalid body size: empty value");
	}
	size_t i = 0;
	if (value[i] == '-') {
		throw std::runtime_error("Invalid body size: negative values not allowed");
	}
	while (i < value.length() && std::isdigit(value[i])) {
		i++;
	}
	if (i == 0) {
		throw std::runtime_error("Invalid body size: no digits found");
	}
	std::string numStr = value.substr(0, i);
	std::string suffix = value.substr(i);
	if (suffix.length() > 1) {
		throw std::runtime_error("Invalid body size: invalid suffix");
	}
	std::istringstream issNum(numStr);
	unsigned long num;
	if (!(issNum >> num))
		throw std::runtime_error("Invalid body size: not a valid number");
	if (suffix.empty()) {
		return (static_cast<size_t>(num));
	}
	char suffixChar = suffix[0];
	size_t multiplier = 1;
	switch (suffixChar) {
		case 'k':
		case 'K':
			multiplier = 1024;
			break;
		case 'm':
		case 'M':
			multiplier = 1024 * 1024;
			break;
		case 'g':
		case 'G':
			multiplier = 1024 * 1024 * 1024;
			break;
		case 't':
		case 'T':
			multiplier = static_cast<size_t>(1024UL * 1024 * 1024 * 1024);
			break;
		default:
			throw std::runtime_error("Invalid body size: unknown suffix '" + std::string(1, suffixChar) + "'");
	}
	return (static_cast<size_t>(num) * multiplier);
}

int Config::parsePort(const std::string &value) {
	if (value.empty()) {
		throw std::runtime_error("Invalid port: empty value");
	}
	std::istringstream iss(value);
	int port;
	char extra;
	if (!(iss >> port) || iss.get(extra)) {
		throw std::runtime_error("Invalid port: '" + value + "' is not a valid number");
	}
	if (port < 1 || port > 65535) {
		throw std::runtime_error("Invalid port: " + value + " out of range (1-65535)");
	}
	return (port);
}

int Config::parseErrorCode(const std::string &value) {
	if (value.empty()) {
		throw std::runtime_error("Invalid error code: empty value");
	}
	std::istringstream iss(value);
	int code;
	char extra;
	if (!(iss >> code) || iss.get(extra)) {
		throw std::runtime_error("Invalid error code: '" + value + "' is not a valid number");
	}
	if (code < 100 || code > 599) {
		throw std::runtime_error("Invalid error code: " + value + " out of range (100-599)");
	}
	return (code);
}



const std::vector<ServerConfig> &Config::getServers() const {
	return (servers);
}
