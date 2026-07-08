#ifndef CGIHANDLER_HPP
# define CGIHANDLER_HPP

# include "HttpRequest.hpp"
# include <string>
# include <vector>
# include <sys/types.h>

struct CgiResult {
	std::string statusCode;
	std::string statusMessage;
	std::string contentType;
	std::string tmpFilePath;
	size_t bodySize;
	size_t fileStartOffset;
	bool success;
	bool useFile;
};

struct CgiProcess {
	pid_t pid;
	int pipeIn;
	int pipeOut;
	int tmpFd;
	char tmpPath[64];
	bool valid;
};

class CgiHandler {
public:
	CgiHandler();
	~CgiHandler();

	static CgiProcess startCgi(const std::string &cgiPath, const std::string &scriptFile,
		const HttpRequest &req);
	static CgiResult finishCgi(int tmpFd, const char *tmpPath, size_t totalOutput);
	static void buildEnvp(const HttpRequest &req, const std::string &scriptFile,
		std::vector<char *> &envp);
	static void freeEnvp(std::vector<char *> &envp);

private:
	CgiHandler(const CgiHandler &other);
	CgiHandler &operator=(const CgiHandler &other);
};

#endif
