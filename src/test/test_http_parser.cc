#include "http/http_parser.hh"
#include "sylar/log.hh"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

const char test_request_data[] = "POST / HTTP/1.1\r\n"
                                "Host: www.sylar.top\r\n"
                                // "Connection: Close\r\n"
                                // "Connection: Keep-Alive\r\n"
                                "Content-Length: 10\r\n\r\n"
                                "1234567890";


void test_request() {
	SYLAR_LOG_INFO(g_logger) << "REQUEST START--------------------------";
	sylar::http::HttpRequestParser parser;
	std::string tmp = test_request_data;
	size_t s = parser.execute(&tmp[0], tmp.size());
	SYLAR_LOG_ERROR(g_logger) << "execute rt=" << s
		<< " has_error=" << parser.hasError() << " is_finished="
		<< parser.isFinished() << " total=" << tmp.size()
		<<" conten_legth=" <<parser.getContentLength();
	tmp.resize(tmp.size() - s);

	SYLAR_LOG_INFO(g_logger) << "test_request_data";
	std::cout << test_request_data << std::endl;
	SYLAR_LOG_INFO(g_logger) << "--------------------------";
	SYLAR_LOG_INFO(g_logger) << parser.getData()->toString();
	SYLAR_LOG_INFO(g_logger) << tmp;
	SYLAR_LOG_INFO(g_logger) << "REQUEST DONE--------------------------";
}

void test_response() {

	SYLAR_LOG_INFO(g_logger) << "RESPONSE START--------------------------";
    std::string body = "<html><body>Hello</body></html>";
    SYLAR_LOG_INFO(g_logger) << body.size() << "______===========___";
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "Content-Length: " << body.size() << "\r\n";
    ss << "Content-Type: text/html\r\n";
    ss << "Connection: Close\r\n";
    ss << "\r\n";
    ss << body;
    std::string test_response_data = ss.str();


	sylar::http::HttpResponseParser parser;
	std::string tmp = test_response_data;
	size_t s = parser.execute(&tmp[0], tmp.size(), true);

	SYLAR_LOG_ERROR(g_logger) << "execute rt=" << s
		<< " has_error=" << parser.hasError() << " is_finished="
		<< parser.isFinished() << " total=" << tmp.size()
		<<" conten_legth=" <<parser.getContentLength();
	tmp.resize(tmp.size() - s);
	// tmp.resize(parser.getContentLength());
	SYLAR_LOG_INFO(g_logger) << parser.getData()->toString();
	SYLAR_LOG_INFO(g_logger) << parser.getParser().close;
	SYLAR_LOG_INFO(g_logger) << tmp;
	SYLAR_LOG_INFO(g_logger) << "RESPONSE DONE--------------------------";
}


int main(int argc, char *argv[]) {

	test_request();

	test_response();
	return 0;
}
