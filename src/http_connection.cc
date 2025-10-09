#include "http/http_connection.hh"
#include "http/http_parser.hh"
#include "http/socketstream.hh"
#include "sylar/hook.hh"
#include "sylar/log.hh"

namespace sylar {

namespace http {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

std::string HttpResult::toString() const
{
    std::stringstream ss;
    ss << "[HttpResult result=" << m_result << " error=" << m_error
       << " response=" << (m_response ? m_response->toString() : "nullptr")
       << "]";
    return ss.str();
}

HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner)
{}

HttpConnection::~HttpConnection()
{
    SYLAR_LOG_DEBUG(g_logger) << "HttpConnection::~HttpConnection";
}

HttpResponse::ptr HttpConnection::recvResponse()
{
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t response_buffer_size =
        HttpResponseParser::GetHttpResponseBufferSize();
    // uint64_t response_buffer_size = 100;

    std::shared_ptr<char> buffer(new char[response_buffer_size + 1],
                                 [](char *ptr) { delete[] ptr; });

    char *data          = buffer.get();
    int unparsed_offset = 0; // 已经解析的数据, 或者说未解析数据的位置
    // 接受并解析头部字段
    do {
        int read_size = read(data + unparsed_offset,
                             response_buffer_size - unparsed_offset);
        if (read_size <= 0) {
            close();
            return nullptr;
        }
        read_size += unparsed_offset;

        data[read_size] = '\0';

        size_t nparser = parser->execute(data, read_size, false);
        if (parser->hasError()) {
            close();
            return nullptr;
        }

        unparsed_offset = read_size - nparser;
        if (unparsed_offset == (int)response_buffer_size) {
            // 首次请求解析的数据等于当前的 response_buffer_size,
            // 那么说明是恶意请求, 数据过大
            close();
            return nullptr;
        }

        if (parser->isFinished()) {
            break;
        }
    } while (true);

    auto &client_parser = parser->getParser();
    if (client_parser.chunked) {
        // 是否是 chunked body, chunk 还有一个小型头, 即长度+数据的形式
        std::string body;
        int unparsed_index = unparsed_offset;
        // 分块传输
        do {
            do {
                int read_size = read(data + unparsed_index,
                                     response_buffer_size - unparsed_index);
                if (read_size <= 0) {
                    close();
                    return nullptr;
                }
                unparsed_index += read_size;

                // chunk 的长度, 需要分段读的情况下, 需要手动添加末尾结束符
                data[unparsed_index] = '\0';

                // 第三个参数为 true, 即使用 chunk, 每次读取都需要重新 init
                size_t nparse = parser->execute(data, unparsed_index, true);
                if (parser->hasError()) {
                    close();
                    return nullptr;
                }
                unparsed_index -= nparse;
                if (unparsed_index == (int)response_buffer_size) {
                    close();
                    return nullptr;
                }
            } while (!parser->isFinished());

            // crlf 占据两个空间
            // 每个 chunk 的数据后面都有 CRLF (\r\n)
            // 减去 2 表示 去掉末尾的 CRLF, 准备拼接到 body
            unparsed_index -= 2;

            SYLAR_LOG_INFO(g_logger)
                << "client_content-len=" << client_parser.content_len;

            // 每个 chunked 段的实际大小： 长度头 + 实际的数据体
            if (client_parser.content_len <= unparsed_index) {
                // 在解析时, 头部字段内容已经被消耗, 追加前 conten_len 个
                body.append(data, client_parser.content_len);
                memmove(data,data+client_parser.content_len,
                       unparsed_index-client_parser.content_len);

                unparsed_index -= client_parser.content_len;
            } else {
                // 只读取固定前长度
                body.append(data, unparsed_index);
                int left = client_parser.content_len - unparsed_index;
                while(left > 0) {
                    int read_size = read(data, left > (int)response_buffer_size
                                         ? (int)response_buffer_size:left);
                    if(read_size <=  0) {
                        close();
                        return nullptr;
                    }
                    body.append(data, read_size);
                    left -= read_size;
                }
                unparsed_index = 0;
            }
        } while (!client_parser.chunks_done);
        parser->getData()->setBody(body);
     }
    else {
        int64_t content_size = parser->getContentLength();
        if (content_size > 0) {
            std::string body;

#if 0
            body.reserve(content_size);
            size_t already_use = 0;
            if ((int64_t)unparsed_offset > 0) {

                // 这里只能拷贝正文长度那么多
                // unparsed_offset 可能大于正文长度(后面跟着下一个请求的起始)
                size_t take =
                    std::min((size_t)unparsed_offset, (size_t)content_size);
                body.append(data, take);
                already_use = take;
            }

            // 还需要从 socket 读多少
            size_t need = (size_t)content_size - already_use;
            if (need > 0) {
                size_t old = body.size();
                body.resize(old + need); // 先扩大 size
                if (readFixSize(&body[old], need) <= 0) {
                    close();
                    return nullptr;
                }
            }
#endif

#if 1
            body.resize(content_size);
            int len = 0;
            if (content_size >= unparsed_offset) {
                memcpy(&body[0], data, unparsed_offset);
                len = unparsed_offset;
            }
            else {
                memcpy(&body[0], data, content_size);
                // 缓冲区中的未解析数据比 content_size 还大
                // 说明缓冲区里的数据已经能把整个 body 填满,
                // 即还存其他的请求数据 因此本次请求解析只需要读取 content_size
                // 个数据即可
                len = content_size;
            }
            content_size -= unparsed_offset;

            if (content_size > 0) {
                // 如果还未读取完毕请求体的话, 接下来读取剩下的请求体
                if (readFixSize(&body[len], content_size) <= 0) {
                    close();
                    return nullptr;
                }
            }
#endif

            parser->getData()->setBody(body);
        }
    }
    return parser->getData();
}

int HttpConnection::sendRequest(HttpRequest::ptr rsp)
{
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

HttpResult::ptr
HttpConnection::DoGet(const std::string &url, uint64_t timeout_ms,
                      const std::map<std::string, std::string> &headers,
                      const std::string &body)
{
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVAILD_URL,
                                            nullptr, "invaild url:" + url);
    }

    return DoGet(uri, timeout_ms, headers, body);
}

HttpResult::ptr
HttpConnection::DoGet(Uri::ptr uri, uint64_t timeout_ms,
                      const std::map<std::string, std::string> &headers,
                      const std::string &body)
{
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr
HttpConnection::DoPost(const std::string &url, uint64_t timeout_ms,
                       const std::map<std::string, std::string> &headers,
                       const std::string &body)
{
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVAILD_URL,
                                            nullptr, "invaild url:" + url);
    }
    return DoPost(uri, timeout_ms, headers, body);
}

HttpResult::ptr
HttpConnection::DoPost(Uri::ptr uri, uint64_t timeout_ms,
                       const std::map<std::string, std::string> &headers,
                       const std::string &body)
{
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(
    HttpMethod method, const std::string &url, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers, const std::string &body)
{
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVAILD_URL,
                                            nullptr, "invaild url:" + url);
    }
    return DoPost(uri, timeout_ms, headers, body);
}

HttpResult::ptr
HttpConnection::DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
                          const std::map<std::string, std::string> &headers,
                          const std::string &body)
{
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(method);

    bool has_host = false;
    for (auto &i : headers) {
        if (strcasecmp(i.first.c_str(), "connection") == 0) {
            if (strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }

        if (!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        req->setHeader(i.first, i.second);
    }

    if (!has_host) {
        req->setHeader("Host", uri->getHost());
    }

    req->setBody(body);
    return DoRequest(req, uri, timeout_ms);
}

HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req, Uri::ptr uri,
                                          uint64_t timeout_ms)
{
    // 创建 Host
    Address::ptr addr = uri->createAddress();
    if (!addr) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::INVAILD_HOST, nullptr,
            "invaild host: " + uri->getHost());
    }

    // 创建 socket
    Socket::ptr sock = Socket::CreateTCP(addr);
    if (!sock) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::CREATE_SOCKET_ERROR, nullptr,
            "create socket fail " + addr->toString());
    }
    if (!sock->connect(addr)) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::CONNECT_FAIL, nullptr,
            "connect fail" + addr->toString());
    }

    // 超时时间
    sock->setRecvTimeout(timeout_ms);

    // 获得连接对象
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    int rt                   = conn->sendRequest(req);
    if (rt == 0) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr,
            "send request closed by peer: " + addr->toString());
    }
    if (rt < 0) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr,
            "send request socket error errno=" + std::to_string(errno)
                + " errstr=" + std::string(strerror(errno)));
    }
    auto rsp = conn->recvResponse();
    if (!rsp) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::TIMEOUT, nullptr,
            "recv response timeout" + addr->toString());
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}

HttpConnectionPool::HttpConnectionPool(const std::string &host,
                                       const std::string &vhost, uint32_t port,
                                       uint32_t max_size,
                                       uint32_t max_alive_time,
                                       uint32_t max_request)
    : m_host(host), m_vhost(vhost), m_port(port), m_maxSize(max_size),
      m_maxAliveTime(max_alive_time), m_maxRequest(max_request)
{}

HttpConnection::ptr HttpConnectionPool::getConnection()
{
    uint64_t now_ms     = sylar::GetCurrentMS();
    HttpConnection *ptr = nullptr;

    std::vector<HttpConnection *> invaild_conns;
    MutexType::Lock lock(m_mutex);

    // 获取有效的连接
    while (!m_conns.empty()) {
        auto conn = *m_conns.begin();
        m_conns.pop_front();
        if (!conn->isConnected()) {
            invaild_conns.push_back(conn);
            continue;
        }
        if ((conn->m_createTime + m_maxAliveTime) > now_ms) {
            invaild_conns.push_back(conn);
            continue;
        }
        ptr = conn;
        break;
    }
    lock.unlock();

    // 统一删除无效的连接
    for (auto &i : invaild_conns) {
        delete i;
    }

    m_total -= invaild_conns.size();

    // 若不存在有效的连接, 那么创建一个连接
    if (!ptr) {
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
        if (!addr) {
            SYLAR_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }

        addr->setPort(m_port);
        Socket::ptr sock = Socket::CreateTCP(addr);

        if (!sock) {
            SYLAR_LOG_ERROR(g_logger) << "create sock faild" << *addr;
            return nullptr;
        }

        if (!sock->connect(addr)) {
            SYLAR_LOG_ERROR(g_logger) << "socket connect faile: " << *addr;
            return nullptr;
        }

        ptr = new HttpConnection(sock);
        ++m_total;
    }

    return HttpConnection::ptr(
        ptr, [this](HttpConnection *p) { this->ReleasePtr(p, this); });
}

void HttpConnectionPool::ReleasePtr(HttpConnection *ptr,
                                    HttpConnectionPool *pool)
{
    ++ptr->m_request;
    if (!ptr->isConnected()
        || ((ptr->m_createTime + pool->m_maxAliveTime) >= sylar::GetCurrentMS())
        || (ptr->m_request >= pool->m_maxRequest))
    {

        delete ptr;
        --pool->m_total;
        return;
    }
    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
}

HttpResult::ptr
HttpConnectionPool::doGet(const std::string &url, uint64_t timeout_ms,
                          const std::map<std::string, std::string> &headers,
                          const std::string &body)
{
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}

HttpResult::ptr
HttpConnectionPool::doGet(Uri::ptr uri, uint64_t timeout_ms,
                          const std::map<std::string, std::string> &headers,
                          const std::string &body)
{
    std::stringstream ss;
    ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery() << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();

    return doGet(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr
HttpConnectionPool::doPost(const std::string &url, uint64_t timeout_ms,
                           const std::map<std::string, std::string> &headers,
                           const std::string &body)
{
    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}

HttpResult::ptr
HttpConnectionPool::doPost(Uri::ptr uri, uint64_t timeout_ms,
                           const std::map<std::string, std::string> &headers,
                           const std::string &body)
{
    std::stringstream ss;
    ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery() << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doPost(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(
    HttpMethod method, const std::string &url, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers, const std::string &body)
{
    // 构造请求对象
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(url);      // 路径
    req->setMethod(method); // 方法
    req->setClose(false);   // 关闭
    bool has_host = false;  // Host 标识

    // 遍历用户传入的 headers 字段
    for (auto &i : headers) {
        if (strcasecmp(i.first.c_str(), "connection") == 0) {
            if (strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }
        if (!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty(); // 进行头部标识
        }

        req->setHeader(i.first, i.second); // 头部字段
    }

    // 若没有 host 字段, 设置默认字段
    if (!has_host) {
        if (m_vhost.empty()) {
            req->setHeader("Host", m_host);
        }
        else {
            req->setHeader("Host", m_vhost);
        }
    }
    req->setBody(body); // 设置请求体

    return doRequest(req, timeout_ms);
}

HttpResult::ptr HttpConnectionPool::doRequest(
    HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string> &headers, const std::string &body)
{

    std::stringstream ss;
    ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery() << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req,
                                              uint64_t timeout_ms)
{
    // 获得一个连接
    auto conn = getConnection();
    if (!conn) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::POOL_GET_CONNECTION, nullptr,
            "pool host: " + m_host + " port:" + std::to_string(m_port));
    }

    // 获得一个 socket
    auto sock = conn->getSocket();
    if (!sock) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::POOL_INVAILD_CONNECTION, nullptr,
            "pool host:" + m_host + " port:" + std::to_string(m_port));
    }

    // 设置超时
    sock->setRecvTimeout(timeout_ms);

    // 发送请求
    int rt = conn->sendRequest(req);
    if (rt == 0) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr,
            "send request closed by peer:"
                + sock->getRemoteAddress()->toString());
    }

    if (rt < 0) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr,
            "send request socket error errno=" + std::to_string(errno)
                + " errstr=" + std::string(strerror(errno)));
    }

    auto rsp = conn->recvResponse();
    // 为简化代码, 响应失败一律视为响应超时
    if (!rsp) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::Error::TIMEOUT, nullptr,
            "recv response timeout:" + sock->getRemoteAddress()->toString()
                + " timeout_ms:" + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}

} // namespace http
} // namespace sylar
