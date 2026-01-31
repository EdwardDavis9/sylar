/**
 * @file      address.hh
 * @brief     网络地址的封装
 * @date      Tue Aug 19 09:40:47 2025
 * @author    edward
 * @copyright BSD-3-Clause
 *
 */

#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <iostream>
#include <map>
#include <vector>
#include <stdint.h>
#include <sys/un.h>

namespace sylar {

class IPAddress;

/**
 * @class Address
 * @brief 网络地址的封装, 抽象类
 */
class Address {

  public:
    using ptr = std::shared_ptr<Address>;

	/**
	* @brief     通过 sockaddr 指针创建 Address
	* @param[in] addr sockaddr指针
	* @param[in] addrlen The length of addr
	* @return    返回和 sockaddr 相匹配的 Address, 失败返回 nullptr
	*/
    static Address::ptr Create(const sockaddr *addr, socklen_t addrlen);

	/**
	* @brief     通过 host 地址返回对应条件的所有的 address
	* @param[in] result 保存满足条件的 Address
	* @param[in] host 域名, 服务器名等, 如 www.sylar.top[:80]
	* @param[in] family 协议族(AF_INET, AF_INET, AF_UNIX)
	* @param[in] type   socket 类型: SOCK_STREAM, SOCK_DGRAM 等
	* @param[in] protocol 协议, IPPROTO_TCP, IPPROTO_UDP 等
	* @return    返回和 sockaddr 相匹配的 Address, 失败返回 nullptr
	*/
    static bool Lookup(std::vector<Address::ptr> &result,
                       const std::string &host, int family = AF_INET,
                       int type = 0, int protocol = 0);

	/**
	* @brief     通过 host 地址返回对应条件的任意  Address
	* @param[in] host   域名, 服务器名等, 如 www.sylar.top[:80]
	* @param[in] family 协议族(AF_INET, AF_INET, AF_UNIX)
	* @param[in] type   socket 类型: SOCK_STREAM, SOCK_DGRAM 等
	* @param[in] protocol 协议, IPPROTO_TCP, IPPROTO_UDP 等
	* @return    返回满足条件的任意 Address, 失败返回 nullptr
	*/
    static Address::ptr LookupAny(const std::string &host, int family = AF_INET,
                          int type = 0, int protocol = 0);

	/**
	* @brief     通过 host 地址返回对应条件的任意  IPAddress
	* @param[in] host   域名, 服务器名等, 如 www.sylar.top[:80]
	* @param[in] family 协议族(AF_INET, AF_INET, AF_UNIX)
	* @param[in] type   socket 类型: SOCK_STREAM, SOCK_DGRAM 等
	* @param[in] protocol 协议, IPPROTO_TCP, IPPROTO_UDP 等
	* @return    返回满足条件的任意 Address, 失败返回 nullptr
	*/
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string &host,
										   int family = AF_INET,
										   int type = 0, int protocol = 0);

	/**
	* @brief     返回本机的所有网卡<网卡名, 地址, 子网掩码位数>
	* @param[in] result 保存本机的所有地址
	* @param[in] family 协议族(AF_INET, AF_INET6, AF_UNIX 等)
	* @return    成功返回 true, 失败返回 false
	*/
    static bool GetInterFaceAddresses(
        std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
        int family = AF_INET);

	/**
	* @brief     获取指定网卡的地址和子网掩码位数
	* @param[in] result 保存指定网卡的所有地址
	* @param[in] iface 网卡名称
	* @param[in] family 协议族(AF_INET, AF_INET6, AF_UNIX 等)
	* @return    成功返回 true, 失败返回 false
	*/
    static bool GetInterFaceAddresses(
        std::vector<std::pair<Address::ptr, uint32_t>> &result,
        const std::string &iface, int family = AF_INET);

	/**
	* @brief 虚析构函数
	*/
	virtual ~Address();

	/**
	* @brief 返回协议族
	*/
	int getFamily() const;


	std::string getFamilyToString() const;

	/**
	* @brief 返回一个只读的 sockaddr 指针
	*/
	virtual const sockaddr* getAddr() const = 0;


   /**
	* @brief 返回一个读写的 sockaddr 指针
	*/
	virtual sockaddr* getAddr() = 0;

	/**
	* @brief 返回 sockaddr 的长度
	*/
	virtual socklen_t getAddrLen() const = 0;

   /**
	* @brief 输出可读的地址
	*/
	virtual std::ostream& insert(std::ostream& os) const = 0;

   /**
    * @brief 返回可读性字符串
    */
	std::string toString();

   /**
    * @brief 重载比较函数: <
    */
	bool operator<(const Address& rhs) const;

   /**
    * @brief 重载比较函数: ==
    */
	bool operator==(const Address& rht) const;

   /**
    * @brief 重载比较函数: !=
    */
	bool operator!=(const Address& rhs) const;
};

/**
 * @class IP 地址的基类
 */
class IPAddress : public Address {
public:
	using ptr = std::shared_ptr<IPAddress>;

   /**
    * @brief     通过域名, IP, 服务器名创建 IPAddress
	* @param[in] address 域名,  IP, 服务器名等
	* @param[in] port 端口号
	* @return    成功返回 IPAddress, 失败返回 nullptrr
    */
	static IPAddress::ptr Create(const char* address, uint16_t port = 0);

   /**
    * @brief     获取该地址的广播地址
	* @param[in] prefix_len 端口号
	* @return    成功返回 IPAddress, 失败返回 nullptrr
    */
	virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

   /**
    * @brief     获取该地址的网段
	* @param[in] prefix_len 子网掩码位数
	* @return    成功返回 IPAddress, 失败返回 nullptrr
    */
	virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;

   /**
    * @brief     获取子网掩码地址
	* @param[in] prefix_len 子网掩码位数
	* @return    成功返回 IPAddress, 失败返回 nullptrr
    */
	virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

   /**
    * @brief 返回端口号
    */
	virtual uint32_t getPort() const = 0;

   /**
    * @brief 设置端口号
    */
	virtual void setPort(uint16_t port) = 0;
};

/**
 * @class IPv4 地址
 */
class IPv4Address : public IPAddress {
public:
	using ptr = std::shared_ptr<IPv4Address>;

   /**
    * @brief     使用点分十进制的字符串创建 IPv4Address
	  * @param[in] address 点分十进制地址, 如 192.168l.1.1
	  * @param[in] port 端口号
	  * @return    成功返回 IPv4Address, 失败返回 nullptrr
    */
	static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

   /**
    * @brief     通过 sockaddr_in 构造 IPv4Address
		* @param[in] address sockaddr_in 结构体
    */
	IPv4Address(const sockaddr_in & address);


   /**
    * @brief     通过二进制地址构造 IPv4Address
    * @param[in] address 二进制地址 address
    * @param[in] port 端口号
    */
	IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

	const sockaddr* getAddr() const override;
	sockaddr* getAddr() override;
	socklen_t getAddrLen() const override;
	std::ostream& insert(std::ostream& os) const override;

   /**
    * @brief     获取当前网段的广播地址
    * @param[in] 网络位前缀长度
		* @return    返回 IPv4Address 对象, 地址为 网络地址 + 主机位全 1
    */
	IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

   /**
    * @brief     获取当前 IP 对应的主机网络地址
    * @param[in] 网络位前缀长度
		* @return    返回 IPv4Address 对象, 地址为 网络位清零 + 保留主机位
    *
    */
	IPAddress::ptr networkAddress(uint32_t prefix_len) override;

   /**
    * @brief     获取 子网掩码地址(标准意义上的网络掩码)
    * @param[in] 网络位前缀长度
		* @return    返回 IPv4Address 对象, 地址为 网络位全 1 + 主机位全 0
    */
	IPAddress::ptr subnetMask(uint32_t perfix_len) override;

	uint32_t getPort() const override;
	void setPort(uint16_t port) override;

	private:
	sockaddr_in m_addr;
};

/**
 * @class IPv6Address
 */
class IPv6Address : public IPAddress {
public:
	using ptr = std::shared_ptr<IPv6Address>;

   /**
    * @brief     通过 IPv6 地址字符串 构造 IPv6Address
    * @param[in] address IPv6 地址字符串
    * @param[in] port 端口号
    */
	static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

   /**
    * @brief 无参构造函数
    */
	IPv6Address();

   /**
    * @brief     通过 sockaddr_in6 构造 IPv6Address
    * @param[in] address sockaddr_in6 结构体
    */
	IPv6Address(const sockaddr_in6& address);

   /**
    * @brief     通过 IPv6 二进制地址构造 IPv6Address
    * @param[in] address IPv6 二进制地址
    */
	IPv6Address(const uint8_t address[16], uint16_t port = 0);

	const sockaddr* getAddr() const override;
	sockaddr* getAddr() override;
	socklen_t getAddrLen() const override;
	std::ostream& insert(std::ostream& os) const override;

	IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
	IPAddress::ptr networkAddress(uint32_t prefix_len) override;
	IPAddress::ptr subnetMask(uint32_t prefix_len) override;
	uint32_t getPort() const override;
	void setPort(uint16_t port) override;

	private:
	sockaddr_in6 m_addr;
};

/**
 * @class UnixAddress
 */
class UnixAddress : public Address {
public:
	using ptr = std::shared_ptr<UnixAddress>;

   /**
    * @brief 无参构造函数
    */
	UnixAddress();

   /**
    * @brief     通过路径构造 UnixAddress
    * @param[in] path UnixSocket 路径
    */
	UnixAddress(const std::string& path);

	const sockaddr* getAddr() const override;
	sockaddr* getAddr() override;
	socklen_t getAddrLen() const override;
	std::ostream& insert(std::ostream& os) const override;

	void setAddrLen(uint32_t v);

	private:
	sockaddr_un m_addr;
	socklen_t m_length; /**< unix 地址结构的总大小 */

};

/**
 * @class UnknowAddress
 */
class UnknowAddress : public Address {
	public:
	using ptr = std::shared_ptr<UnknowAddress>;

	UnknowAddress(int family);
	UnknowAddress(const sockaddr& addr);
	const sockaddr* getAddr() const override;
	sockaddr* getAddr() override;
	socklen_t getAddrLen() const override;
	std::ostream& insert(std::ostream& os) const override;

	private:
	sockaddr m_addr;
};

/**
 * @brief 流式输出 Address
 */
std::ostream& operator<<(std::ostream& os, const Address& addr);

}; // namespace sylar

#endif // __SYLAR_ADDRESS_H__
