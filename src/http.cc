#include "http/http.hh"
// #include "http.hh"
#include <fstream>
#include <string.h>

namespace sylar {
namespace http {


HttpMethod StringToHttpMethod(const std::string &m)
{
#define XX(num, name, string)              \
    if (strcmp(#string, m.c_str()) == 0) { \
        return HttpMethod::name;           \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVAILD_METHOD;
}

HttpMethod CharsToHttpMethod(const char *m)
{
#define XX(num, name, string)						 \
    if (strncmp(#string, m, strlen(#string)) == 0) { \
        return HttpMethod::name;					 \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVAILD_METHOD;
}

static const char* s_method_string[] = {
#define XX(num, name, string) #string,
	HTTP_METHOD_MAP(XX)
#undef XX
};

const char* HttpMethodToString(const HttpMethod& m) {
	uint32_t idx = (uint32_t)m;
	if(idx >= (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
		return "<unknow>";
	}
	return s_method_string[idx];
}

const char* HttpStatusToString(const HttpStatus& m) {
	switch(m) {
#define XX(code, name, desc)					\
		case HttpStatus::name:					\
			return #desc;
		HTTP_STATUS_MAP(XX);
#undef XX
		default:
			return "<unknow>";
	}
}

bool CaseInsensitiveLess::operator()(const std::string& lhs,
									 const std::string& rhs) const {
	return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

HttpRequest::HttpRequest(uint8_t version, bool close)
	:m_method(HttpMethod::GET)
	,m_close(close)
	,m_version(version)
	,m_path("/")
{}

std::string HttpRequest::getHeader(const std::string &key,
                                   const std::string &def) const
{
	auto it = m_headers.find(key);
	return it == m_headers.end() ? def : it->second;
}

std::string HttpRequest::getParam(const std::string &key,
                                  const std::string &def) const
{
	auto it = m_params.find(key);
	return it == m_params.end() ? def : it->second;
}

std::string HttpRequest::getCookie(const std::string &key,
                                   const std::string &def) const
{
	auto it = m_cookies.find(key);
	return it == m_cookies.end() ? def : it->second;
}
void HttpRequest::setHeader(const std::string &key, const std::string &val) {
	m_headers[key] = val;
}
void HttpRequest::setParam(const std::string &key, const std::string &val) {
	m_params[key] = val;
}
void HttpRequest::setCookie(const std::string &key, const std::string &val) {
	m_cookies[key] = val;
}
void HttpRequest::delHeader(const std::string &key) {
	m_headers.erase(key);
}
void HttpRequest::delParam(const std::string &key) {
	m_params.erase(key);
}
void HttpRequest::delCookie(const std::string &key) {
	m_cookies.erase(key);
}
bool HttpRequest::hasHeader(const std::string &key, std::string *val)
{
	auto it = m_headers.find(key);
	if(it == m_headers.end()) {
		return false;
	}
	if(val) {
		*val = it->second;
	}
	return true;
}
bool HttpRequest::hasParam(const std::string &key, std::string *val)
{
	auto it = m_params.find(key);
	if(it == m_params.end()) {
		return false;
	}
	if(val) {
		*val = it->second;
	}
	return true;
}
bool HttpRequest::hasCookie(const std::string &key, std::string *val)
{
	auto it = m_cookies.find(key);
	if(it == m_cookies.end()) {
		return false;
	}
	if(val) {
		*val = it->second;
	}
	return true;
}

std::string HttpRequest::toString() const
{
	std::stringstream ss;
	dump(ss);
	return ss.str();
}

std::ostream & HttpRequest::dump(std::ostream &os) const {
	/**
	* 基本报文格式
	* --------------------------------------
	* GET /path?query=value#fragment HTTP/1.1
	* Host: example.com
	* Connection: keep-alive
	* User-Agent: curl/7.68.0
	* \r\n\r\n[body 可选]
	*/

	os << HttpMethodToString(m_method) << " "
		<< m_path
		<< (m_query.empty() ? "" : "?")
		<< m_query
		<< (m_fragment.empty() ? "" : "#")
		<< m_fragment
		<< " HTTP/"
		<< ((uint32_t)(m_version >> 4))
		<< "."
		<< ((uint32_t)(m_version & 0x0F))
		<< "\r\n";

	os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
	for(auto& i : m_headers) {
		if(strcasecmp(i.first.c_str(), "connection") == 0) {
			continue;
		}
		os << i.first << ": " << i.second << "\r\n";
	}

	if(!m_body.empty()) {
		os << "content-length: " << m_body.size() << "\r\n\r\n"
			<< m_body;
	} else {
		os << "\r\n";
	}
	return os;
}



HttpResponse::HttpResponse(uint8_t version, bool close)
	:m_status(HttpStatus::OK)
	 ,m_version(version)
	 ,m_close(close)
{ }

std::string HttpResponse::getHeaders(const std::string& key,
									 const std::string& def)
	const
{
	auto it = m_headers.find(key);
	return (it == m_headers.end()) ? def : it->second;
}

void HttpResponse::setHeader(const std::string key, const std::string& val)
{
	m_headers[key] = val;
}

void HttpResponse::delHeader(const std::string& key)
{
	m_headers.erase(key);
}

std::ostream& HttpResponse::dump(std::ostream & os) const
{
	/**
	* HTTP/1.1 301 Moved Permanently
	* Server: Tengine
	* Date: Mon, 11 Aug 2025 03:49:07 GMT
	* Content-Type: text/html
	* Content-Length: 239
	* Connection: keep-alive
	* Location: https://www.bilibili.com/
	*/
	os << "HTTP/"
	   << ((uint32_t)(m_version >> 4))
	   << "."
	   << ((uint32_t)(m_version & 0x0F))
	   << " "
	   << (uint32_t)(m_status)
	   << " "
	   << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
	   << "\r\n";

	for(auto &i : m_headers) {
		if(strcasecmp(i.first.c_str(), "connection") == 0) {
			continue;
		}
		os<< i.first << ": " << i.second << "\r\n";
	}

	os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";

	if(!m_body.empty()) {
		os << "content-length: " << m_body.size() << "\r\n\r\n"
			<< m_body;
	} else {
		os << "\r\n";
	}

	return os;
}

std::string HttpResponse::toString() const
{
	std::stringstream ss;
	dump(ss);
	return ss.str();
}


std::ostream& operator<<(std::ostream& os, const HttpRequest& req)
{
	return req.dump(os);
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& resp)
{
	return resp.dump(os);
}

} // namespace http
} // namespace sylar
