#include "LocationConfig.hpp"

LocationConfig::LocationConfig() :
	path(""),
	root(""),
	index(""),
	autoindex(false),
	cgiExtension(""),
	cgiPath(""),
	clientMaxBodySize(0),
	uploadStore(""),
	redirectCode(0),
	redirectUrl("")
	{}

LocationConfig::LocationConfig(const std::string &locationPath) :
	path(locationPath),
	root(""),
	index(""),
	autoindex(false),
	cgiExtension(""),
	cgiPath(""),
	clientMaxBodySize(0),
	uploadStore(""),
	redirectCode(0),
	redirectUrl("")
	{}

LocationConfig::LocationConfig(const LocationConfig &other) {
	*this = other;
}

LocationConfig &LocationConfig::operator=(const LocationConfig &other) {
	if (this != &other) {
		path = other.path;
		root = other.root;
		index = other.index;
		autoindex = other.autoindex;
		allowedMethods = other.allowedMethods;
		cgiExtension = other.cgiExtension;
		cgiPath = other.cgiPath;
		clientMaxBodySize = other.clientMaxBodySize;
		uploadStore = other.uploadStore;
		redirectCode = other.redirectCode;
		redirectUrl = other.redirectUrl;
	}
	return (*this);
}

LocationConfig::~LocationConfig() {}

const std::string &LocationConfig::getPath() const {
	return (path);
}

const std::string &LocationConfig::getRoot() const {
	return (root);
}

const std::string &LocationConfig::getIndex() const {
	return (index);
}

bool LocationConfig::getAutoindex() const {
	return (autoindex);
}

const std::vector<std::string> &LocationConfig::getAllowedMethods() const {
	return (allowedMethods);
}

const std::string &LocationConfig::getCgiExtension() const {
	return (cgiExtension);
}

const std::string &LocationConfig::getCgiPath() const {
	return (cgiPath);
}

size_t LocationConfig::getClientMaxBodySize() const {
	return (clientMaxBodySize);
}

const std::string &LocationConfig::getUploadStore() const {
	return (uploadStore);
}

void LocationConfig::setRoot(const std::string &rootDir) {
	root = rootDir;
}

void LocationConfig::setIndex(const std::string &indexFile) {
	index = indexFile;
}

void LocationConfig::setAutoindex(bool isAutoindex) {
	autoindex = isAutoindex;
}

void LocationConfig::addAllowedMethod(const std::string &method) {
	allowedMethods.push_back(method);
}

void LocationConfig::setCgiExtension(const std::string &ext) {
	cgiExtension = ext;
}

void LocationConfig::setCgiPath(const std::string &cgiExecutablePath) {
	cgiPath = cgiExecutablePath;
}

void LocationConfig::setClientMaxBodySize(size_t size) {
	clientMaxBodySize = size;
}

void LocationConfig::setUploadStore(const std::string &uploadPath) {
	uploadStore = uploadPath;
}

int LocationConfig::getRedirectCode() const {
	return (redirectCode);
}

const std::string &LocationConfig::getRedirectUrl() const {
	return (redirectUrl);
}

void LocationConfig::setRedirectCode(int code) {
	redirectCode = code;
}

void LocationConfig::setRedirectUrl(const std::string &url) {
	redirectUrl = url;
}
