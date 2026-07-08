#include "HttpRequest.hpp"
#include <sstream>
#include <vector>

static std::string decodeChunkedBody(const std::string &raw) {
	std::string decoded;
	size_t pos = 0;
	while (pos < raw.length()) {
		size_t lineEnd = raw.find("\r\n", pos);
		if (lineEnd == std::string::npos)
			break;
		std::string hexSize = raw.substr(pos, lineEnd - pos);
		size_t semiPos = hexSize.find(';');
		if (semiPos != std::string::npos)
			hexSize = hexSize.substr(0, semiPos);
		std::istringstream hexStream(hexSize);
		unsigned long chunkSize;
		if (!(hexStream >> std::hex >> chunkSize) || !hexStream.eof())
			break;
		if (chunkSize == 0)
			break;
		pos = lineEnd + 2;
		if (pos + chunkSize > raw.length())
			break;
		decoded.append(raw, pos, chunkSize);
		pos += chunkSize;
		if (pos + 2 <= raw.length())
			pos += 2;
	}
	return (decoded);
}

HttpRequest::HttpRequest(const std::string &rawData) {
	method = "";
	uri = "";
	version = "";
	body = "";
	valid = false;
	size_t headerEnd = rawData.find("\r\n\r\n");
	std::string headerPart;
	if (headerEnd != std::string::npos) {
		headerPart = rawData.substr(0, headerEnd);
		std::string rawBody = rawData.substr(headerEnd + 4);
		bool isChunked = (headerPart.find("Transfer-Encoding: chunked") != std::string::npos) || (headerPart.find("transfer-encoding: chunked") != std::string::npos);
		if (isChunked)
			body = decodeChunkedBody(rawBody);
		else {
			size_t contentLength = 0;
			size_t clPos = headerPart.find("Content-Length: ");
			if (clPos == std::string::npos)
				clPos = headerPart.find("Content-length: ");
			if (clPos == std::string::npos)
				clPos = headerPart.find("content-length: ");
			if (clPos != std::string::npos) {
				size_t valueStart = clPos + 16;
				size_t valueEnd = headerPart.find("\r\n", valueStart);
				if (valueEnd != std::string::npos) {
					std::string clStr = headerPart.substr(valueStart, valueEnd - valueStart);
					size_t trimStart = clStr.find_first_not_of(" \t");
					size_t trimEnd = clStr.find_last_not_of(" \t");
					if (trimStart != std::string::npos)
						clStr = clStr.substr(trimStart, trimEnd - trimStart + 1);
					std::istringstream clStream(clStr);
					if (!(clStream >> contentLength))
						contentLength = 0;
				}
			}
			if (contentLength > 0 && contentLength <= rawBody.length())
				body = rawBody.substr(0, contentLength);
			else
				body = rawBody;
		}
	}
	else
		headerPart = rawData;
	std::istringstream stream(headerPart);
	std::string line;
	if (std::getline(stream, line)) {
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);
		parseRequestLine(line);
	}
	while (std::getline(stream, line)) {
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);
		parseHeader(line);
	}
}

HttpRequest::~HttpRequest() {}

static std::string sanitizeUri(const std::string &uri) {
	size_t queryPos = uri.find('?');
	std::string path = (queryPos == std::string::npos) ? uri : uri.substr(0, queryPos);
	std::string query = (queryPos == std::string::npos) ? "" : uri.substr(queryPos);

	std::string result;
	std::vector<std::string> parts;
	size_t pos = 0;
	while (pos < path.length()) {
		size_t nextPos = path.find('/', pos);
		if (nextPos == std::string::npos) {
			parts.push_back(path.substr(pos));
			break;
		}
		if (nextPos > pos)
			parts.push_back(path.substr(pos, nextPos - pos));
		pos = nextPos + 1;
	}
	std::vector<std::string> safeParts;
	for (size_t i = 0; i < parts.size(); ++i) {
		if (parts[i] == "." || parts[i].empty())
			continue;
		else if (parts[i] == "..") {
			if (!safeParts.empty())
				safeParts.pop_back();
		}
		else {
			safeParts.push_back(parts[i]);
		}
	}
	if (safeParts.empty())
		result = "/";
	else {
		for (size_t i = 0; i < safeParts.size(); ++i) {
			result += "/" + safeParts[i];
		}
		if (path.length() > 1 && path[path.length() - 1] == '/')
			result += "/";
	}
	return (result + query);
}

void HttpRequest::parseRequestLine(const std::string &line) {
	std::istringstream stream(line);
	std::string tempMethod, tempUri, tempVersion;
	if (!(stream >> tempMethod >> tempUri >> tempVersion))
		return;
	std::string extraToken;
	if (stream >> extraToken)
		return;
	if (tempMethod.empty() || tempMethod.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos)
		return;
	if (tempUri.empty() || tempUri[0] != '/')
		return;
	if (tempVersion.find("HTTP/") != 0)
		return;
	method = tempMethod;
	uri = sanitizeUri(tempUri);
	version = tempVersion;
	valid = true;
}

void HttpRequest::parseHeader(const std::string &line) {
	size_t colon = line.find(':');
	if (colon != std::string::npos) {
		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		size_t start = value.find_first_not_of(" \t");
		if (start != std::string::npos) {
			value = value.substr(start);
		}
		headers[key] = value;
	}
}

const std::string &HttpRequest::getMethod() const {
	return (method);
}

const std::string &HttpRequest::getUri() const {
	return (uri);
}

const std::string &HttpRequest::getVersion() const {
	return (version);
}

const std::map<std::string, std::string> &HttpRequest::getHeaders() const {
	return (headers);
}

const std::string &HttpRequest::getBody() const {
	return (body);
}

bool HttpRequest::isValid() const {
	return (valid);
}

std::string HttpRequest::getCookie(const std::string &name) const {
	std::map<std::string, std::string>::const_iterator it = headers.find("Cookie");
	if (it == headers.end())
		return ("");
	std::string cookieHeader = it->second;
	size_t pos = 0;
	while (pos < cookieHeader.length()) {
		size_t semiPos = cookieHeader.find(';', pos);
		if (semiPos == std::string::npos)
			semiPos = cookieHeader.length();
		std::string cookie = cookieHeader.substr(pos, semiPos - pos);
		size_t eqPos = cookie.find('=');
		if (eqPos != std::string::npos) {
			std::string cookieName = cookie.substr(0, eqPos);
			size_t start = cookieName.find_first_not_of(" \t");
			if (start != std::string::npos)
				cookieName = cookieName.substr(start);
			size_t end = cookieName.find_last_not_of(" \t");
			if (end != std::string::npos)
				cookieName = cookieName.substr(0, end + 1);
			if (cookieName == name) {
				std::string cookieValue = cookie.substr(eqPos + 1);
				start = cookieValue.find_first_not_of(" \t");
				if (start != std::string::npos)
					cookieValue = cookieValue.substr(start);
				return (cookieValue);
			}
		}
		pos = semiPos + 1;
	}
	return ("");
}
