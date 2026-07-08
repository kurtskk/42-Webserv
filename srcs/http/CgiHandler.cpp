#include "CgiHandler.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cstring>
#include <sstream>

CgiHandler::CgiHandler() {}
CgiHandler::~CgiHandler() {}

void CgiHandler::freeEnvp(std::vector<char *> &envp) {
	for (size_t i = 0; i < envp.size(); ++i) {
		if (envp[i])
			delete[] envp[i];
	}
	envp.clear();
}

static bool pushEnv(std::vector<char *> &envp, const std::string &str) {
	char *dup = new char[str.length() + 1];
	if (!dup)
		return (false);
	std::strcpy(dup, str.c_str());
	envp.push_back(dup);
	return (true);
}

void CgiHandler::buildEnvp(const HttpRequest &req, const std::string &scriptFile, std::vector<char *> &envp) {
	const std::string &body = req.getBody();
	std::string methodEnv     = "REQUEST_METHOD=" + req.getMethod();
	std::string protocolEnv   = "SERVER_PROTOCOL=HTTP/1.1";
	std::string pathInfoEnv   = "PATH_INFO=" + req.getUri();
	std::string gatewayEnv    = "GATEWAY_INTERFACE=CGI/1.1";
	std::string redirectEnv   = "REDIRECT_STATUS=200";
	std::string serverNameEnv = "SERVER_NAME=localhost";
	std::string scriptNameEnv = "SCRIPT_FILENAME=" + scriptFile;
	std::ostringstream cl;
	cl << "CONTENT_LENGTH=" << body.length();
	std::string contentLengthEnv = cl.str();
	std::string contentTypeEnv = "CONTENT_TYPE=application/octet-stream";
	const std::map<std::string, std::string> &headers = req.getHeaders();
	std::map<std::string, std::string>::const_iterator ctIt = headers.find("Content-Type");
	if (ctIt == headers.end())
		ctIt = headers.find("Content-type");
	if (ctIt != headers.end())
		contentTypeEnv = "CONTENT_TYPE=" + ctIt->second;
	std::string queryStringEnv = "QUERY_STRING=";
	size_t qPos = req.getUri().find('?');
	if (qPos != std::string::npos)
		queryStringEnv = "QUERY_STRING=" + req.getUri().substr(qPos + 1);
	if (!pushEnv(envp, methodEnv)      || !pushEnv(envp, protocolEnv)    ||
		!pushEnv(envp, pathInfoEnv)    || !pushEnv(envp, contentLengthEnv) ||
		!pushEnv(envp, contentTypeEnv) || !pushEnv(envp, gatewayEnv)     ||
		!pushEnv(envp, redirectEnv)    || !pushEnv(envp, serverNameEnv)  ||
		!pushEnv(envp, scriptNameEnv)  || !pushEnv(envp, queryStringEnv)) {
		freeEnvp(envp);
		return;
	}
	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
		it != headers.end(); ++it) {
		std::string key = it->first;
		if (key == "Content-Type" || key == "Content-type" ||
			key == "Content-Length" || key == "Content-length" ||
			key == "Transfer-Encoding" || key == "transfer-encoding")
			continue;
		std::string envName = "HTTP_";
		for (size_t i = 0; i < key.length(); ++i) {
			char c = key[i];
			if (c == '-')
				envName += '_';
			else if (c >= 'a' && c <= 'z')
				envName += (c - 'a' + 'A');
			else
				envName += c;
		}
		if (!pushEnv(envp, envName + "=" + it->second)) {
			freeEnvp(envp);
			return;
		}
	}
	envp.push_back(NULL);
}

CgiProcess CgiHandler::startCgi(const std::string &cgiPath, const std::string &scriptFile,
	const HttpRequest &req) {
	CgiProcess proc;
	proc.valid = false;
	proc.pid = -1;
	proc.pipeIn = -1;
	proc.pipeOut = -1;
	proc.tmpFd = -1;
	std::memset(proc.tmpPath, 0, sizeof(proc.tmpPath));
	int pipeIn[2];
	int pipeOut[2];
	if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0)
		return proc;
	std::vector<char *> envp;
	buildEnvp(req, scriptFile, envp);
	if (envp.empty()) {
		close(pipeIn[0]); close(pipeIn[1]);
		close(pipeOut[0]); close(pipeOut[1]);
		return (proc);
	}
	{
		static unsigned int tmpCounter = 0;
		++tmpCounter;
		std::ostringstream tmpName;
		tmpName << "/tmp/webserv_cgi_" << getpid() << "_" << static_cast<long>(time(NULL)) << "_" << tmpCounter;
		std::strncpy(proc.tmpPath, tmpName.str().c_str(), sizeof(proc.tmpPath) - 1);
	}
	proc.tmpFd = open(proc.tmpPath, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (proc.tmpFd < 0) {
		close(pipeIn[0]); close(pipeIn[1]);
		close(pipeOut[0]); close(pipeOut[1]);
		freeEnvp(envp);
		return (proc);
	}
	pid_t pid = fork();
	if (pid < 0) {
		close(pipeIn[0]); close(pipeIn[1]);
		close(pipeOut[0]); close(pipeOut[1]);
		close(proc.tmpFd);
		unlink(proc.tmpPath);
		freeEnvp(envp);
		return (proc);
	}
	if (pid == 0) {
		close(pipeIn[1]);
		close(pipeOut[0]);
		dup2(pipeIn[0], STDIN_FILENO);
		dup2(pipeOut[1], STDOUT_FILENO);
		close(pipeIn[0]);
		close(pipeOut[1]);
		char *arg0 = new char[cgiPath.length() + 1];
		std::strcpy(arg0, cgiPath.c_str());
		char *arg1 = new char[scriptFile.length() + 1];
		std::strcpy(arg1, scriptFile.c_str());
		char *argv[] = { arg0, arg1, NULL };
		execve(cgiPath.c_str(), argv, &envp[0]);
		write(STDOUT_FILENO, "Status: 500\r\n\r\n", 15);
		delete[] arg0;
		delete[] arg1;
		freeEnvp(envp);
		_exit(1);
	}
	close(pipeIn[0]);
	close(pipeOut[1]);
	freeEnvp(envp);
	fcntl(pipeIn[1], F_SETFL, O_NONBLOCK);
	fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);
	proc.pid = pid;
	proc.pipeIn = pipeIn[1];
	proc.pipeOut = pipeOut[0];
	proc.valid = true;
	return (proc);
}

static std::string trimTrailingCRLF(const std::string &value) {
	std::string result = value;
	while (!result.empty() && (result[result.length() - 1] == '\r' || result[result.length() - 1] == '\n'))
		result.erase(result.length() - 1);
	return (result);
}

static void parseCgiHeaders(const std::string &headerChunk, size_t headerEnd,
	std::string &statusCode, std::string &statusMessage, std::string &contentType) {
	std::string cgiHeaders = headerChunk.substr(0, headerEnd);
	std::string lineEnd = "\r\n";
	if (cgiHeaders.find("\r\n") == std::string::npos)
		lineEnd = "\n";
	size_t statusPos = cgiHeaders.find("Status: ");
	if (statusPos != std::string::npos) {
		size_t statusEndPos = cgiHeaders.find(lineEnd, statusPos);
		if (statusEndPos == std::string::npos)
			statusEndPos = cgiHeaders.length();
		std::string statusLine = cgiHeaders.substr(statusPos + 8, statusEndPos - (statusPos + 8));
		size_t spacePos = statusLine.find(' ');
		if (spacePos != std::string::npos) {
			statusCode = statusLine.substr(0, spacePos);
			statusMessage = statusLine.substr(spacePos + 1);
		}
		else {
			statusCode = statusLine;
			statusMessage = "Unknown";
		}
	}
	size_t ctPos = cgiHeaders.find("Content-Type: ");
	if (ctPos == std::string::npos)
		ctPos = cgiHeaders.find("Content-type: ");
	if (ctPos != std::string::npos) {
		size_t ctEnd = cgiHeaders.find(lineEnd, ctPos);
		if (ctEnd == std::string::npos)
			ctEnd = cgiHeaders.length();
		contentType = cgiHeaders.substr(ctPos + 14, ctEnd - (ctPos + 14));
	}
	statusCode = trimTrailingCRLF(statusCode);
	statusMessage = trimTrailingCRLF(statusMessage);
	contentType = trimTrailingCRLF(contentType);
}

CgiResult CgiHandler::finishCgi(int tmpFd, const char *tmpPath, size_t totalOutput) {
	CgiResult result;
	result.statusCode = "200";
	result.statusMessage = "OK";
	result.contentType = "text/html";
	result.bodySize = 0;
	result.fileStartOffset = 0;
	result.success = false;
	result.useFile = false;
	close(tmpFd);
	tmpFd = open(tmpPath, O_RDONLY);
	if (tmpFd < 0) {
		result.statusCode = "500";
		result.statusMessage = "Internal Server Error";
		return (result);
	}
	char headerBuf[8192];
	ssize_t headerBytesRead = read(tmpFd, headerBuf, sizeof(headerBuf));
	if (headerBytesRead <= 0) {
		close(tmpFd);
		result.statusCode = "500";
		result.statusMessage = "Internal Server Error";
		return (result);
	}
	std::string headerChunk(headerBuf, headerBytesRead);
	size_t headerEnd = headerChunk.find("\r\n\r\n");
	size_t headerSepLen = 4;
	if (headerEnd == std::string::npos) {
		headerEnd = headerChunk.find("\n\n");
		headerSepLen = 2;
	}
	size_t bodyOffset = 0;
	if (headerEnd != std::string::npos) {
		parseCgiHeaders(headerChunk, headerEnd, result.statusCode, result.statusMessage, result.contentType);
		bodyOffset = headerEnd + headerSepLen;
	}
	else {
		parseCgiHeaders(headerChunk, headerChunk.size(), result.statusCode, result.statusMessage, result.contentType);
		bodyOffset = totalOutput;
	}
	result.bodySize = totalOutput > bodyOffset ? totalOutput - bodyOffset : 0;
	result.tmpFilePath = tmpPath;
	result.fileStartOffset = bodyOffset;
	result.useFile = true;
	result.success = true;
	close(tmpFd);
	return (result);
}
