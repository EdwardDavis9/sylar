#include "sylar/address.hh"
#include "sylar/log.hh"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test()
{

    std::vector<sylar::Address::ptr> addrs;

    SYLAR_LOG_INFO(g_logger) << "begin";
    bool v = sylar::Address::Lookup(addrs, "www.baidu.com");

    if (!v) {
        SYLAR_LOG_ERROR(g_logger) << "lookup fail";
    }

    for (size_t i = 0; i < addrs.size(); ++i) {
        SYLAR_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
    SYLAR_LOG_INFO(g_logger) << "end";
}

void test_iface()
{
    std::multimap<std::string, std::pair<sylar::Address::ptr, uint32_t>>
        results;

    bool v = sylar::Address::GetInterFaceAddresses(results);

    if (!v) {
        SYLAR_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for (auto &i : results) {
        SYLAR_LOG_INFO(g_logger)
            << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

void test_ipv4()
{
    // auto addr1 = sylar::IPAddr1ess::Create("www.bilibili.com");
    // auto addr1 = sylar::IPAddr1ess::Create("www.sylar.top");
    auto addr1 = sylar::IPAddress::Create("127.0.0.1");

    if (addr1) {
        SYLAR_LOG_INFO(g_logger) << addr1->toString();
        SYLAR_LOG_INFO(g_logger) << addr1->getFamily();
        SYLAR_LOG_INFO(g_logger) << addr1->getFamilyToString();
        SYLAR_LOG_INFO(g_logger) << addr1->networkAddress(24)->toString();
        SYLAR_LOG_INFO(g_logger) << addr1->subnetMask(24)->toString();
    }


    auto addr2 = sylar::IPAddress::Create("192.168.1.23");
    if (!addr2) {
        SYLAR_LOG_ERROR(g_logger) << "create ip addr2ess failed";
        return;
    }

    SYLAR_LOG_INFO(g_logger) << "IP: " << addr2->toString();

    std::cout << std::endl;
    // 测试前缀长度 24、16、8
    uint32_t prefixes[] = {24, 16, 8};
    for (uint32_t prefix : prefixes) {
        auto net = addr2->networkAddress(prefix);
        auto broad = addr2->broadcastAddress(prefix);
        auto mask = addr2->subnetMask(prefix);

        SYLAR_LOG_INFO(g_logger) << "prefix=" << prefix
                                 << " net=" << net->toString()
                                 << " broadcast=" << broad->toString()
                                 << " mask=" << mask->toString();
    }
}


int main(int argc, char *argv[])
{

    // test();
    // test_iface();
    test_ipv4();


    return 0;
}
