#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include <string>
# include <map>

class HttpRequest {
public:
	HttpRequest(const std::string &rawData);
	~HttpRequest();

	const std::string &getMethod() const;
	const std::string &getUri() const;
	const std::string &getVersion() const;
	const std::map<std::string, std::string> &getHeaders() const;
	const std::string &getBody() const;
	bool isValid() const;
	std::string getCookie(const std::string &name) const;

private:
	std::string method;
	std::string uri;
	std::string version;
	std::map<std::string, std::string> headers;
	std::string body;
	bool valid;

	void parseRequestLine(const std::string &line);
	void parseHeader(const std::string &line);

	HttpRequest(const HttpRequest &other);
	HttpRequest &operator=(const HttpRequest &other);
};

#endif
