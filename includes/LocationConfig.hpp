#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP

# include <string>
# include <vector>

class LocationConfig {
public:
	LocationConfig();
	LocationConfig(const std::string &locationPath);
	LocationConfig(const LocationConfig &other);
	LocationConfig &operator=(const LocationConfig &other);
	~LocationConfig();

	const std::string &getPath() const;
	const std::string &getRoot() const;
	const std::string &getIndex() const;
	bool getAutoindex() const;
	const std::vector<std::string> &getAllowedMethods() const;
	const std::string &getCgiExtension() const;
	const std::string &getCgiPath() const;
	size_t getClientMaxBodySize() const;
	const std::string &getUploadStore() const;
	int getRedirectCode() const;
	const std::string &getRedirectUrl() const;

	void setRoot(const std::string &rootDir);
	void setIndex(const std::string &indexFile);
	void setAutoindex(bool isAutoindex);
	void addAllowedMethod(const std::string &method);
	void setCgiExtension(const std::string &ext);
	void setCgiPath(const std::string &cgiExecutablePath);
	void setClientMaxBodySize(size_t size);
	void setUploadStore(const std::string &uploadPath);
	void setRedirectCode(int code);
	void setRedirectUrl(const std::string &url);

private:
	std::string path;
	std::string root;
	std::string index;
	bool autoindex;
	std::vector<std::string> allowedMethods;
	std::string cgiExtension;
	std::string cgiPath;
	size_t clientMaxBodySize;
	std::string uploadStore;
	int redirectCode;
	std::string redirectUrl;
};

#endif
