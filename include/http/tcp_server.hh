#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include <memory>
#include <functional>

#include "sylar/address.hh"
#include "sylar/iomanager.hh"
#include "sylar/socket.hh"
#include "sylar/noncopyable.hh"

namespace sylar {

	class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
	public:
		using ptr = std::shared_ptr<TcpServer>;

		TcpServer(sylar::IOManager* worker = sylar::IOManager::GetThis(),
				  sylar::IOManager* accept_worker = sylar::IOManager::GetThis());
		uint64_t getRecvTimeout() const { return m_recvTimeout; }
		std::string getName() const { return m_name; }
		void setName(const std::string& name) { m_name = name; }
		void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
		bool isStop() const { return m_isStop; }


		virtual ~TcpServer();
		virtual bool bind(sylar::Address::ptr addr);
		virtual bool bind(const std::vector<Address::ptr>& addrs,
						  std::vector<Address::ptr>& fails);
		virtual bool start();
		virtual void stop();

	protected:
		virtual void handleClient(Socket::ptr client);
		virtual void startAccept(Socket::ptr sock);

	private:
		std::vector<Socket::ptr> m_socks;
		IOManager* m_worker;
		IOManager* m_acceptWorker;
		uint64_t m_recvTimeout;
		std::string m_name;
		bool m_isStop;
	};
};


#endif // __SYLAR_TCP_SERVER_H__
