#ifndef __SYLAR_HTTP_PARSER_H__
#define __SYLAR_HTTP_PARSER_H__

#include "http.hh"
#include "http11_parser.hh"
#include "httpclient_parser.hh"

#include <memory>
#include <sys/types.h>

namespace sylar {
	namespace http {

    /**
     * @brief HTTP 请求解析类
     */
		class HttpRequestParser {
		public:
			using ptr = std::shared_ptr<HttpRequestParser>;

      /**
       * @brief 构造函数
       */
			HttpRequestParser();

      /**
       * @brief          解析协议
       * @param[in, out] data 协议文本内存
       * @param[in]      len 协议文本内存长度
       * @return         返回实际解析的长度,并且将已解析的数据移除
       */
			size_t execute(char* data, size_t len);

      /**
       * @brief  是否解析完成
       * @return 是否解析完成
       */
			int isFinished();

      /**
       * @brief  是否有错误
       * @return 是否有错误
       */
			int hasError();

      /**
       * @brief 返回 HttpRequest 结构体
       */
			HttpRequest::ptr getData() const { return m_data; }

      /**
       * @brief     设置错误
       * @param[in] v 错误值
			 *            1000:invaild method
			 *            1001:invaild version
			 *            1002:invaild filed
       */
			void setError(int v) { m_error = v; }

      /**
       * @brief 获取消息体长度
       */
			uint64_t getContentLength();

      /**
       * @brief 获取http_parser结构体
       */
			const http_parser& getParser() const { return m_parser; }

		public:

      /**
       * @brief 返回 HttpRequest 协议解析的缓存大小
       */
			static uint64_t GetHttpRequestBufferSize();

      /**
       * @brief 返回 HttpRequest 协议的最大消息体大小
       */
			static uint64_t GetHttpRequestMaxBodySize();

		private:
			http_parser m_parser; /**< http_parser */
			HttpRequest::ptr m_data; /**< data */
			int m_error;
			};

    /**
     * @brief Http响应解析结构体
     */
		class HttpResponseParser {
		public:
			using ptr = std::shared_ptr<HttpResponseParser>;

      /**
       * @brief 构造函数
       */
			HttpResponseParser();

			/**
			* @brief         解析数据
			* @param[in,out] data 待解析的数据
			* @param[in]     len 待解析数据的长度
			* @param[in]     chunck 是否分段
			* @return        成功解析的字符数
			*/
			size_t execute(char* data, size_t len, bool chunck);

      /**
       * @brief 是否解析完成
       */
			int isFinished();

      /**
       * @brief 是否有错误
       */
			int hasError();

      /**
       * @brief 返回 HttpResponse
       */
			HttpResponse::ptr getData() const { return m_data; }

      /**
       * @brief     设置错误码
       * @param[in] v 错误码
			 *            1001: invaild version
		   *            1002: invaild field
       */
			void setError(int v) { m_error = v; }

      /**
       * @brief 获取消息体长度
       */
			uint64_t getContentLength();

      /**
       * @brief 返回 httpclient_parser
       */
			const httpclient_parser& getParser() const { return m_parser; }

		public:

      /**
       * @brief 返回 HTTP 响应解析缓存大小
       */
			static uint64_t GetHttpResponseBufferSize();

      /**
       * @brief 返回 HTTP 响应最大消息体大小
       */
			static uint64_t GetHttpResponseMaxBodySize();


		private:
		httpclient_parser m_parser; /**< httpclient_parser */
		HttpResponse::ptr m_data; /**< httpresponse */
		int m_error;
		};
	};
};



#endif // __SYLAR_HTTP_PARSER_H__
