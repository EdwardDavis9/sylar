#include "http/http.hh"
#include "sylar/log.hh"

void test_request() {
	sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
	req->setHeader("host", "www.bilibili.com");
	req->setBody("hello sylar");
	// req->dump(std::cout) << std::endl;
	std::cout << req->toString();
}

void test_response() {
	sylar::http::HttpResponse::ptr rsp(new sylar::http::HttpResponse);
	rsp->setHeader("X-X", "sylar");
	rsp->setBody("hello sylar");
	rsp->setStatus((sylar::http::HttpStatus)400);
	rsp->setClose(true);

	rsp->dump(std::cout) << std::endl;
}

int main() {

	test_request();

	test_response();
}
