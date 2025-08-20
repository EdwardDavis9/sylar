#include "http/http_connection.hh"
#include "http/http_parser.hh"
#include "http/socketstream.hh"
#include "sylar/hook.hh"
#include "sylar/log.hh"

namespace sylar {

namespace http {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner)
{}

HttpResponse::ptr HttpConnection::recvResponse()
{
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t response_buffer_size =
        HttpResponseParser::GetHttpResponseBufferSize();
    // uint64_t response_buffer_size = 100;

    std::shared_ptr<char> buffer(new char[response_buffer_size + 1],
                                 [](char *ptr) { delete[] ptr; });

    char *data          = buffer.get();
    int unparsed_offset = 0; // 已经解析的数据，或者说未解析数据的位置
    do {
        int read_size = read(data + unparsed_offset,
                             response_buffer_size - unparsed_offset);
        if (read_size <= 0) {
            close();
            return nullptr;
        }
        read_size += unparsed_offset;

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
        std::string body;
        int unparsed_index = unparsed_offset;
        do {
            do {
                int read_size = read(data + unparsed_index,
                                     response_buffer_size - unparsed_index);
                if (read_size <= 0) {
                    close();
                    return nullptr;
                }
                unparsed_index += read_size;
                data[unparsed_index] =
                    '\0'; // 分段读的情况下，需要手动添加末尾结束符
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

            unparsed_index -= 2;

            SYLAR_LOG_INFO(g_logger)
                << "client_len=" << client_parser.content_len;

            if (client_parser.content_len <= unparsed_index) {
                body.append(data, client_parser.content_len);
                int left = client_parser.content_len - unparsed_index;
                while (left > 0) {
                    int read_size = read(data, left > (int)response_buffer_size
                                                   ? (int)response_buffer_size
                                                   : left);

                    if (read_size <= 0) {
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

#if 1
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

#if 0
		body.resize(content_size);
		int len = 0;
		if(content_size >= unparsed_offset) {
			memcpy(&body[0], data, unparsed_offset);
			len = unparsed_offset;
		}
		else {
			memcpy(&body[0], data, content_size);
		    // 缓冲区中的未解析数据比 content_size 还大
			// 说明缓冲区里的数据已经能把整个 body 填满, 即还存其他的请求数据
			// 因此本次请求解析只需要读取 content_size 个数据即可
			len = content_size;
		}
		content_size -= unparsed_offset;

		if(content_size > 0) {
			如果还未读取完毕请求体的话, 接下来读取剩下的请求体
			if(readFixSize(&body[len], content_size) <= 0) {
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

} // namespace http
} // namespace sylar
