#include "http/http_session.hh"
#include "http/http_parser.hh"
#include "http/socketstream.hh"
#include "sylar/hook.hh"

namespace sylar {

namespace http {
HttpSession::HttpSession(Socket::ptr sock, bool owner)
	:SocketStream(sock, owner)
{ }

HttpRequest::ptr HttpSession::recvRequest()
{
	HttpRequestParser::ptr parser(new HttpRequestParser);
	uint64_t buffer_size = HttpRequestParser::GetHttpRequestBufferSize();
	// uint64_t buffer_size = 100;

	std::shared_ptr<char> buffer(
		new char[buffer_size], [](char* ptr) {
			delete[] ptr;
		});

	char* data = buffer.get();
	int unparsed_offset  = 0;
	do {
		int read_size
			= read(data + unparsed_offset, buffer_size - unparsed_offset);
		if (read_size <= 0) {
			return nullptr;
		}
		read_size += unparsed_offset;

		// 解析数据
		size_t nparser = parser->execute(data, read_size);
		if(parser->hasError()) {
			return nullptr;
		}

		unparsed_offset = read_size - nparser;
		if(unparsed_offset == (int)buffer_size) {
			// 首次请求解析的数据等于当前的 buffer_size, 那么说明是恶意请求, 数据过大
			return nullptr;
		}

		if(parser->isFinished()) {
			break;
		}
	} while(true);

	int64_t content_size = parser->getContentLength();
	if(content_size > 0) {
		std::string body;

		#if 1
		body.reserve(content_size);
		size_t already_use = 0;
		 if ((int64_t)unparsed_offset > 0) {

            // 这里只能拷贝正文长度那么多
            // unparsed_offset 可能大于正文长度(后面跟着下一个请求的起始)
            size_t take = std::min((size_t)unparsed_offset,
								   (size_t)content_size);
            body.append(data, take);
            already_use = take;
        }

        // 还需要从 socket 读多少
        size_t need = (size_t)content_size - already_use;
        if (need > 0) {
            size_t old = body.size();
            body.resize(old + need); // 先扩大 size
            if (readFixSize(&body[old], need) <= 0) {
                return nullptr;
            }
        }
		#endif

		#if 0
		body.resize(content_size);
		int len = 0;
		if(content_size >= unparsed_offset) {
			memcpy(&body[0], data, unparsed_offset);
	    	// 缓冲区里的数据还不足以把整个 body 填满, 即还存在剩余的请求体
			body.append(data, unparsed_offset);
		}
		else {
			memcpy(&body[0], data, content_size);
		    // 缓冲区中的未解析数据比 content_size 还大
			// 说明缓冲区里的数据已经能把整个 body 填满, 即还存其他的请求数据
			// 因此本次请求解析只需要读取 content_size 个数据即可
			body.append(data, content_size);
			len = content_size;
		}
		content_size -= unparsed_offset;

		if(content_size > 0) {
			如果还未读取完毕请求体的话, 接下来读取剩下的请求体
			if(readFixSize(&body[body.size()], content_size) <= 0) {
				return nullptr;
			}
		}
		#endif

		parser->getData()->setBody(body);
	}
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
