#include "HttpResponse.hpp"
#include "CgiHandler.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

HttpResponse::HttpResponse() :
	statusCode("200"),
	statusMessage("OK"),
	contentType("text/html; charset=utf-8"),
	isFileResponse(false),
	fileSize(0),
	needsCgi(false)
	{}

HttpResponse::~HttpResponse() {}

static std::string loadErrorPage(const std::string &filePath, const std::string &rootDir) {
	std::string fullPath = filePath;
	if (!rootDir.empty()) {
		if (!fullPath.empty() && fullPath[0] == '/')
			fullPath = rootDir + fullPath;
		else
			fullPath = rootDir + "/" + fullPath;
	}
	std::ifstream file(fullPath.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open())
		return ("");
	std::ostringstream ss;
	ss << file.rdbuf();
	return (ss.str());
}

void HttpResponse::setErrorResponse(int code, const ServerConfig *srv) {
	switch (code) {
		case 400: statusCode = "400"; statusMessage = "Bad Request"; break;
		case 403: statusCode = "403"; statusMessage = "Forbidden"; break;
		case 404: statusCode = "404"; statusMessage = "Not Found"; break;
		case 405: statusCode = "405"; statusMessage = "Method Not Allowed"; break;
		case 413: statusCode = "413"; statusMessage = "Payload Too Large"; break;
		case 504: statusCode = "504"; statusMessage = "Gateway Timeout"; break;
		default:  statusCode = "500"; statusMessage = "Internal Server Error"; break;
	}
	if (srv) {
		const std::map<int, std::string> &pages = srv->getErrorPages();
		std::map<int, std::string>::const_iterator it = pages.find(code);
		if (it != pages.end()) {
			std::string content = loadErrorPage(it->second, srv->getRoot());
			if (!content.empty()) {
				body = content;
				contentType = "text/html; charset=utf-8";
				return;
			}
		}
	}
	body = "<html><body><h1>" + statusCode + " " + statusMessage + "</h1></body></html>";
	contentType = "text/html; charset=utf-8";
}

std::string HttpResponse::buildResponseHeader() const {
	std::ostringstream response;
	size_t bodyLen = isFileResponse ? fileSize : body.length();
	response << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
	response << "Content-Type: " << contentType << "\r\n";
	response << "Content-Length: " << bodyLen << "\r\n";
	if ((statusCode == "301" || statusCode == "302") && !redirectUrl.empty())
		response << "Location: " << redirectUrl << "\r\n";
	for (size_t i = 0; i < cookies.size(); ++i)
		response << "Set-Cookie: " << cookies[i] << "\r\n";
	response << "Connection: close\r\n";
	response << "\r\n";
	return (response.str());
}

std::string HttpResponse::buildResponseString() const {
	if (isFileResponse)
		return (buildResponseHeader());
	std::string header = buildResponseHeader();
	return (header + body);
}

bool HttpResponse::getIsFileResponse() const {
	return (isFileResponse);
}

const std::string &HttpResponse::getFilePath() const {
	return (filePath);
}

size_t HttpResponse::getFileSize() const {
	return (fileSize);
}

bool HttpResponse::needsCgiExec() const {
	return (needsCgi);
}

const std::string &HttpResponse::getCgiScriptPath() const {
	return (cgiScriptPath);
}

const std::string &HttpResponse::getCgiExecPath() const {
	return (cgiExecPath);
}

std::string HttpResponse::generateResponse(const HttpRequest &req, const ServerConfig &serverConfig, std::map<std::string, std::map<std::string, std::string> > &sessions) {
	const LocationConfig *matchedLoc = NULL;
	std::string uri = req.getUri();
	std::string searchUri = uri;
	if (searchUri == "/session_login") {
		if (req.getMethod() == "GET") {
			statusCode = "200";
			statusMessage = "OK";
			contentType = "text/html; charset=utf-8";
			body = "<html><body><h1 style='color:blue;'>Login (Bonus Sessions)</h1><form method='POST' action='/session_login'><input type='text' name='username' placeholder='Seu Nome'><input type='submit' value='Logar'></form></body></html>";
			return (buildResponseString());
		}
		else if (req.getMethod() == "POST") {
			std::string username = "Anonymous";
			std::string reqBody = req.getBody();
			size_t pos = reqBody.find("username=");
			if (pos != std::string::npos) {
				size_t ampersand = reqBody.find("&", pos);
				if (ampersand == std::string::npos) ampersand = reqBody.length();
				username = reqBody.substr(pos + 9, ampersand - (pos + 9));
			}
			std::ostringstream sid;
			sid << "sess_" << time(NULL);
			std::string sessionId = sid.str();
			setSessionData(sessions, sessionId, "user", username);
			addCookie("session_id=" + sessionId + "; Path=/; HttpOnly");
			statusCode = "302";
			statusMessage = "Found";
			redirectUrl = "/session_profile";
			return (buildResponseString());
		}
	}
	else if (searchUri == "/session_profile") {
		std::string sessionId = "";
		std::map<std::string, std::string>::const_iterator cookieIt = req.getHeaders().find("Cookie");
		if (cookieIt != req.getHeaders().end()) {
			size_t pos = cookieIt->second.find("session_id=");
			if (pos != std::string::npos) {
				size_t semi = cookieIt->second.find(";", pos);
				if (semi == std::string::npos) semi = cookieIt->second.length();
				sessionId = cookieIt->second.substr(pos + 11, semi - (pos + 11));
			}
		}
		std::string username = getSessionData(sessions, sessionId, "user");
		statusCode = "200";
		statusMessage = "OK";
		contentType = "text/html; charset=utf-8";
		if (username.empty()) {
			body = "<html><body style='font-family: sans-serif;'><h1 style='color:red;'>Nao logado!</h1><p>Nenhuma sessao ativa foi detectada nos cookies.</p><a href='/session_login'>Ir para a pagina de Login</a></body></html>";
		}
		else {
			body = "<html><body style='font-family: sans-serif;'><h1 style='color:green;'>Bem-vindo ao seu Perfil, " + username + "!</h1><p>Sua ID de sessao armazena no map do webserv: <strong>" + sessionId + "</strong></p><a href='/session_login'>Logar novamente</a></body></html>";
		}
		return (buildResponseString());
	}
	if (searchUri.length() > 1 && searchUri[searchUri.length() - 1] == '/')
		searchUri.erase(searchUri.length() - 1);
	size_t longestMatch = 0;
	const std::vector<LocationConfig> &locations = serverConfig.getLocations();
	for (size_t i = 0; i < locations.size(); ++i) {
		std::string locPath = locations[i].getPath();
		if (locPath.length() > 1 && locPath[locPath.length() - 1] == '/')
			locPath.erase(locPath.length() - 1);
		if (searchUri.find(locPath) == 0) {
			if (searchUri.length() == locPath.length() || searchUri[locPath.length()] == '/' || locPath == "/") {
				if (locPath.length() > longestMatch) {
					longestMatch = locPath.length();
					matchedLoc = &locations[i];
				}
			}
		}
	}
	if (!matchedLoc) {
		setErrorResponse(404, &serverConfig);
		return (buildResponseString());
	}
	if (matchedLoc->getRedirectCode() != 0) {
		statusCode = (matchedLoc->getRedirectCode() == 301) ? "301" : "302";
		statusMessage = (matchedLoc->getRedirectCode() == 301) ? "Moved Permanently" : "Found";
		redirectUrl = matchedLoc->getRedirectUrl();
		return (buildResponseString());
	}
	const std::vector<std::string> &allowed = matchedLoc->getAllowedMethods();
	bool methodAllowed = false;
	for (size_t i = 0; i < allowed.size(); ++i) {
		if (allowed[i] == req.getMethod()) {
			methodAllowed = true;
			break;
		}
	}
	if (!methodAllowed) {
		if (!matchedLoc->getCgiExtension().empty() &&
			searchUri.find(matchedLoc->getCgiExtension()) != std::string::npos &&
			req.getMethod() == "POST")
			methodAllowed = true;
	}
	if (!methodAllowed) {
		setErrorResponse(405, &serverConfig);
		return (buildResponseString());
	}
	std::string rootPath = matchedLoc->getRoot().empty() ? serverConfig.getRoot() : matchedLoc->getRoot();
	if (rootPath.empty())
		rootPath = "www";
	std::string locPath = matchedLoc->getPath();
	if (locPath.length() > 1 && locPath[locPath.length() - 1] == '/')
		locPath.erase(locPath.length() - 1);
	std::string targetFile = rootPath;
	if (searchUri == locPath) {
		if (!matchedLoc->getIndex().empty())
			targetFile += "/" + matchedLoc->getIndex();
	}
	else {
		std::string subUri = searchUri.substr(locPath.length());
		if (!rootPath.empty() && rootPath[rootPath.length() - 1] != '/' && !subUri.empty() && subUri[0] != '/')
			targetFile += "/";
		targetFile += subUri;
	}
	struct stat dirStat;
	if (stat(targetFile.c_str(), &dirStat) == 0 && S_ISDIR(dirStat.st_mode)) {
		if (!matchedLoc->getIndex().empty())
			targetFile += "/" + matchedLoc->getIndex();
	}
	size_t maxBody = matchedLoc->getClientMaxBodySize() > 0 ? matchedLoc->getClientMaxBodySize() : serverConfig.getClientMaxBodySize();
	if (req.getBody().length() > maxBody) {
		statusCode = "413";
		statusMessage = "Payload Too Large";
		body = "<html><body><h1>413 Payload Too Large</h1></body></html>";
		contentType = "text/html; charset=utf-8";
		return (buildResponseString());
	}
	if (!matchedLoc->getCgiExtension().empty() && targetFile.find(matchedLoc->getCgiExtension()) != std::string::npos) {
		needsCgi = true;
		cgiScriptPath = targetFile;
		cgiExecPath = matchedLoc->getCgiPath();
		return ("");
	}
	if (req.getMethod() == "GET")
		handleGet(req, serverConfig, targetFile, matchedLoc);
	else if (req.getMethod() == "POST")
		handlePost(req, serverConfig, targetFile, matchedLoc);
	else if (req.getMethod() == "DELETE")
		handleDelete(req, serverConfig, targetFile);
	else
		setErrorResponse(405, &serverConfig);
	return (buildResponseString());
}

void HttpResponse::handleGet(const HttpRequest &, const ServerConfig &serverConfig, const std::string &targetFile, const LocationConfig *matchedLoc) {
	struct stat fileStat;

	if (stat(targetFile.c_str(), &fileStat) == 0 && S_ISDIR(fileStat.st_mode)) {
		if (matchedLoc && matchedLoc->getAutoindex()) {
			DIR *dir = opendir(targetFile.c_str());
			if (!dir) {
				setErrorResponse(403, &serverConfig);
				return;
			}
			std::ostringstream page;
			page << "<html><head><title>Index of " << targetFile << "</title></head>";
			page << "<body><h1>Index of " << targetFile << "</h1><hr><pre>";
			struct dirent *entry;
			while ((entry = readdir(dir)) != NULL) {
				std::string name = entry->d_name;
				if (name == ".")
					continue;
				page << "<a href=\"" << name << "\">" << name << "</a>\n";
			}
			closedir(dir);
			page << "</pre><hr></body></html>";
			body = page.str();
			contentType = "text/html; charset=utf-8";
			statusCode = "200";
			statusMessage = "OK";
		}
		else
			setErrorResponse(404, &serverConfig);
		return;
	}
	if (stat(targetFile.c_str(), &fileStat) == 0 && S_ISREG(fileStat.st_mode)) {
		if (targetFile.find(".css") != std::string::npos)
			contentType = "text/css";
		else if (targetFile.find(".png") != std::string::npos)
			contentType = "image/png";
		else if (targetFile.find(".jpg") != std::string::npos)
			contentType = "image/jpeg";
		else
			contentType = "text/html; charset=utf-8";
		isFileResponse = true;
		filePath = targetFile;
		fileSize = fileStat.st_size;
		statusCode = "200";
		statusMessage = "OK";
	}
	else
		setErrorResponse(404, &serverConfig);
}

void HttpResponse::handlePost(const HttpRequest &req, const ServerConfig &, const std::string &, const LocationConfig *matchedLoc) {
	const std::map<std::string, std::string> &headers = req.getHeaders();
	std::map<std::string, std::string>::const_iterator it = headers.find("Content-Type");
	if (it == headers.end())
		it = headers.find("Content-type");
	if (it != headers.end() && it->second.find("multipart/form-data") != std::string::npos) {
		std::string boundary;
		size_t pos = it->second.find("boundary=");
		if (pos != std::string::npos)
			boundary = "--" + it->second.substr(pos + 9);
		if (!boundary.empty()) {
			const std::string &reqBody = req.getBody();
			size_t fileStart = reqBody.find(boundary);
			if (fileStart != std::string::npos) {
				fileStart += boundary.length();
				fileStart = reqBody.find("\r\n\r\n", fileStart);
				if (fileStart != std::string::npos) {
					fileStart += 4;
					size_t fileEnd = reqBody.find(boundary, fileStart);
					if (fileEnd != std::string::npos) {
						if (fileEnd >= 2)
							fileEnd -= 2;
						std::string filename = "uploaded_file";
						size_t headerStart = reqBody.find(boundary) + boundary.length();
						std::string partHeaders = reqBody.substr(headerStart, fileStart - headerStart);
						size_t fnPos = partHeaders.find("filename=\"");
						if (fnPos != std::string::npos) {
							fnPos += 10;
							size_t fnEndPos = partHeaders.find("\"", fnPos);
							if (fnEndPos != std::string::npos)
								filename = partHeaders.substr(fnPos, fnEndPos - fnPos);
						}
						std::string uploadDir = "upload_store";
						if (matchedLoc && !matchedLoc->getUploadStore().empty())
							uploadDir = matchedLoc->getUploadStore();
						struct stat st;
						if (stat(uploadDir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
							setErrorResponse(500);
							return;
						}
						std::string savePath = uploadDir + "/" + filename;
						std::ofstream ofs(savePath.c_str(), std::ios::out | std::ios::binary);
						if (ofs) {
							ofs.write(reqBody.c_str() + fileStart, fileEnd - fileStart);
							ofs.close();
							statusCode = "201";
							statusMessage = "Created";
							this->body = "<html><body><h1>File uploaded successfully</h1></body></html>";
							return;
						}
						else {
							setErrorResponse(500);
							return;
						}
					}
				}
			}
		}
	}
	statusCode = "201";
	statusMessage = "Created";
	this->body = "Received POST data: \n" + req.getBody();
}

void HttpResponse::handleDelete(const HttpRequest &, const ServerConfig &serverConfig, const std::string &targetFile) {
	if (unlink(targetFile.c_str()) == 0) {
		statusCode = "200";
		statusMessage = "OK";
		body = "<html><body><h1>File Deleted Successfully</h1></body></html>";
		contentType = "text/html; charset=utf-8";
	}
	else
		setErrorResponse(404, &serverConfig);
}

std::string HttpResponse::getErrorResponse(int code, const ServerConfig *srv) {
	setErrorResponse(code, srv);
	return (buildResponseString());
}

void HttpResponse::addCookie(const std::string &cookie) {
	cookies.push_back(cookie);
}

void HttpResponse::setSessionData(std::map<std::string, std::map<std::string, std::string> > &sessions, const std::string &sessionId, const std::string &key, const std::string &value) {
	sessions[sessionId][key] = value;
}

const std::string &HttpResponse::getSessionData(const std::map<std::string, std::map<std::string, std::string> > &sessions, const std::string &sessionId, const std::string &key) const {
	static std::string emptyStr = "";
	std::map<std::string, std::map<std::string, std::string> >::const_iterator it = sessions.find(sessionId);
	if (it == sessions.end())
		return (emptyStr);
	std::map<std::string, std::string>::const_iterator dataIt = it->second.find(key);
	if (dataIt == it->second.end())
		return (emptyStr);
	return (dataIt->second);
}
