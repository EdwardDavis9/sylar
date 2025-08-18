#ifndef __HTTP_SESSION_H__
#define __HTTP_SESSION_H__

#include "http/socketstream.hh"
#include "http/http.hh"

namespace sylar {
namespace http {

class HttpSession : public SocketStream {

  public:
    using ptr = std::shared_ptr<HttpSession>;
    HttpSession(Socket::ptr sock, bool owner = true);
    HttpRequest::ptr recvRequest();
    int sendResponse(HttpResponse::ptr rsp);
};

}; // namespace http
}; // namespace sylar

#endif // __HTTP_SESSION_H__
