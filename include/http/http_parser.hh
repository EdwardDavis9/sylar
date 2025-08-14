#ifndef __SYLAR_HTTP_PARSER_H__
#define __SYLAR_HTTP_PARSER_H__

#include "http.hh"
#include "http11_parser.hh"
#include "httpclient_parser.hh"

#include <memory>
#include <sys/types.h>

namespace sylar {
	namespace http {
		class HttpRequestParser {
		public:
			using ptr = std::shared_ptr<HttpRequestParser>;
			HttpRequestParser();
			size_t execute(char* data, size_t len);
			int isFinished();
			int hasError();

			HttpRequest::ptr getData() const { return m_data; }
			void setError(int v) { m_error = v; }

			uint64_t getContentLength();

		private:
			http_parser m_parser;
			HttpRequest::ptr m_data;
			// 1000:invaild method
			// 1001:invaild version
			// 1002:invaild filed
			int m_error;
			};

		class HttpResponseParser {
		public:
			using ptr = std::shared_ptr<HttpResponseParser>;
			HttpResponseParser();

			size_t execute(char* daata, size_t len);
			int isFinished();
			int hasError();

			HttpResponse::ptr getData() const { return m_data; }
			void setError(int v) { m_error = v; }

			uint64_t getContentLength();

		private:
		httpclient_parser m_parser;
		HttpResponse::ptr m_data;
		// 1000: invaild version
		// 1002: invaild field
		int m_error;
		};
	};
};



#endif // __SYLAR_HTTP_PARSER_H__
