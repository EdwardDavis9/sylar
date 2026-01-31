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

/**
 * @brief Servlet 封装
 */
class Servlet {
  public:
    using ptr = std::shared_ptr<Servlet>;

    /**
     * @brief     构造函数
     * @param[in] name 名称
     */
    Servlet(const std::string &name) : m_name(name) {}

    /**
     * @brief 返回Servlet名称
     */
    const std::string &getName() const { return m_name; }

    /**
     * @brief 析构函数
     */
    virtual ~Servlet() {}

    /**
     * @brief     处理请求
     * @param[in] request HTTP请求
     * @param[in] response HTTP响应
     * @param[in] session HTTP连接
     * @return    是否处理成功
     */
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                           sylar::http::HttpResponse::ptr response,
                           sylar::http::HttpSession::ptr session) = 0;

  private:
    std::string m_name; /**< 名称 */
};

/**
 * @brief 函数式 Servlet
 */
class ServletFunction : public Servlet {
  public:
    using ptr = std::shared_ptr<ServletFunction>;
    using callBack =
        std::function<int32_t(sylar::http::HttpRequest::ptr request,
                              sylar::http::HttpResponse::ptr response,
                              sylar::http::HttpSession::ptr session)>;

    /**
     * @brief     构造函数
     * @param[in] cb 回调函数
     */
    ServletFunction(callBack cb);

    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                           sylar::http::HttpResponse::ptr response,
                           sylar::http::HttpSession::ptr session) override;

  private:
    callBack m_cb;
};

/**
 * @brief Servlet 分发器
 */
class ServletDispatcher : public Servlet {
  public:
    using ptr         = std::shared_ptr<ServletDispatcher>;
    using RWMutexType = RWMutex;

    /**
     * @brief 构造函数
     * @details 默认的 servlet 是 servletvNotFound
     */
    ServletDispatcher();

    /**
     * @brief     处理 servlet
     * @param[in] uri uri
     * @param[in] slt serlvet
     */
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                           sylar::http::HttpResponse::ptr response,
                           sylar::http::HttpSession::ptr session) override;

    /**
     * @brief     添加 servlet
     * @param[in] uri uri
     * @param[in] cb FunctionServlet 回调函数
     */
    void addServlet(const std::string &uri, Servlet::ptr servlet);

    /**
     * @brief     添加 servlet
     * @param[in] uri uri
     * @param[in] cb FunctionServlet 回调函数
     */
    void addServlet(const std::string &uri, ServletFunction::callBack cb);

    /**
     * @brief     添加模糊匹配 servlet
     * @param[in] uri uri 模糊匹配 /sylar_*
     * @param[in] slt servlet
     */
    void addGlobServlet(const std::string &uri, Servlet::ptr servlet);

    /**
     * @brief     添加模糊匹配 servlet
     * @param[in] uri uri 模糊匹配 /sylar_*
     * @param[in] cb FunctionServlet 回调函数
     */
    void addGlobServlet(const std::string &uri, ServletFunction::callBack cb);

    /**
     * @brief     删除 servlet
     * @param[in] uri uri
     */
    void delServlet(const std::string &uri);

    /**
     * @brief     删除模糊匹配 servlet
     * @param[in] uri uri
     */
    void delGlobServlet(const std::string &uri);

    /**
     * @brief 返回默认 servlet
     */
    Servlet::ptr getDefault() const { return m_default; }

    /**
     * @brief     设置默认 servlet
     * @param[in] v servlet
     */
    void setDefault(Servlet::ptr servlet) { m_default = servlet; }

    /**
     * @brief     通过 uri 获取 servlet
     * @param[in] uri uri
     * @return    返回对应的 servlet
     */
    Servlet::ptr getServlet(const std::string &uri);

    /**
     * @brief 通过 uri 获取模糊匹配 servlet
     * @param[in] uri uri
     * @return 返回对应的 servlet
     */
    Servlet::ptr getGlobServlet(const std::string &uri);

    /**
     * @brief 通过 uri 获取 servlet
     * @param[in] uri uri
     * @return 优先精准匹配,其次模糊匹配,最后返回默认
     */
    Servlet::ptr getMatchedServlet(const std::string &uri);

  private:
    RWMutexType m_mutex;

    // uri(/sylar/xxx) -> servlet
    std::unordered_map<std::string, Servlet::ptr> m_datas;

    // uri(/sylar/*) -> servlet
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;

    // default servlet
    Servlet::ptr m_default;
};

/**
 * @brief NotFoundServlet (默认返回404)
 */
class ServletNotFound : public Servlet {
  public:
    using ptr = std::shared_ptr<ServletNotFound>;

    /**
     * @brief 构造函数
     */
    ServletNotFound();

    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                           sylar::http::HttpResponse::ptr response,
                           sylar::http::HttpSession::ptr session) override;
};

}; // namespace http
}; // namespace sylar

#endif // __HTTP_SERVLET_H__
