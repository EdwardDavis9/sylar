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
			const http_parser& getParser() const { return m_parser; }

		public:
			static uint64_t GetHttpRequestBufferSize();
			static uint64_t GetHttpRequestMaxBodySize();

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

			/**
			* @brief         解析数据
			* @param[in,out] data 待解析的数据
			* @param[in]     len 待解析数据的长度
			* @param[in]     chunck 是否分段
			* @return        成功解析的字符数
			*/
			size_t execute(char* data, size_t len, bool chunck);
			int isFinished();
			int hasError();

			HttpResponse::ptr getData() const { return m_data; }
			void setError(int v) { m_error = v; }

			uint64_t getContentLength();
			const httpclient_parser& getParser() const { return m_parser; }

		public:
			static uint64_t GetHttpResponseBufferSize();
			static uint64_t GetHttpResponseMaxBodySize();


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
