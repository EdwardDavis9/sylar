#include "http/http_session.hh"
#include "http/http_parser.hh"
#include "http/socketstream.hh"
#include "sylar/hook.hh"

#include "sylar/log.hh"

namespace sylar {

namespace http {
HttpSession::HttpSession(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner)
{}

HttpRequest::ptr HttpSession::recvRequest()
{
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buffer_size = HttpRequestParser::GetHttpRequestBufferSize();

    std::shared_ptr<char> buffer(new char[buffer_size],
                                 [](char *ptr) { delete[] ptr; });

    char *data        = buffer.get();
    int unparsed_data = 0;
    do {
        // [请求行 + 请求头][请求体][下一个请求行 + 请求头 + ...]
        // unparsed_offset 表示本次read过程中没有解析的偏移
        // content-size 表示本次数据包中的 数据体的大小

        // 获取数据
        int read_size = read(data + unparsed_data, buffer_size - unparsed_data);
        if (read_size <= 0) {
            close();
            return nullptr;
        }
        read_size += unparsed_data;

        // 解析数据
        size_t nparser = parser->execute(data, read_size);
        if (parser->hasError()) {
            close();
            return nullptr;
        }

        // 本轮中读到的数据 - 已经解析的数据,这也就对读到数据的一个偏移
        unparsed_data = read_size - nparser;
        if (unparsed_data == (int)buffer_size) {
            // 首次请求解析的数据等于当前的 buffer_size, 那么说明是恶意请求,
            // 数据过大
            close();
            return nullptr;
        }

        if (parser->isFinished()) {
            // 解析完成后, 退出循环
            break;
        }
    } while (true);

    int64_t content_size = parser->getContentLength();
    if (content_size > 0) {
        std::string body;

        body.reserve(content_size);
        size_t already_use = 0;
        if ((int64_t)unparsed_data > 0) {

            // 这里只能拷贝正文长度那么多
            // unparsed_offset 可能大于正文长度(后面跟着下一个请求的起始)

            // 这里通过min((size_t)unparsed_offset,(size_t)content_size)
            // 不能多取 → 不要把下一请求的数据当成当前请求体,只取 content_size
            // 不能少取 → 如果请求体还没读完,就先把已收到的部分放进 body,只取
            // unparsed_offset
            size_t take = std::min((size_t)unparsed_data, (size_t)content_size);
            body.append(data, take);
            already_use = take;
        }

        // 手动读取剩余的请求体数据: content_size - take
        size_t need = (size_t)content_size - already_use;
        if (need > 0) {
            size_t old = body.size();
            body.resize(old + need); // 先扩大 size
            if (readFixSize(&body[old], need) <= 0) {
                close();
                return nullptr;
            }
        }

        parser->getData()->setBody(body);
    }

    // // 增加对长连接的设置
    // std::string keep_alive = parser->getData()->getHeader("Connection");
    // if (!strcasecmp(keep_alive.c_str(), "keep-alive")) {
    //     parser->getData()->setClose(false);
    // } else {
    //     parser->getData()->setClose(true);
    // }

    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp)
{
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

} // namespace http
} // namespace sylar
