#include "http/uri.hh"

#include <iostream>

int main(int argc, char *argv[])
{
    sylar::http::Uri::ptr uri = sylar::http::Uri::Create(
        "http://www.baidu.com:80/test/uri?id=100&name=sylar#frag");

    // sylar::http::Uri::ptr uri =
    //     sylar::http::Uri::Create("http://admin@www.sylar.top/test/中文/"
    //                              "uri?id=100&name=sylar&vv=中文#frg中文");

    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}
