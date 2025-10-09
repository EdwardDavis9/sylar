/**
 * @file      uri.hh
 * @brief     Header of uri
 * @date      Thu Aug 21 10:28:56 2025
 * @author    edward
 * @copyright BSD-3-Clause
 */

#ifndef __HTTP_URI_H__
#define __HTTP_URI_H__

#include <memory>
#include <string>
#include <stdint.h>

#include "sylar/address.hh"

namespace sylar {

/*
     foo://user@sylar.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment
*/
namespace http {
/**
 * @class Uri
 */
class Uri {
  public:
    using ptr = std::shared_ptr<Uri>;

    static Uri::ptr Create(const std::string &uri);

    /**
     * @brief 构造函数
     */
    Uri();

    /**
     * @brief  返回 scheme
     * @return scheme
     */
    const std::string &getScheme() const { return m_scheme; }
    /**
     * @brief  返回用户信息
     * @return 用户信息
     */
    const std::string &getUserInfo() const { return m_userInfo; }

    /**
     * @brief  返回 host
     * @return host
     */
    const std::string &getHost() const { return m_host; }

    /**
     * @brief  返回 path
     * @return path
     */
    const std::string &getPath() const;

    /**
     * @brief  返回查询条件
     * @return 查询条件
     */
    const std::string &getQuery() const { return m_query; }

    /**
     * @brief  返回 fragment
     * @return fragment
     */
    const std::string &getFragment() const { return m_fragment; }

    /**
     * @brief  返回端口
     * @return 端口
     */
    int32_t getPort() const;

    /**
     * @brief     设置 scheme
     * @param[in] scheme
     */
    void setScheme(const std::string &scheme) { m_scheme = scheme; }

    /**
     * @brief     设置 userInfo
     * @param[in] userInfo
     */
    void setUserInfo(const std::string &userInfo) { m_userInfo = userInfo; }

    /**
     * @brief     设置 host
     * @param[in] host
     */
    void setHost(const std::string &host) { m_host = host; }

    /**
     * @brief     设置 path
     * @param[in] path
     */
    void setPath(const std::string &path) { m_path = path; }

    /**
     * @brief     设置 query
     * @param[in] query
     */
    void setQuery(const std::string &query) { m_query = query; }

    /**
     * @brief     设置 fragment
     * @param[in] fragment
     */
    void setFragment(const std::string &fragment) { m_fragment = fragment; }

    /**
     * @brief     设置 port
     * @param[in] port
     */
    void setPort(int32_t port) { m_port = port; }

    /**
     * @brief     序列化到输出流
     * @param[in] os 输出流
     * @return    输出流
     */
    std::ostream &dump(std::ostream &os) const;

    /**
     * @brief toString
     */
    std::string toString() const;

    /**
     * @brief  创建 Address
     * @return 返回创建好的 Address
     */
    Address::ptr createAddress() const;

  private:
    /**
     * @brief  是否是默认端口
     * @return 返回是否是默认端口
     */
    bool isDefaultPort() const;

  private:
    std::string m_scheme;   /**< scheme */
    std::string m_userInfo; /**< 用户信息 */
    std::string m_host;     /**< host */
    std::string m_path;     /**< 路径 */
    std::string m_query;    /**< 查询参数 */
    std::string m_fragment; /**< fragment */
    int32_t m_port;         /**< port */
};
}; // namespace http
}; // namespace sylar

#endif //  __HTTP_URI_H__
