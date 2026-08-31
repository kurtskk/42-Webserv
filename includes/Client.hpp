#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "SocketUtils.hpp"
# include <string>
# include <sys/types.h>
# include <time.h>

class Client {
public:
	bool sending100Continue;
	bool cgiRunning;
	pid_t cgiPid;
	int cgiPipeOut;
	int cgiTmpFd;
	char cgiTmpPath[64];
	size_t cgiTotalOutput;
	time_t cgiStartTime;
	static const time_t CGI_TIMEOUT = 30;

	Client(int fd, int serverFd);
	~Client();

	void appendRequest(const char *buffer, ssize_t bytes);
	const std::string &getRequestBuffer() const;
	void clearRequestBuffer();
	
	int getReqTmpFd() const;
	const char *getReqTmpPath() const;
	size_t getReqBodyWritten() const;

	void setResponse(const std::string &response);
	void setFileResponse(const std::string &header, const std::string &filePath, size_t fileSize, size_t fileStartOffset = 0, bool deleteFile = false);

	size_t getResponseRemaining() const;
	void advanceResponseOffset(ssize_t bytes);
	ssize_t getResponseData(char *buf, size_t maxLen);

	bool isRequestComplete();
	bool isRequestTooLarge(size_t maxBodySize) const;

	bool isHeadersParsed() const;
	bool getNeeds100Continue() const;
	bool getHasSent100Continue() const;
	void setHasSent100Continue(bool status);

	bool isResponseComplete() const;
	bool isCgiTimedOut() const;
	int getServerFd() const;

private:
	int fd;
	int serverFd;
	std::string requestBuffer;
	std::string responseBuffer;
	std::string responseFilePath;
	size_t responseFileSize;
	int responseFileFd;
	size_t responseFileStartOffset;
	size_t responseSendOffset;
	size_t responseHeaderSize;
	size_t cacheOffset;
	bool requestComplete;
	bool responseComplete;
	bool fileBasedResponse;
	bool deleteFileOnClose;
	bool headersParsed;
	size_t headerEnd;
	size_t contentLength;
	bool isChunked;
	bool needs100Continue;
	bool hasSent100Continue;

	int reqTmpFd;
	char reqTmpPath[64];
	size_t reqBodyWritten;
	std::string chunkBuf;
	size_t chunkRemaining;
	bool chunkReadingSize;

	void processChunkedBody(const char *data, size_t len);

	Client(const Client &other);
	Client &operator=(const Client &other);
};

#endif
