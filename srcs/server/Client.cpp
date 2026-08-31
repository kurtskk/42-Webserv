#include <cstdio>
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
	cgiPipeOut(-1),
	cgiTmpFd(-1),
	cgiTotalOutput(0),
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
	hasSent100Continue(false),
	reqTmpFd(-1),
	reqBodyWritten(0),
	chunkRemaining(0),
	chunkReadingSize(true)
{
	std::memset(cgiTmpPath, 0, sizeof(cgiTmpPath));
	std::memset(reqTmpPath, 0, sizeof(reqTmpPath));
}

Client::~Client() {
	if (responseFileFd >= 0)
		close(responseFileFd);
	if (!responseFilePath.empty() && deleteFileOnClose)
		std::remove(responseFilePath.c_str());
	if (cgiPipeOut >= 0)
		close(cgiPipeOut);
	if (cgiTmpFd >= 0)
		close(cgiTmpFd);
	if (cgiTmpPath[0])
		std::remove(cgiTmpPath);
	if (reqTmpFd >= 0)
		close(reqTmpFd);
	if (reqTmpPath[0])
		std::remove(reqTmpPath);
}

void Client::processChunkedBody(const char *data, size_t len) {
	size_t pos = 0;
	while (pos < len) {
		if (chunkReadingSize) {
			chunkBuf += data[pos++];
			if (chunkBuf.length() >= 2 && chunkBuf.substr(chunkBuf.length() - 2) == "\r\n") {
				std::string hexSize = chunkBuf.substr(0, chunkBuf.length() - 2);
				size_t semiPos = hexSize.find(';');
				if (semiPos != std::string::npos)
					hexSize = hexSize.substr(0, semiPos);
				std::istringstream hexStream(hexSize);
				if (hexStream >> std::hex >> chunkRemaining) {
					chunkReadingSize = false;
					if (chunkRemaining == 0)
						requestComplete = true;
				}
				chunkBuf.clear();
			}
		}
		else {
			size_t toWrite = len - pos;
			if (toWrite > chunkRemaining)
				toWrite = chunkRemaining;
			if (toWrite > 0 && reqTmpFd >= 0) {
				write(reqTmpFd, data + pos, toWrite);
				reqBodyWritten += toWrite;
			}
			chunkRemaining -= toWrite;
			pos += toWrite;
			if (chunkRemaining == 0) {
				chunkReadingSize = true;
				// Need to skip \r\n after chunk data
				if (pos < len && data[pos] == '\r') { pos++; }
				if (pos < len && data[pos] == '\n') { pos++; }
			}
		}
	}
}

void Client::appendRequest(const char *buffer, ssize_t bytes) {
	if (!headersParsed) {
		requestBuffer.append(buffer, bytes);
		if (requestBuffer.find("\r\n\r\n") != std::string::npos) {
			isRequestComplete(); // Trigger parsing
		}
	} else {
		if (isChunked)
			processChunkedBody(buffer, bytes);
		else if (contentLength > 0) {
			size_t toWrite = bytes;
			if (reqBodyWritten + bytes > contentLength)
				toWrite = contentLength - reqBodyWritten;
			if (toWrite > 0 && reqTmpFd >= 0) {
				write(reqTmpFd, buffer, toWrite);
				reqBodyWritten += toWrite;
			}
		}
	}
}

int Client::getReqTmpFd() const { return (reqTmpFd); }
const char *Client::getReqTmpPath() const { return (reqTmpPath); }
size_t Client::getReqBodyWritten() const { return (reqBodyWritten); }

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
		if (clPos == std::string::npos) clPos = findHeaderValue(requestBuffer, headerEnd, "Content-length: ");
		if (clPos == std::string::npos) clPos = findHeaderValue(requestBuffer, headerEnd, "content-length: ");
		if (clPos != std::string::npos) {
			size_t valueStart = clPos + 16;
			size_t valueEnd = requestBuffer.find("\r\n", valueStart);
			if (valueEnd != std::string::npos && valueEnd <= headerEnd) {
				std::string clStr = requestBuffer.substr(valueStart, valueEnd - valueStart);
				size_t trimStart = clStr.find_first_not_of(" \t");
				size_t trimEnd = clStr.find_last_not_of(" \t");
				if (trimStart != std::string::npos) clStr = clStr.substr(trimStart, trimEnd - trimStart + 1);
				std::istringstream clStream(clStr);
				if (!(clStream >> contentLength)) contentLength = 0;
			}
		}
		if (requestBuffer.find("Transfer-Encoding: chunked") != std::string::npos ||
			requestBuffer.find("transfer-encoding: chunked") != std::string::npos) {
			isChunked = true;
		}
		size_t expectPos = requestBuffer.find("Expect: 100-continue");
		if (expectPos == std::string::npos) expectPos = requestBuffer.find("Expect: 100-Continue");
		if (expectPos != std::string::npos && expectPos < headerEnd) needs100Continue = true;

		if (contentLength > 0 || isChunked) {
			static unsigned int tmpCounter = 0;
			++tmpCounter;
			std::ostringstream tmpName;
			tmpName << "/tmp/webserv_req_" << static_cast<const void*>(this) << "_" << static_cast<long>(SocketUtils::getCurrentTime()) << "_" << tmpCounter;
			std::strncpy(reqTmpPath, tmpName.str().c_str(), sizeof(reqTmpPath) - 1);
			reqTmpFd = open(reqTmpPath, O_RDWR | O_CREAT | O_EXCL, 0600);

			std::string initialBody = requestBuffer.substr(headerEnd + 4);
			requestBuffer.erase(headerEnd + 4); // Keep only headers in memory

			if (!initialBody.empty()) {
				if (isChunked)
					processChunkedBody(initialBody.c_str(), initialBody.length());
				else {
					write(reqTmpFd, initialBody.c_str(), initialBody.length());
					reqBodyWritten += initialBody.length();
				}
			}
		} else {
			requestBuffer.erase(headerEnd + 4);
		}
	}

	if (headersParsed) {
		if (contentLength > 0) {
			if (reqBodyWritten >= contentLength) {
				requestComplete = true;
				return (true);
			}
			return (false);
		}
		if (isChunked) {
			return (requestComplete);
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
	time_t now = SocketUtils::getCurrentTime();
	return ((now - cgiStartTime) > CGI_TIMEOUT);
}

int Client::getServerFd() const {
	return (serverFd);
}

bool Client::isRequestTooLarge(size_t maxBodySize) const {
	if (headersParsed && contentLength > maxBodySize) {
		return (true);
	}
	if (isChunked && reqBodyWritten > maxBodySize) {
		return (true);
	}
	return (requestBuffer.length() > maxBodySize + 16384);
}
