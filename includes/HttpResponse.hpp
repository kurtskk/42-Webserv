#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

# include "HttpRequest.hpp"
# include "Config.hpp"
# include <string>
# include <vector>
# include <map>

class HttpResponse {
public:
	HttpResponse();
	~HttpResponse();

	std::string generateResponse(const HttpRequest &req, const ServerConfig &serverConfig, std::map<std::string, std::map<std::string, std::string> > &sessions);
	std::string buildResponseString() const;
	std::string buildResponseHeader() const;
	std::string getErrorResponse(int code, const ServerConfig *srv = NULL);

	bool getIsFileResponse() const;
	const std::string &getFilePath() const;
	size_t getFileSize() const;

	bool needsCgiExec() const;
	const std::string &getCgiScriptPath() const;
	const std::string &getCgiExecPath() const;

	void addCookie(const std::string &cookie);
	void setSessionData(std::map<std::string, std::map<std::string, std::string> > &sessions, const std::string &sessionId, const std::string &key, const std::string &value);
	const std::string &getSessionData(const std::map<std::string, std::map<std::string, std::string> > &sessions, const std::string &sessionId, const std::string &key) const;

private:
	std::string statusCode;
	std::string statusMessage;
	std::string body;
	std::string contentType;
	std::string redirectUrl;
	std::vector<std::string> cookies;

	bool isFileResponse;
	std::string filePath;
	size_t fileSize;

	bool needsCgi;
	std::string cgiScriptPath;
	std::string cgiExecPath;

	void handleGet(const HttpRequest &req, const ServerConfig &serverConfig, const std::string &targetFile, const LocationConfig *matchedLoc);
	void handlePost(const HttpRequest &req, const ServerConfig &serverConfig, const std::string &targetFile, const LocationConfig *matchedLoc);
	void handleDelete(const HttpRequest &req, const ServerConfig &serverConfig, const std::string &targetFile);
	void setErrorResponse(int code, const ServerConfig *srv = NULL);

	HttpResponse(const HttpResponse &other);
	HttpResponse &operator=(const HttpResponse &other);
};

#endif
