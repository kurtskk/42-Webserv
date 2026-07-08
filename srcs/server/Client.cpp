#include "Client.hpp"
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <time.h>

Client::Client(int f, int s) :
	sending100Continue(false),
	cgiRunning(false),
	cgiPid(-1),
	cgiPipeIn(-1),
	cgiPipeOut(-1),
	cgiTmpFd(-1),
	cgiBodyWritten(0),
	cgiTotalOutput(0),
	cgiBodyDone(false),
	cgiStartTime(0),
	fd(f),
	serverFd(s),
	responseFileSize(0),
	responseFileFd(-1),
	responseFileStartOffset(0),
	responseSendOffset(0),
	responseHeaderSize(0),
	cacheOffset(0),
	requestComplete(false),
	responseComplete(false),
	fileBasedResponse(false),
	deleteFileOnClose(false),
	headersParsed(false),
	headerEnd(0),
	contentLength(0),
	isChunked(false),
	needs100Continue(false),
	hasSent100Continue(false)
{
	std::memset(cgiTmpPath, 0, sizeof(cgiTmpPath));
}

Client::~Client() {
	if (responseFileFd >= 0)
		close(responseFileFd);
	if (!responseFilePath.empty() && deleteFileOnClose)
		unlink(responseFilePath.c_str());
	if (cgiPipeIn >= 0)
		close(cgiPipeIn);
	if (cgiPipeOut >= 0)
		close(cgiPipeOut);
	if (cgiTmpFd >= 0)
		close(cgiTmpFd);
	if (cgiTmpPath[0])
		unlink(cgiTmpPath);
}

void Client::appendRequest(const char *buffer, ssize_t bytes) {
	requestBuffer.append(buffer, bytes);
}

const std::string &Client::getRequestBuffer() const {
	return (requestBuffer);
}

void Client::clearRequestBuffer() {
	requestBuffer.clear();
	requestComplete = false;
}

void Client::setResponse(const std::string &response) {
	responseBuffer = response;
	responseSendOffset = 0;
	cacheOffset = 0;
	responseComplete = false;
	fileBasedResponse = false;
	responseHeaderSize = 0;
	responseFileSize = 0;
}

void Client::setFileResponse(const std::string &header, const std::string &filePath, size_t fileSize, size_t fileStartOffset, bool deleteFile) {
	responseHeaderSize = header.length();
	responseFilePath = filePath;
	responseFileSize = fileSize;
	responseFileStartOffset = fileStartOffset;
	responseSendOffset = 0;
	cacheOffset = 0;
	responseComplete = false;
	fileBasedResponse = true;
	deleteFileOnClose = deleteFile;
	responseFileFd = open(filePath.c_str(), O_RDONLY);
	if (responseFileFd >= 0 && responseFileStartOffset > 0) {
		char skipBuf[8192];
		size_t toSkip = responseFileStartOffset;
		while (toSkip > 0) {
			size_t readSize = toSkip > sizeof(skipBuf) ? sizeof(skipBuf) : toSkip;
			ssize_t ret = read(responseFileFd, skipBuf, readSize);
			if (ret <= 0) break;
			toSkip -= ret;
		}
	}
	responseBuffer = header;
}

size_t Client::getResponseRemaining() const {
	size_t total;
	if (fileBasedResponse)
		total = responseHeaderSize + responseFileSize;
	else
		total = responseBuffer.length();
	if (responseSendOffset >= total)
		return (0);
	return (total - responseSendOffset);
}

void Client::advanceResponseOffset(ssize_t bytes) {
	responseSendOffset += bytes;
	if (fileBasedResponse)
		cacheOffset += bytes;
	size_t total;
	if (fileBasedResponse)
		total = responseHeaderSize + responseFileSize;
	else
		total = responseBuffer.length();
	if (responseSendOffset >= total)
		responseComplete = true;
}

ssize_t Client::getResponseData(char *buf, size_t maxLen) {
	if (!fileBasedResponse) {
		size_t remaining = responseBuffer.length() - responseSendOffset;
		if (remaining == 0)
			return (0);
		size_t toSend = remaining < maxLen ? remaining : maxLen;
		std::memcpy(buf, responseBuffer.c_str() + responseSendOffset, toSend);
		return (toSend);
	}
	size_t cachedRemaining = responseBuffer.length() - cacheOffset;
	if (cachedRemaining == 0) {
		responseBuffer.clear();
		cacheOffset = 0;
		if (responseFileFd >= 0) {
			char readBuf[65536];
			ssize_t bytesRead = read(responseFileFd, readBuf, sizeof(readBuf));
			if (bytesRead > 0) {
				responseBuffer.assign(readBuf, bytesRead);
				cachedRemaining = bytesRead;
			}
		}
	}
	if (cachedRemaining == 0)
		return (0);
	size_t toSend = cachedRemaining < maxLen ? cachedRemaining : maxLen;
	std::memcpy(buf, responseBuffer.c_str() + cacheOffset, toSend);
	return (toSend);
}

static size_t findHeaderValue(const std::string &buf, size_t headerEnd, const std::string &key) {
	size_t pos = buf.find(key);
	if (pos != std::string::npos && pos < headerEnd)
		return (pos);
	return (std::string::npos);
}

bool Client::isRequestComplete() {
	if (requestComplete)
		return (true);
	if (!headersParsed) {
		headerEnd = requestBuffer.find("\r\n\r\n");
		if (headerEnd == std::string::npos) {
			return (false);
		}
		headersParsed = true;
		size_t clPos = findHeaderValue(requestBuffer, headerEnd, "Content-Length: ");
		if (clPos == std::string::npos)
			clPos = findHeaderValue(requestBuffer, headerEnd, "Content-length: ");
		if (clPos == std::string::npos)
			clPos = findHeaderValue(requestBuffer, headerEnd, "content-length: ");
		if (clPos != std::string::npos) {
			size_t valueStart = clPos + 16;
			size_t valueEnd = requestBuffer.find("\r\n", valueStart);
			if (valueEnd != std::string::npos && valueEnd < headerEnd) {
				std::string clStr = requestBuffer.substr(valueStart, valueEnd - valueStart);
				size_t trimStart = clStr.find_first_not_of(" \t");
				size_t trimEnd = clStr.find_last_not_of(" \t");
				if (trimStart != std::string::npos)
					clStr = clStr.substr(trimStart, trimEnd - trimStart + 1);
				std::istringstream clStream(clStr);
				if (!(clStream >> contentLength))
					contentLength = 0;
			}
		}
		if (requestBuffer.find("Transfer-Encoding: chunked") != std::string::npos ||
			requestBuffer.find("transfer-encoding: chunked") != std::string::npos) {
			isChunked = true;
		}
		size_t expectPos = requestBuffer.find("Expect: 100-continue");
		if (expectPos == std::string::npos)
			expectPos = requestBuffer.find("Expect: 100-Continue");
		if (expectPos != std::string::npos && expectPos < headerEnd)
			needs100Continue = true;
	}
	if (headersParsed) {
		if (contentLength > 0) {
			if (requestBuffer.length() >= headerEnd + 4 + contentLength) {
				requestComplete = true;
				return (true);
			}
			return (false);
		}
		if (isChunked) {
			if (requestBuffer.length() >= 5) {
				const char* buf = requestBuffer.c_str();
				size_t len = requestBuffer.length();
				if (buf[len-5] == '0' && buf[len-4] == '\r' && buf[len-3] == '\n' && buf[len-2] == '\r' && buf[len-1] == '\n') {
					requestComplete = true;
					return (true);
				}
			}
			return (false);
		}
		requestComplete = true;
		return (true);
	}
	return (false);
}

bool Client::isHeadersParsed() const {
	return (headersParsed);
}

bool Client::getNeeds100Continue() const {
	return (needs100Continue);
}

bool Client::getHasSent100Continue() const {
	return (hasSent100Continue);
}

void Client::setHasSent100Continue(bool status) {
	hasSent100Continue = status;
}

bool Client::isResponseComplete() const {
	return (responseComplete);
}

bool Client::isCgiTimedOut() const {
	if (!cgiRunning || cgiStartTime == 0)
		return (false);
	time_t now = time(NULL);
	return ((now - cgiStartTime) > CGI_TIMEOUT);
}

int Client::getServerFd() const {
	return (serverFd);
}

bool Client::isRequestTooLarge(size_t maxBodySize) const {
	if (headersParsed && contentLength > maxBodySize) {
		return (true);
	}
	return (requestBuffer.length() > maxBodySize + 16384);
}
