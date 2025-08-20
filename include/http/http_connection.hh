#ifndef __SYLAR_HTTP_CONNECTION_H__
#define __SYLAR_HTTP_CONNECTION_H__

#include "http/socketstream.hh"
#include "http/http.hh"

namespace sylar {
namespace http {
class HttpConnection : public SocketStream {
  public:
    using ptr = std::shared_ptr<HttpConnection>;

    HttpConnection(Socket::ptr sock, bool owner = true);
    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);
};
}; // namespace http
}; // namespace sylar

#endif // __SYLAR_HTTP_CONNECTION_H__
