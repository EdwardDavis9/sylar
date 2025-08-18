#ifndef __HTTP_SERVLET_H__
#define __HTTP_SERVLET_H__

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

#include "sylar/thread.hh"
#include "http/http_session.hh"
#include "http/http.hh"

namespace sylar {
namespace http {

class Servlet {
  public:
    using ptr = std::shared_ptr<Servlet>;

    Servlet(const std::string &name) : m_name(name) {}

    const std::string &getName() const { return m_name; }

    virtual ~Servlet() {}
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                           sylar::http::HttpResponse::ptr response,
                           sylar::http::HttpSession::ptr session) = 0;

  private:
    std::string m_name;
};

class ServletFunction : public Servlet {
  public:
    using ptr = std::shared_ptr<ServletFunction>;
    using callBack =
        std::function<int32_t(sylar::http::HttpRequest::ptr request,
                               sylar::http::HttpResponse::ptr response,
                               sylar::http::HttpSession::ptr session)>;

    ServletFunction(callBack cb);

    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                           sylar::http::HttpResponse::ptr response,
                           sylar::http::HttpSession::ptr session) override;

  private:
    callBack m_cb;
};

class ServletDispatcher : public Servlet {
  public:
    using ptr         = std::shared_ptr<ServletDispatcher>;
    using RWMutexType = RWMutex;

    ServletDispatcher();

    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                           sylar::http::HttpResponse::ptr response,
                           sylar::http::HttpSession::ptr session) override;

  void addServlet(const std::string& uri, Servlet::ptr servlet);
  void addServlet(const std::string& uri, ServletFunction::callBack cb);
  void addGlobServlet(const std::string& uri, Servlet::ptr servlet);
  void addGlobServlet(const std::string& uri, ServletFunction::callBack cb);

  void delServlet(const std::string& uri);
  void delGlobServlet(const std::string& uri);

  Servlet::ptr getDefault() const { return m_default; }
  void setDefault(Servlet::ptr servlet) { m_default = servlet; }

  Servlet::ptr getServlet(const std::string& uri);
  Servlet::ptr getGlobServlet(const std::string& uri);

  Servlet::ptr getMatchedServlet(const std::string& uri);

  private:
    RWMutexType m_mutex;

    // uri(/sylar/xxx) -> servlet
    std::unordered_map<std::string, Servlet::ptr> m_datas;

    // uri(/sylar/*) -> servlet
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;

    // default servlet
    Servlet::ptr m_default;
};

class ServletNotFound : public Servlet {
  public:
    using ptr = std::shared_ptr<ServletNotFound>;

    ServletNotFound();

    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                           sylar::http::HttpResponse::ptr response,
                           sylar::http::HttpSession::ptr session) override;

  private:
};

}; // namespace http
}; // namespace sylar

#endif // __HTTP_SERVLET_H__
