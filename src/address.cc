#include "address.hh"
#include <string.h>
#include <stdint.h>
#include <vector>
#include "log.hh"
#include <sys/types.h>
#include <sys/socket.h>
#include <map>
#include <netdb.h>
#include <arpa/inet.h>
#include "endian.hh"
#include <ifaddrs.h>

namespace sylar {
static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

template <class T>
static T CreateMask(uint32_t net_bits)
{
    // 1 左移 n位, 然后 - 1, 创建一个掩码地址
    // 前 n(网络地址) 位为0, 后 n(主机地址) 位为1
    return (1 << (sizeof(T) * 8 - net_bits)) - 1;
}

template <class T>
static uint32_t CountBytes(T value)
{
    uint32_t result = 0;
    for (; value; ++result) {
        value &= value - 1;
    }
    return result;
}

Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen)
{
    if (addr == nullptr) {
        return nullptr;
    }

    Address::ptr result;
    switch (addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in *)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6 *)addr));
            break;
        default:
            result.reset(new UnknowAddress(*addr));
            break;
    }

    return result;
}

bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
                     int family, int type, int protocol)
{
    std::string node;
    const char *service = nullptr;

    // 一个 ipv6 的地址示例, http://[2001:0:3238:E1:0063:FEFB]:80
    if (!host.empty() && host[0] == '[') {
        const char *endipv6 = static_cast<const char *>(
            memchr(host.c_str() + 1, ']', host.size() - 1));

        if (endipv6) {
            if ((endipv6 + 1 < host.c_str() + host.size())
                && *(endipv6 + 1) == ':')
            {
                service = endipv6 + 2; // 定位服务端口 ip
            }
            node = host.substr(1, endipv6 - host.c_str() - 1); // 定位主机 host
        }
    }

    // 非 IPV6 格式地址
    if (node.empty()) {
        service =
            static_cast<const char *>(memchr(host.c_str(), ':', host.size()));
        if (service) {
            if (!(memchr(service + 1, ':',
                         host.c_str() + host.size() - service - 1)))
            {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    //  非 ipv4 和 ipv6 地址的话
    if (node.empty()) {
        node = host;
    }

    addrinfo hints, *results, *next;

    hints.ai_flags     = 0;
    hints.ai_family    = family;
    hints.ai_socktype  = type;
    hints.ai_protocol  = protocol;
    hints.ai_addrlen   = 0;
    hints.ai_canonname = nullptr;
    hints.ai_addr      = nullptr;
    hints.ai_next      = nullptr;

    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if (error) {
        SYLAR_LOG_ERROR(g_logger)
            << "Address::Lookup getaddress(" << host << ", " << family << ", "
            << type << ") err=" << error << " errstr=" << strerror(errno);
        return false;
    }

    next = results;
    while (next) {
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return true;
}

int Address::getFamily() const { return getAddr()->sa_family; }

std::string Address::toString()
{
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

Address::ptr Address::LookupAny(const std::string &host, int family, int type,
                                int protocol)
{
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

Address::ptr Address::LookupAnyIPAddress(const std::string &host, int family,
                                         int type, int protocol)
{
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        for (auto &i : result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if (v) {
                return v;
            }
        }
    }

    return nullptr;
}

bool Address::operator<(const Address &rhs) const
{
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    int result       = memcmp(getAddr(), rhs.getAddr(), minlen);

    if (result < 0) {
        return true;
    }
    else if (result > 0) {
        return false;
    }
    else if (getAddr() < rhs.getAddr()) {
        return true;
    }

    return false;
}

bool Address::operator==(const Address &rhs) const
{
    return getAddrLen() == rhs.getAddrLen()
           && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address &rhs) const { return !(*this == rhs); }

bool Address::GetInterfaceAddresses(
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
    int family)
{
    struct ifaddrs *next, *results;

    if(getifaddrs(&results) != 0) {
        SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress getifaddrs "
            << " err = " << errno << " errstr = " << strerror(errno);
        return false;
    }

    try  {
        for(next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            switch(next->ifa_addr->sa_family) {
            case AF_INET: {
                addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                uint32_t netmask =
                    ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                prefix_len = CountBytes(netmask);
            }
                break;
            case AF_INET6: {
                addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                in6_addr & netmask =
                    ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                prefix_len = 0;

                for(int i = 0; i < 16; ++i) {
                    prefix_len += CountBytes(netmask.s6_addr[i]);
                }
            }
                break;
            default:
                break;
            }

            if(addr) {
                result.insert(std::make_pair(next->ifa_name,
                                             std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
            SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress exception";
            freeifaddrs(results);
            return false;
    }

    freeifaddrs(results);
    return true;
}


bool Address::GetInterFaceAddresses(
    std::vector<std::pair<Address::ptr, uint32_t>> &result,
    const std::string &iface, int family)
{
    if(iface.empty() || iface == "*") {
        if(family == AF_INET || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results;
    if(!GetInterfaceAddresses(results, family)) {
        return false;
    }
    auto its = results.equal_range(iface);
    for(; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return true;
}

IPAddress::ptr IPAddress::Create(const char *address, uint32_t port)
{
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags  = 0;
    hints.ai_family = AF_UNSPEC;

    int error = getaddrinfo(address, nullptr, &hints, &results);
    if (error) {
        SYLAR_LOG_ERROR(g_logger) << "IPAddress::Create(" << address << ", "
                                  << port << ") errstr=" << strerror(errno);
        return nullptr;
    }

    try {
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
            Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));

        if (result) {
            result->setPort(port);
        }
        freeaddrinfo(results);

        return result; // 返回创建好的对象
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}

IPv4Address::ptr IPv4Address::Create(const char *address, uint32_t port)
{

    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = byteswapOnLittleEndian(port); // question

    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if (result <= 0) {
        SYLAR_LOG_ERROR(g_logger)
            << "IPv4Address::Create(" << address << ", " << port
            << ") rt =" << result << " errno=" << errno
            << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in &address) { m_addr = address; }

IPv4Address::IPv4Address(uint32_t address, uint32_t port)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family      = AF_INET;
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address); // question
    m_addr.sin_port        = byteswapOnLittleEndian(port);
}

const sockaddr *IPv4Address::getAddr() const { return (sockaddr *)&m_addr; }

socklen_t IPv4Address::getAddrLen() const { return sizeof(m_addr); }

std::ostream &IPv4Address::insert(std::ostream &os) const
{
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "." /*移除低24位,保留第1字段,然后十进制输出*/
       << ((addr >> 16) & 0xff)
       << "." /*移除低16位, 保留第2字段, 然后十进制输出*/
       << ((addr >> 8) & 0xff) << "." /*移除低8位, 保留第3字段, 然后十进制输出*/
       << (addr & 0xff);              /*保留第4字段, 然后十进制输出*/

    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len)
{
    // IPv4 地址总共有 32 位, 错误检测
    if (prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |=
        byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));

    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len)
{
    if (prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &=
        byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t perfix_len)
{
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr =
        ~byteswapOnLittleEndian(CreateMask<uint32_t>(perfix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

uint32_t IPv4Address::getPort() const
{
    return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPv4Address::setPort(uint32_t port)
{
    m_addr.sin_port = byteswapOnLittleEndian(port);
}

IPv6Address::ptr IPv6Address::Create(const char *address, uint32_t port)
{
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result           = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if (result <= 0) {
        SYLAR_LOG_ERROR(g_logger)
            << "IPv6Address::Create(" << address << ", " << port
            << ") rt=" << result << " errno=" << errno
            << " errstr=" << strerror(errno);
        return nullptr;
    }

    return rt;
}

IPv6Address::IPv6Address()
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6 &address) { m_addr = address; }

IPv6Address::IPv6Address(const uint8_t address[16], uint32_t port)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port   = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr, address, 16);
}

const sockaddr *IPv6Address::getAddr() const { return (sockaddr *)&m_addr; }

socklen_t IPv6Address::getAddrLen() const { return sizeof(m_addr); }

std::ostream &IPv6Address::insert(std::ostream &os) const
{
    os << "[";

    // 128位ipv6 地址, 强制转换成16位一段, 共8段长度
    uint16_t *addr = (uint16_t *)m_addr.sin6_addr.s6_addr;

    // 用来标记是否已经使用过一次 :: 省略连续 0 的缩写
    // (IPv6 压缩语法只允许使用一次 ::)
    bool used_zeros = false;

    for (size_t i = 0; i < 8; ++i) {
        if (addr[i] == 0 && !used_zeros) {
            continue;
        }

        // 当前不是0, 但是前面一位是0的话
        if (i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }

        // 除了第一个字段, 每个字段都需要添加:
        if (i) {
            os << ":";
        }

        // 变成主机字节序, 以16进制的形式进行输出
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    // 如果还没有进行压缩::,  那么在结尾进行追加
    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len)
{
    // 模拟 ipv6
    sockaddr_in6 baddr(m_addr);

    // 设置某一段的掩码
    // 或操作保留的是前面的网络地址
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
        CreateMask<uint8_t>(prefix_len % 8);

    for (int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff; // 将剩下的主机位全部置1
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len)
{
    sockaddr_in6 baddr(m_addr);

    // 通过 & 清除主机位, 因为创建的掩码一定是网络位＋主机位
    baddr.sin6_addr.s6_addr[prefix_len / 8] &=
        ~CreateMask<uint8_t>(prefix_len % 8);

    for (int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len)
{
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len / 8] =
        ~CreateMask<uint8_t>(prefix_len % 8);

    for (uint32_t i = 0; i < prefix_len / 8; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}

uint32_t IPv6Address::getPort() const
{
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint32_t port)
{
    m_addr.sin6_port = byteswapOnLittleEndian(port);
}

static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

UnixAddress::UnixAddress()
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length          = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string &path)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length          = path.size() + 1;

    if (!path.empty() && path[0] == '\0') {
        --m_length;
    }

    if (m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }

    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}

const sockaddr *UnixAddress::getAddr() const { return (sockaddr *)&m_addr; }

socklen_t UnixAddress::getAddrLen() const { return m_length; }

std::ostream &UnixAddress::insert(std::ostream &os) const
{
    if (m_length > offsetof(sockaddr_un, sun_path)
        && m_addr.sun_path[0] == '\0')
    {
        return os << "\\0"
                  << std::string(m_addr.sun_path + 1,
                                 m_length - offsetof(sockaddr_un, sun_path)
                                     - 1);
    }
    return os << m_addr.sun_path;
}


UnknowAddress::UnknowAddress(int family)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknowAddress::UnknowAddress(const sockaddr& addr)
{
    m_addr = addr;
}

const sockaddr* UnknowAddress::getAddr() const
{
    return &m_addr;
}

socklen_t UnknowAddress::getAddrLen() const
{
    return sizeof(m_addr);
}

std::ostream& UnknowAddress::insert(std::ostream& os) const
{
    os << "[UnknowAddress family = " << m_addr.sa_family << "]";
    return os;
}


}; // namespace sylar
