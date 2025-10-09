#include <fnmatch.h>

#include "http/servlet.hh"

namespace sylar {
namespace http {

ServletFunction::ServletFunction(callBack cb)
    : Servlet("FunctionServlet"), m_cb(cb)
{}

int32_t ServletFunction::handle(sylar::http::HttpRequest::ptr request,
                                sylar::http::HttpResponse::ptr response,
                                sylar::http::HttpSession::ptr session)
{
	return m_cb(request, response, session);
}

ServletDispatcher::ServletDispatcher()
	: Servlet("ServletDispatcher")
{
	  m_default.reset(new ServletNotFound());
}

int32_t ServletDispatcher::handle(sylar::http::HttpRequest::ptr request,
                                  sylar::http::HttpResponse::ptr response,
                                  sylar::http::HttpSession::ptr session)
{
	auto servlet = getMatchedServlet(request->getPath());
	if(servlet) {
		servlet->handle(request, response, session);
	}
	return 0;
}

void ServletDispatcher::addServlet(const std::string &uri,
								   ServletFunction::callBack cb)
{
	RWMutexType::WriteLock lock(m_mutex);
	m_datas[uri].reset(new ServletFunction(cb));
}

auto ServletDispatcher::addServlet(const std::string &uri,
								   Servlet::ptr servlet)
	-> void
{
	RWMutexType::WriteLock lock(m_mutex);
	m_datas[uri] = servlet;
}

auto ServletDispatcher::addGlobServlet(const std::string &uri,
									   ServletFunction::callBack cb)
	-> void
{
	return addGlobServlet(uri, ServletFunction::ptr(new ServletFunction(cb)));
}

auto ServletDispatcher::addGlobServlet(const std::string& uri,
									   Servlet::ptr servlet)
		-> void
{
	RWMutexType::WriteLock lock(m_mutex);
	for(auto it = m_globs.begin();
		it != m_globs.end(); ++it) {
		if(it->first == uri) {
			m_globs.erase(it);
			break;
		}
	}
	m_globs.push_back(std::make_pair(uri, servlet));

  // 加锁解锁带来了不必要的开销
  // delglobservlet(uri);
	// rwmutextype::writelock lock(m_mutex);
	// m_globs.push_back(std::make_pair(uri, servlet));
}

auto ServletDispatcher::delServlet(const std::string &uri)
	-> void
{
	RWMutexType::WriteLock lock(m_mutex);
	m_datas.erase(uri);
}

auto ServletDispatcher::delGlobServlet(const std::string &uri) -> void
{
	RWMutexType::WriteLock lock(m_mutex);
	for(auto it = m_globs.begin();
		it != m_globs.end(); ++it) {
		if(it->first == uri) {
			m_globs.erase(it);
			break;
		}
	}
}

auto ServletDispatcher::getServlet(const std::string &uri)
	-> Servlet::ptr
{
	RWMutexType::WriteLock lock(m_mutex);
	auto it = m_datas.find(uri);
	return it == m_datas.end() ? nullptr : it->second;
}

auto ServletDispatcher::getGlobServlet(const std::string &uri)
	-> Servlet::ptr
{
	RWMutexType::WriteLock lock(m_mutex);
	for(auto it = m_globs.begin();
		it != m_globs.end(); ++it) {
		if(it->first == uri) {
			return it->second;
		}
	}
	return nullptr;
}

auto ServletDispatcher::getMatchedServlet(const std::string &uri)
	-> Servlet::ptr
{
	RWMutexType::WriteLock lock(m_mutex);

    auto mit = m_datas.find(uri);
    if(mit != m_datas.end()) {
        return mit->second;
    }
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
            return it->second;
        }
    }

	return m_default ;
}

ServletNotFound::ServletNotFound()
	:Servlet("ServletNotFound")
{ }

int32_t ServletNotFound::handle(sylar::http::HttpRequest::ptr request,
                                sylar::http::HttpResponse::ptr response,
                                sylar::http::HttpSession::ptr session)
{
    static const std::string& RSP_BODY = "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>sylar/1.0.0</center></body></html>";

    response->setStatus(sylar::http::HttpStatus::NOT_FOUND);
    response->setHeader("Server", "sylar/1.0.0");
    response->setHeader("Content-Type", "text/html");
    response->setBody(RSP_BODY);

    return 0;
}
} // namespace http
} // namespace sylar
